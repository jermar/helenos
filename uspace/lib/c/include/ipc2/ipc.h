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

/** @addtogroup libcipc2
 * @{
 */
/** @file
 */

#ifndef _LIBC_IPC2_H_
#define _LIBC_IPC2_H_

#include <abi/ipc2/ipc.h>
#include <abi/cap.h>
#include <abi/synch.h>

#include <stdbool.h>

extern errno_t ipc2_buf_alloc(size_t, void *, cap_ipc_buf_handle_t *);
extern errno_t ipc2_buf_free(cap_ipc_buf_handle_t);

extern errno_t ipc2_ep_create(cap_ipc_ep_handle_t *, void *,
    cap_caplist_handle_t);
extern errno_t ipc2_ep_destroy(cap_ipc_ep_handle_t);

extern errno_t ipc2_buf_send(const void *, size_t, cap_ipc_buf_handle_t,
    cap_ipc_ep_handle_t, cap_caplist_handle_t);
extern errno_t ipc2_buf_receive(void *, size_t, cap_handle_t, cap_handle_t,
    synch_timeout_t, ipc_buf_receive_info_t *);
extern errno_t ipc2_buf_finish(const void *, size_t, cap_ipc_buf_handle_t);
extern errno_t ipc2_buf_wait(void *, size_t, cap_handle_t, synch_timeout_t,
    bool, ipc_buf_wait_info_t *);

#endif

/** @}
 */
