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

/** @addtogroup libc
 * @{
 * @}
 */

/** @addtogroup libcipc2 IPC v2
 * @brief HelenOS uspace IPC v2
 * @{
 * @ingroup libc
 */
/** @file
 */

#include <ipc2/ipc.h>
#include <abi/synch.h>
#include <abi/syscall.h>
#include <libc.h>
#include <errno.h>

#include <stdlib.h>
#include <stdbool.h>

/** @copydoc sys_ipc2_buf_alloc() */
errno_t ipc2_buf_alloc(size_t size, void *label, cap_ipc_buf_handle_t *bhandle)
{
	return __SYSCALL3(SYS_IPC2_BUF_ALLOC, (sysarg_t) size,
	    (sysarg_t) label, (sysarg_t) bhandle);
}

/** @copydoc sys_ipc2_buf_free() */
errno_t ipc2_buf_free(cap_ipc_buf_handle_t bhandle)
{
	return __SYSCALL1(SYS_IPC2_BUF_FREE, cap_handle_raw(bhandle));
}

/** @copydoc sys_ipc2_ep_create() */
errno_t ipc2_ep_create(cap_ipc_ep_handle_t *ehandle, void *label,
    cap_caplist_handle_t clhandle)
{
	return __SYSCALL3(SYS_IPC2_EP_CREATE, (sysarg_t) ehandle,
	    (sysarg_t) label, cap_handle_raw(clhandle));
}

/** @copydoc sys_ipc2_ep_destroy() */
errno_t ipc2_ep_destroy(cap_ipc_ep_handle_t ehandle)
{
	return __SYSCALL1(SYS_IPC2_EP_DESTROY, cap_handle_raw(ehandle));
}

/** @copydoc sys_ipc2_buf_send() */
errno_t ipc2_buf_send(const void *src, size_t size,
    cap_ipc_buf_handle_t bhandle, cap_ipc_ep_handle_t ehandle,
    cap_caplist_handle_t clhandle)
{
	return __SYSCALL5(SYS_IPC2_BUF_SEND, (sysarg_t) src, (sysarg_t) size,
	    cap_handle_raw(bhandle), cap_handle_raw(ehandle),
	    cap_handle_raw(clhandle));
}

/** @copydoc sys_ipc2_buf_receive() */
errno_t ipc2_buf_receive(void *dst, size_t size, cap_handle_t bhandle,
    cap_handle_t ehandle, synch_timeout_t timeout, ipc_buf_receive_info_t *info)
{
	return __SYSCALL6(SYS_IPC2_BUF_RECEIVE, (sysarg_t) dst, (sysarg_t) size,
	    cap_handle_raw(bhandle), cap_handle_raw(ehandle),
	    (sysarg_t) timeout, (sysarg_t) info);
}

/** @copydoc sys_ipc2_buf_finish() */
errno_t ipc2_buf_finish(const void *src, size_t size,
    cap_ipc_buf_handle_t bhandle)
{
	return __SYSCALL3(SYS_IPC2_BUF_FINISH, (sysarg_t) src, (sysarg_t) size,
	    cap_handle_raw(bhandle));
}

/** @copydoc sys_ipc2_buf_wait() */
errno_t
ipc2_buf_wait(void *dst, size_t size, cap_handle_t bhandle,
    synch_timeout_t timeout, bool delist, ipc_buf_wait_info_t *info)
{
	return __SYSCALL6(SYS_IPC2_BUF_WAIT, (sysarg_t) dst, (sysarg_t) size,
	    cap_handle_raw(bhandle), (sysarg_t) timeout, (sysarg_t) delist,
	    (sysarg_t) info);
}

/** @}
 */
