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

#ifndef KERN_IPC2_H_
#define KERN_IPC2_H_

#include <cap/cap.h>
#include <cap/caplist.h>
#include <adt/list.h>
#include <abi/ipc2/ipc.h>
#include <abi/synch.h>
#include <synch/mutex.h>

typedef enum {
	IPC_BUF_READY,
	IPC_BUF_PENDING,
	IPC_BUF_FINISHED
} ipc_buf_state_t;

struct ipc_ep;

/** IPC buffer.
 *
 * IPC buffers are units of information exchange between communicating parties.
 * Each buffer has a fixed size specified at the time of its creation. Both
 * their number and their sizes can be arbitrary, subject to resources available
 * to the task. Each IPC buffer can be used for indefinite number of IPC
 * roundtrips.
 */
typedef struct ipc_buf {
	/** Mutex protecting the IPC buffer. */
	mutex_t mutex;
	/** State of the buffer. */
	ipc_buf_state_t state;

	/**
	 * Result of the buffer. EOK if the buffer was finished or error code if
	 * the buffer was failed.
	 */
	errno_t wait_result;

	/** Total size of @c data in bytes. */
	size_t size;
	/** How much of the buffer contains valid data. */
	size_t used;
	/** Allocated memory buffer of @c size bytes. */
	uint8_t *data;
	/** Linkage for the IPC endpoint's buffer_list or caplist's queue. */
	link_t link;

	/**
	 * Address of the IPC endpoint in which the buffer is enqueued, if any.
	 */
	struct ipc_ep *in_ep;

	/**
	 * Address of the IPC endpoint caplist in which the buffer is enqueued,
	 * if any.
	 */
	caplist_t *in_ep_caplist;

	/** User-defined label.*/
	uspace_addr_t buf_label;
	/** Label of the last IPC endpoint which received the buffer. */
	uspace_addr_t ep_label;

	/** Condition variable used for waiting on the buffer being finished. */
	condvar_t cv;

	/** Associated kernel object. */
	kobject_t *kobject;
} ipc_buf_t;

/** IPC endpoint.
 *
 * IPC endpoints are asynchronous communication endpoints via which
 * communicating parties exchange IPC buffers. The owner of the endpoint can
 * receive from it and all other tasks can send to it, provided they have the
 * respective capability. The endpoint is essentially a queue of IPC buffers.
 */
typedef struct ipc_ep {
	/** Mutex protecting the endpoint. */
	mutex_t mutex;
	/** Condvar used for synchronization. */
	condvar_t cv;
	/** List of queued IPC buffers. */
	list_t buffers;
	/** User-defined label. */
	uspace_addr_t label;
	/** Associated kernel object. */
	kobject_t *kobject;
} ipc_ep_t;


extern kobject_ops_t ipc_buf_kobject_ops;
extern kobject_ops_t ipc_ep_kobject_ops;

extern void ipc2_init(void);

extern sys_errno_t sys_ipc2_buf_alloc(size_t, uspace_addr_t,
    uspace_ptr_cap_ipc_buf_handle_t);
extern sys_errno_t sys_ipc2_buf_free(cap_ipc_buf_handle_t);

extern sys_errno_t sys_ipc2_ep_create(uspace_ptr_cap_ipc_ep_handle_t,
    uspace_addr_t, cap_caplist_handle_t);
extern sys_errno_t sys_ipc2_ep_destroy(cap_ipc_ep_handle_t);

extern sys_errno_t sys_ipc2_buf_send(uspace_addr_t, size_t,
    cap_ipc_buf_handle_t, cap_ipc_ep_handle_t, cap_caplist_handle_t);
extern sys_errno_t sys_ipc2_buf_receive(uspace_addr_t, size_t, cap_handle_t,
    cap_handle_t, synch_timeout_t, uspace_ptr_ipc_buf_receive_info_t);

extern sys_errno_t sys_ipc2_buf_finish(uspace_addr_t, size_t,
    cap_ipc_buf_handle_t);
extern sys_errno_t sys_ipc2_buf_wait(uspace_addr_t, size_t, cap_handle_t,
    synch_timeout_t, bool, uspace_ptr_ipc_buf_wait_info_t);

#endif

/** @}
 */
