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

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#include <cap/caplist.h>
#include <cap/cap.h>
#include <abi/cap.h>
#include <adt/list.h>
#include <mm/slab.h>
#include <proc/task.h>
#include <synch/mutex.h>
#include <syscall/copy.h>

static slab_cache_t *caplist_cache;

/** Initialize the caplist subsystem. */
void caplist_init(void)
{
	caplist_cache = slab_cache_create("caplist_t", sizeof(caplist_t), 0,
	    NULL, NULL, 0);
}

static void caplist_initialize(caplist_t *cl, kobject_type_t type)
{
	cl->type = type;
	mutex_initialize(&cl->mutex, MUTEX_PASSIVE);
	list_initialize(&cl->objects);
	list_initialize(&cl->queue);
	condvar_initialize(&cl->cv);
}

static void caplist_destroy(void *arg)
{
	caplist_t *cl = (caplist_t *) arg;

	slab_free(caplist_cache, cl);
}

kobject_ops_t caplist_kobject_ops = {
	.destroy = caplist_destroy
};

/** Create capability list for kobjects of specified type.
 *
 * @param clhandle  User address of the capability handle that will receive the
 *                  handle of the newly created capability list.
 * @param type      Type of kernel objects that can be stored in the capability
 *                  list.
 *
 * @return          EOK on success.
 * @return          Error code on failure.
 */
sys_errno_t sys_caplist_create(uspace_ptr_cap_caplist_handle_t clhandle,
    kobject_type_t type)
{
	cap_handle_t handle;
	errno_t rc = cap_alloc(TASK, &handle);
	if (rc != EOK)
		return rc;

	rc = copy_to_uspace(clhandle, &handle, sizeof(cap_handle_t));
	if (rc != EOK) {
		cap_free(TASK, handle);
		return rc;
	}

	caplist_t *cl = (caplist_t *) slab_alloc(caplist_cache, FRAME_ATOMIC);
	if (!cl) {
		cap_free(TASK, handle);
		return ENOMEM;
	}

	kobject_t *kobject = kobject_alloc(FRAME_ATOMIC);
	if (!kobject) {
		slab_free(caplist_cache, cl);
		cap_free(TASK, handle);
		return ENOMEM;
	}

	caplist_initialize(cl, type);
	kobject_initialize(kobject, KOBJECT_TYPE_CAPLIST, cl);
	cap_publish(TASK, handle, kobject);
	if (rc != EOK)
		kobject_put(kobject);

	return rc;
}

/** Destroy capability list.
 *
 * @param clhandle  Capability handle of the capability list to be destroyed.
 *
 * @return          EOK on success.
 * @return          Error code on failure.
 */
sys_errno_t sys_caplist_destroy(cap_caplist_handle_t clhandle)
{
	kobject_t *kobj = cap_unpublish(TASK, clhandle, KOBJECT_TYPE_CAPLIST);
	if (!kobj)
		return ENOENT;

	kobject_put(kobj);
	return cap_free(TASK, clhandle);
}

/** Add kernel object to a capability list.
 *
 * @param cl  Capability list to which to add the kernel object.
 * @param k   Kernel kobject to be added to the capability list.
 *
 * @return    EOK on success.
 * @return    Error code on failure.
 */
errno_t caplist_add(caplist_t *cl, kobject_t *k)
{
	assert(mutex_locked(&cl->mutex));
	assert(mutex_locked(&k->lock));

	if (k->in_caplist)
		return EBUSY;
	k->in_caplist = cl;
	list_append(&k->cl_link, &cl->objects);
	kobject_add_ref(k);
	return EOK;
}
/** Remove kernel object from a capability list.
 *
 * @param cl  Capability list from which to remove the kernel object.
 * @param k   Kernel kobject to be removed from the capability list.
 *
 * @return    EOK on success.
 * @return    Error code on failure.
 */
errno_t caplist_del(caplist_t *cl, kobject_t *k)
{
	assert(mutex_locked(&cl->mutex));
	assert(mutex_locked(&k->lock));

	if (k->in_caplist != cl)
		return ENOENT;
	k->in_caplist = NULL;
	list_remove(&k->cl_link);
	kobject_put(k);
	return EOK;
}

/** Add kernel object to a capability list.
 *
 * @param clhandle  Capability handle of the capability list to which to add
 *                  the kernel object referred by \c handle.
 * @param handle    Capability handle of the kernel object to be added to the
 *                  capability list.
 *
 * @return          EOK on success.
 * @return          Error code on failure.
 */
sys_errno_t sys_caplist_add(cap_caplist_handle_t clhandle, cap_handle_t handle)
{
	kobject_t *cl_kobj = kobject_get(TASK, clhandle, KOBJECT_TYPE_CAPLIST);
	if (!cl_kobj)
		return ENOENT;
	caplist_t *cl = cl_kobj->caplist;

	kobject_t *kobj = kobject_get(TASK, handle, cl->type);
	if (!kobj) {
		kobject_put(cl_kobj);
		return ENOENT;
	}

	mutex_lock(&cl->mutex);
	mutex_lock(&kobj->lock);

	errno_t rc = caplist_add(cl, kobj);

	mutex_unlock(&kobj->lock);
	mutex_unlock(&cl->mutex);

	kobject_put(kobj);
	kobject_put(cl_kobj);

	return rc;
}

/** Remove kernel object from a capability list.
 *
 * @param clhandle  Capability handle of the capability list from which to
 *                  remove the kernel object referred by \c handle.
 * @param handle    Capability handle of the kernel object to be removed from
 *                  the capability list.
 *
 * @return          EOK on success.
 * @return          Error code on failure.
 */
sys_errno_t sys_caplist_del(cap_caplist_handle_t clhandle, cap_handle_t handle)
{
	kobject_t *cl_kobj = kobject_get(TASK, clhandle, KOBJECT_TYPE_CAPLIST);
	if (!cl_kobj)
		return ENOENT;
	caplist_t *cl = cl_kobj->caplist;

	kobject_t *kobj = kobject_get(TASK, handle, cl->type);
	if (!kobj) {
		kobject_put(cl_kobj);
		return ENOENT;
	}

	mutex_lock(&cl->mutex);
	mutex_lock(&kobj->lock);

	errno_t rc = caplist_del(cl, kobj);

	mutex_unlock(&kobj->lock);
	mutex_unlock(&cl->mutex);

	kobject_put(kobj);
	kobject_put(cl_kobj);

	return rc;
}

/** @}
 */
