/*
 * Copyright (c) 2019 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup kernel_generic_ipc
 * @{
 */
/** @file
 */

#include <ipc2/ipc.h>
#include <abi/ipc2/ipc.h>
#include <cap/cap.h>
#include <cap/caplist.h>
#include <mm/slab.h>
#include <mem.h>
#include <errno.h>
#include <proc/task.h>
#include <syscall/copy.h>
#include <synch/mutex.h>

#include <stdlib.h>

static slab_cache_t *ipc_buf_cache;
static slab_cache_t *ipc_ep_cache;

static void ipc2_buf_finish(ipc_buf_t *, errno_t);

/** Initialize the IPC v2 subsystem. */
void ipc2_init(void)
{
	ipc_buf_cache = slab_cache_create("ipc_buf_t", sizeof(ipc_buf_t), 0,
	    NULL, NULL, 0);
	ipc_ep_cache = slab_cache_create("ipc_ep_t", sizeof(ipc_ep_t), 0,
	    NULL, NULL, 0);
}

static void
ipc_buf_initialize(ipc_buf_t *buf, size_t size, void *data, uspace_addr_t label,
    kobject_t *kobj)
{
	memset(buf, 0, sizeof(*buf));
	mutex_initialize(&buf->mutex, MUTEX_PASSIVE);
	link_initialize(&buf->link);
	buf->state = IPC_BUF_READY;
	buf->used = 0;
	buf->size = size;
	buf->data = data;
	buf->buf_label = label;
	buf->ep_label = 0;
	buf->in_ep = NULL;
	buf->in_ep_caplist = NULL;
	condvar_initialize(&buf->cv);
	buf->kobject = kobj;
}

static void ipc_buf_destroy(void *arg)
{
	ipc_buf_t *buf = (ipc_buf_t *) arg;

	free(buf->data);
	slab_free(ipc_buf_cache, buf);
}

kobject_ops_t ipc_buf_kobject_ops = {
	.destroy = ipc_buf_destroy
};

/** Allocate IPC buffer of desired size.
 *
 * @param[in] size      Desired size of the IPC buffer.
 * @param[in] label     User-defined label.
 * @param[out] bhandle  Userspace address of the variable which will receive the
 *                      capability handle of the allocated IPC buffer.
 *
 * @return              EOK on success.
 * @return              Error code on failure.
 */
sys_errno_t
sys_ipc2_buf_alloc(size_t size, uspace_addr_t label,
    uspace_ptr_cap_ipc_buf_handle_t bhandle)
{
	cap_handle_t handle;
	errno_t rc = cap_alloc(TASK, &handle);
	if (rc != EOK)
		return rc;

	rc = copy_to_uspace(bhandle, &handle, sizeof(cap_handle_t));
	if (rc != EOK) {
		cap_free(TASK, handle);
		return rc;
	}

	ipc_buf_t *buf = (ipc_buf_t *) slab_alloc(ipc_buf_cache,
	    FRAME_ATOMIC);
	if (!buf) {
		cap_free(TASK, handle);
		return ENOMEM;
	}

	void *data = malloc(size);
	if (!data) {
		slab_free(ipc_buf_cache, buf);
		cap_free(TASK, handle);
		return ENOMEM;
	}

	kobject_t *kobject = kobject_alloc(FRAME_ATOMIC);
	if (!kobject) {
		free(data);
		slab_free(ipc_buf_cache, buf);
		cap_free(TASK, handle);
		return ENOMEM;
	}

	ipc_buf_initialize(buf, size, data, label, kobject);
	kobject_initialize(kobject, KOBJECT_TYPE_IPC_BUF, buf);
	rc = cap_publish(TASK, handle, kobject);
	if (rc != EOK)
		kobject_put(kobject);

	return rc;
}

/** Free IPC buffer.
 *
 * @param[in] bhandle  Capability handle of the IPC buffer to free.
 *
 * @return             EOK on success.
 * @return             Error code on failure.
 */
sys_errno_t
sys_ipc2_buf_free(cap_ipc_buf_handle_t bhandle)
{
	kobject_t *kobj = cap_unpublish(TASK, bhandle, KOBJECT_TYPE_IPC_BUF);
	if (!kobj)
		return ENOENT;

	kobject_put(kobj);
	return cap_free(TASK, bhandle);
}

static errno_t ipc2_buf_send_check(ipc_buf_t *buf, caplist_t *cl)
{
	assert(mutex_locked(&buf->mutex));

	if (buf->state == IPC_BUF_FINISHED) {
		/*
		 * Finished buffers must be first made ready again by waiting on
		 * them.
		 */
		return EBUSY;
	}

	if (buf->in_ep || buf->in_ep_caplist) {
		/*
		 * Cannot send the buffer if it is already queueing in an
		 * endpoint or and endpoint caplist.
		 */
		return EBUSY;
	}

	assert(buf->state == IPC_BUF_READY || buf->state == IPC_BUF_PENDING);

	if (cl) {
		if (buf->state == IPC_BUF_READY && buf->kobject->in_caplist) {
			/*
			 * The buffer is already in a caplist.
			 */
			return EBUSY;
		}
		if (buf->state == IPC_BUF_PENDING) {
			/*
			 * Cannot put a pending buffer into a caplist.
			 */
			return EINVAL;
		}
	}

	return EOK;
}

static errno_t ipc2_copy_to_buf(ipc_buf_t *buf, uspace_addr_t src, size_t size)
{
	assert(mutex_locked(&buf->mutex));

	if (size > buf->size)
		return ELIMIT;
	errno_t rc = copy_from_uspace(buf->data, src, size);
	if (rc != EOK)
		return rc;
	if (size)
		buf->used = size;
	return EOK;
}

static errno_t ipc2_copy_from_buf(ipc_buf_t *buf, uspace_addr_t dst,
    size_t size)
{
	assert(mutex_locked(&buf->mutex));

	if (size > buf->used)
		size = buf->used;
	errno_t rc = copy_to_uspace(dst, buf->data, size);
	if (rc != EOK)
		return rc;
	return EOK;
}

/** Unpublish an IPC buffer capability during a forwarding send or finish.
 *
 * @param bhandle  IPC buffer capability handle. This capability is expected to
 *                 be in the published state and still associated with the
 *                 same buffer as after sys_ipc2_buf_receive().
 */
static void ipc2_unpublish_temp_cap(cap_ipc_buf_handle_t bhandle)
{
	kobject_t *k = cap_unpublish(TASK, bhandle,
	    KOBJECT_TYPE_IPC_BUF);
	/*
	 * We tolerate the possibility of the user task tampering with the
	 * capability handle. This might result in the user task entering some
	 * undefined state, but the kernel will not be impacted.
	 */
	if (k)
		kobject_put(k);
}

/** Send IPC buffer to IPC endpoint.
 *
 * @param[in] src       Source userspace address of data to be copied into the
 *                      buffer.
 * @param[in] size      Size of the source data. The size can be zero, in which
 *                      case the used size of the buffer is not updated and no
 *                      data is copied.
 * @param[in] bhandle   Capability handle of the IPC buffer. For forwarding
 *                      sends the capability gets unpublished.
 * @param[in] ehandle   Capability handle of the IPC endpoint.
 * @param[in] clhandle  Optional capability handle of an IPC buffer capability
 *                      list. If not CAP_NIL and bhandle is in the ready state
 *                      then the buffer will be added to the capabitlity list.
 *
 * @return              EOK on success.
 * @return              Error code on failure.
 */
sys_errno_t
sys_ipc2_buf_send(uspace_addr_t src, size_t size,
    cap_ipc_buf_handle_t bhandle, cap_ipc_ep_handle_t ehandle,
    cap_caplist_handle_t clhandle)
{
	kobject_t *buf_kobj = kobject_get(TASK, bhandle, KOBJECT_TYPE_IPC_BUF);
	if (!buf_kobj)
		return ENOENT;
	ipc_buf_t *buf = buf_kobj->ipc_buf;

	kobject_t *ep_kobj = kobject_get(TASK, ehandle, KOBJECT_TYPE_IPC_EP);
	if (!ep_kobj) {
		kobject_put(buf->kobject);
		return ENOENT;
	}
	ipc_ep_t *ep = ep_kobj->ipc_ep;

	caplist_t *cl = NULL;
	kobject_t *cl_kobj = NULL;
	if (clhandle != CAP_NIL) {
		cl_kobj = kobject_get(TASK, clhandle, KOBJECT_TYPE_CAPLIST);
		if (!cl_kobj) {
			kobject_put(buf_kobj);
			kobject_put(ep_kobj);
			return ENOENT;
		}
		cl = cl_kobj->caplist;
	}

	/*
	 * Lock everything in the locking order.
	 */
	mutex_lock(&ep->mutex);
	mutex_lock(&ep->kobject->lock);
	if (ep->kobject->in_caplist)
		mutex_lock(&ep->kobject->in_caplist->mutex);
	mutex_lock(&buf->mutex);
	if (cl)
		mutex_lock(&cl->mutex);
	mutex_lock(&buf->kobject->lock);

	errno_t rc = ipc2_buf_send_check(buf, cl);
	if (rc == EOK)
		rc = ipc2_copy_to_buf(buf, src, size);
	if (rc != EOK)
		goto out;

	if (cl) {
		/*
		 * Put the buffer into the caplist for waiting on multiple
		 * finished buffers.
		 */
		rc = caplist_add(cl, buf->kobject);
		/* We checked for errors already in ipc2_buf_send_check(). */
		assert(rc == EOK);
	}

	if (buf->state == IPC_BUF_PENDING)
		ipc2_unpublish_temp_cap(bhandle);

	buf->state = IPC_BUF_PENDING;

	/* Imprint the endpoint's label on the buffer */
	buf->ep_label = ep->label;

	kobject_add_ref(buf_kobj);	/* for adding to the EP/CL list */
	if (ep->kobject->in_caplist) {
		list_append(&buf->link, &ep->kobject->in_caplist->queue);
		buf->in_ep_caplist = ep->kobject->in_caplist;
		condvar_signal(&ep->kobject->in_caplist->cv);
	} else {
		list_append(&buf->link, &ep->buffers);
		buf->in_ep = ep;
		condvar_signal(&ep->cv);
	}

out:
	mutex_unlock(&buf->kobject->lock);
	if (cl)
		mutex_unlock(&cl->mutex);
	mutex_unlock(&buf->mutex);
	if (ep->kobject->in_caplist)
		mutex_unlock(&ep->kobject->in_caplist->mutex);
	mutex_unlock(&ep->kobject->lock);
	mutex_unlock(&ep->mutex);

	kobject_put(buf_kobj);
	kobject_put(ep_kobj);
	if (cl)
		kobject_put(cl_kobj);

	return rc;
}

static errno_t ipc2_block_on(list_t *list, condvar_t *cv, mutex_t *mutex,
    uint32_t usec, unsigned long flags, ipc_buf_t **buf, bool unlock)
{
	mutex_lock(mutex);
	while (list_empty(list)) {
		errno_t rc = _condvar_wait_timeout(cv, mutex, usec, flags);
		if (rc != EOK) {
			mutex_unlock(mutex);
			return rc;
		}
		// TODO: update usec
	}

	assert(!list_empty(list));
	/* Hand over the list's reference to *buf */
	*buf = list_get_instance(list_first(list), ipc_buf_t, link);
	mutex_lock(&(*buf)->mutex);
	list_remove(&(*buf)->link);
	if (unlock)
		mutex_unlock(mutex);

	return EOK;
}

/** Receive IPC buffer from an IPC endpoint.
 *
 * @param[in] dst      Destination userspace address for copying data out of the
 *                     buffer.
 * @param[in] size     Size of the destination buffer.
 * @param[in] bhandle  Allocated, unpublished capability handle that will be
 *                     associated with the received IPC buffer and published.
 * @param[in] ehandle  IPC endpoint capability (list) handle from which to
 *                     receive.
 * @param[in] timeout  Synchronization timeout.
 * @param[out] info    Userspace address that will receive the IPC buffer info
 *                     structure.
 *
 * @return             EOK on success.
 * @return             Error code on failure.
 */
sys_errno_t sys_ipc2_buf_receive(uspace_addr_t dst, size_t size,
    cap_handle_t bhandle, cap_handle_t ehandle, synch_timeout_t timeout,
    uspace_ptr_ipc_buf_receive_info_t info)
{
	errno_t rc;
	kobject_t *kobj;
	ipc_buf_t *buf;

	unsigned long flags = synch_timeout_flags(timeout) |
	    SYNCH_FLAGS_INTERRUPTIBLE;
	uint32_t usec = synch_timeout_usec(timeout);

	if ((kobj = kobject_get(TASK, ehandle, KOBJECT_TYPE_IPC_EP))) {
		rc = ipc2_block_on(&kobj->ipc_ep->buffers, &kobj->ipc_ep->cv,
		    &kobj->ipc_ep->mutex, usec, flags, &buf, true);
		kobject_put(kobj);
		if (rc != EOK)
			return rc;
		buf->in_ep = NULL;
	} else if ((kobj = kobject_get(TASK, ehandle, KOBJECT_TYPE_CAPLIST))) {
		if (kobj->caplist->type != KOBJECT_TYPE_IPC_EP) {
			kobject_put(kobj);
			return EINVAL;
		}
		rc = ipc2_block_on(&kobj->caplist->queue, &kobj->caplist->cv,
		    &kobj->caplist->mutex, usec, flags, &buf, true);
		kobject_put(kobj);
		if (rc != EOK)
			return rc;
		buf->in_ep_caplist = NULL;
	} else {
		return ENOENT;
	}

	rc = ipc2_copy_from_buf(buf, dst, size);
	if (rc != EOK)
		goto error;

	ipc_buf_receive_info_t _info = {
		.ep_label = buf->ep_label,
		.used = buf->used,
		.size = buf->size,
	};

	rc = copy_to_uspace(info, &_info, sizeof(_info));
	if (rc != EOK)
		goto error;

	/*
	 * Take an extra reference to buf so that we can work with it even
	 * after publishing a new capability to it.
	 */
	kobject_add_ref(buf->kobject);

	/* Transfer buf's reference to bhandle on success */
	rc = cap_publish(TASK, bhandle, buf->kobject);
	if (rc != EOK) {
		/*
		 * bhandle does not correspond to an allocated, unpublished
		 * capability, we still have the reference. We also need to
		 * drop the extra reference.
		 */
		kobject_put(buf->kobject);
		goto error;
	}

	mutex_unlock(&buf->mutex);
	kobject_put(buf->kobject);

	return EOK;

error:
	ipc2_buf_finish(buf, rc);
	mutex_unlock(&buf->mutex);
	kobject_put(buf->kobject);
	return rc;
}

static void ipc2_buf_finish(ipc_buf_t *buf, errno_t rc)
{
	assert(mutex_locked(&buf->mutex));

	mutex_lock(&buf->kobject->lock);

	buf->state = IPC_BUF_FINISHED;
	buf->wait_result = rc;

	if (buf->kobject->in_caplist) {
		kobject_add_ref(buf->kobject);
		mutex_lock(&buf->kobject->in_caplist->mutex);
		list_append(&buf->link, &buf->kobject->in_caplist->queue);
		mutex_unlock(&buf->kobject->in_caplist->mutex);
		condvar_signal(&buf->kobject->in_caplist->cv);
	} else {
		condvar_signal(&buf->cv);
	}

	mutex_unlock(&buf->kobject->lock);
}

static errno_t ipc2_buf_finish_check(ipc_buf_t *buf)
{
	if (buf->state != IPC_BUF_PENDING)
		return EINVAL;
	if (buf->in_ep || buf->in_ep_caplist)
		return EBUSY;
	return EOK;
}

/** Mark IPC buffer finished.
 *
 * @param[in] src      Source userspace address of data to be copied into the
 *                     buffer.
 * @param[in] size     Size of the source data.
 * @param[in] bhandle  Capability handle of the IPC buffer. The capability gets
 *                     unpublised.
 *
 * @return             EOK on success.
 * @return             Error code on failure.
 */
sys_errno_t
sys_ipc2_buf_finish(uspace_addr_t src, size_t size,
    cap_ipc_buf_handle_t bhandle)
{
	kobject_t *buf_kobj = kobject_get(TASK, bhandle, KOBJECT_TYPE_IPC_BUF);
	if (!buf_kobj)
		return ENOENT;
	ipc_buf_t *buf = buf_kobj->ipc_buf;
	mutex_lock(&buf->mutex);

	errno_t rc = ipc2_buf_finish_check(buf);
	if (rc == EOK)
		rc = ipc2_copy_to_buf(buf, src, size);
	if (rc != EOK) {
		mutex_unlock(&buf->mutex);
		kobject_put(buf_kobj);
		return rc;
	}

	if (buf->state == IPC_BUF_PENDING)
		ipc2_unpublish_temp_cap(bhandle);

	ipc2_buf_finish(buf, EOK);

	mutex_unlock(&buf->mutex);
	kobject_put(buf_kobj);
	return EOK;
}

/** Wait for an IPC buffer to be finished.
 *
 * @param[in] dst      Destination userspace address for copying data out of
 *                     the buffer.
 * @param[in] size     Size of the destination buffer.
 * @param[in] bhandle  IPC buffer capability (list) handle to wait on.
 * @param[in] timeout  Synchronization timeout.
 * @param[in] delist   If true bhandle is an IPC buffer capability list handle,
 *                     the finished buffer will be removed from the capability
 *                     list.
 * @param[out] info    Userspace address that will receive the IPC buffer info
 *                     structure.
 *
 * @return             EOK on success.
 * @return             Error code on failure.
 */
sys_errno_t
sys_ipc2_buf_wait(uspace_addr_t dst, size_t size, cap_handle_t bhandle,
    synch_timeout_t timeout, bool delist, uspace_ptr_ipc_buf_wait_info_t info)
{
	errno_t rc;
	kobject_t *kobj;
	ipc_buf_t *buf;

	unsigned long flags = synch_timeout_flags(timeout) |
	    SYNCH_FLAGS_INTERRUPTIBLE;
	uint32_t usec = synch_timeout_usec(timeout);

	if ((kobj = kobject_get(TASK, bhandle, KOBJECT_TYPE_IPC_BUF))) {
		buf = kobj->ipc_buf;
		mutex_lock(&buf->mutex);
		while (buf->state != IPC_BUF_FINISHED) {
			rc = _condvar_wait_timeout(&buf->cv, &buf->mutex, usec,
			    flags);
			if (rc != EOK)
				goto error;
			// TODO: update usec
		}
	} else if ((kobj = kobject_get(TASK, bhandle, KOBJECT_TYPE_CAPLIST))) {
		if (kobj->caplist->type != KOBJECT_TYPE_IPC_BUF) {
			kobject_put(kobj);
			return EINVAL;
		}
		caplist_t *cl = kobj->caplist;
		rc = ipc2_block_on(&cl->queue, &cl->cv, &cl->mutex, usec, flags,
		    &buf, false);
		if (rc != EOK) {
			kobject_put(kobj);
			return rc;
		}

		/* ipc2_block_on() must not unlock the cl on success */
		assert(mutex_locked(&cl->mutex));

		if (delist) {
			/*
			 * If the buffer comes from a caplist, we need to delist
			 * it so that it can be put into the same, different or
			 * no caplist at all during the next send.
			 */
			mutex_lock(&buf->kobject->lock);
			rc = caplist_del(cl, buf->kobject);
			assert(rc == EOK);
			mutex_unlock(&buf->kobject->lock);
		}

		mutex_unlock(&cl->mutex);

	} else {
		return ENOENT;
	}

	buf->state = IPC_BUF_READY;

	rc = ipc2_copy_from_buf(buf, dst, size);
	if (rc != EOK)
		goto error;

	ipc_buf_wait_info_t _info = {
		.buf_label = buf->buf_label,
		.used = buf->used,
		.size = buf->size,
		.result = buf->wait_result,
	};

	rc = copy_to_uspace(info, &_info, sizeof(_info));

error:
	mutex_unlock(&buf->mutex);
	kobject_put(buf->kobject);

	return rc;
}

static void
ipc_ep_initialize(ipc_ep_t *ep, uspace_addr_t label, kobject_t *kobj)
{
	memset(ep, 0, sizeof(*ep));
	mutex_initialize(&ep->mutex, MUTEX_PASSIVE);
	condvar_initialize(&ep->cv);
	list_initialize(&ep->buffers);
	ep->label = label;
	ep->kobject = kobj;
}

static void ipc_ep_destroy(void *arg)
{
	ipc_ep_t *ep = (ipc_ep_t *) arg;

	slab_free(ipc_ep_cache, ep);
}

kobject_ops_t ipc_ep_kobject_ops = {
	.destroy = ipc_ep_destroy
};

/** Create an IPC endpoint.
 *
 * @param[out] ehandle  Userspace address which will receive the capability
 *                      handle of the created endpoint.
 * @param[in] label     User-defined label associated with the endpoint.
 * @param[in] clhandle  If not CAP_NIL, the endpoint is added to a caplist
 *                      referred by this capability handle.
 *
 * @return              EOK on success.
 * @return              Error code on failure.
 */
sys_errno_t
sys_ipc2_ep_create(uspace_ptr_cap_ipc_ep_handle_t ehandle, uspace_addr_t label,
    cap_caplist_handle_t clhandle)
{
	cap_handle_t handle;
	errno_t rc = cap_alloc(TASK, &handle);
	if (rc != EOK)
		return rc;

	rc = copy_to_uspace(ehandle, &handle, sizeof(cap_handle_t));
	if (rc != EOK) {
		cap_free(TASK, handle);
		return rc;
	}

	ipc_ep_t *ep = (ipc_ep_t *) slab_alloc(ipc_ep_cache, FRAME_ATOMIC);
	if (!ep) {
		cap_free(TASK, handle);
		return ENOMEM;
	}

	kobject_t *kobject = kobject_alloc(FRAME_ATOMIC);
	if (!kobject) {
		slab_free(ipc_ep_cache, ep);
		cap_free(TASK, handle);
		return ENOMEM;
	}

	ipc_ep_initialize(ep, label, kobject);
	kobject_initialize(kobject, KOBJECT_TYPE_IPC_EP, ep);
	cap_publish(TASK, handle, kobject);
	if (rc != EOK) {
		kobject_put(kobject);
		return rc;
	}

	if (clhandle != CAP_NIL) {
		rc = sys_caplist_add(clhandle, handle);
		if (rc != EOK)
			(void) sys_ipc2_ep_destroy(handle);
	}

	return rc;
}

/** Destroy an IPC endpoint.
 *
 * @param[in] ehandle  Capability handle of the IPC endpoint to destroy.
 *
 * @return             EOK on success.
 * @return             Error code on failure.
 */
sys_errno_t
sys_ipc2_ep_destroy(cap_ipc_ep_handle_t ehandle)
{
	kobject_t *kobj = cap_unpublish(TASK, ehandle, KOBJECT_TYPE_IPC_EP);
	if (!kobj)
		return ENOENT;

	kobject_put(kobj);
	return cap_free(TASK, ehandle);
}

/** @}
 */
