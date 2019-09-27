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

/** @addtogroup abi_generic
 * @{
 */
/** @file
 */

#ifndef _ABI_IPC2_IPC_H_
#define _ABI_IPC2_IPC_H_

#include <abi/cap.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <_bits/native.h>

/**
 * Info structure returned after receiving from an IPC endpoint.
 */
typedef struct ipc_buf_receive_info {
	/** Label of the IPC endpoint which received the IPC buffer. */
	uspace_addr_t ep_label;
	/** How much data is in the IPC buffer. */
	size_t used;
	/** Total size of the IPC buffer. */
	size_t size;
} ipc_buf_receive_info_t;

/**
 * Info structure returned after waiting on an IPC buffer handle.
 */
typedef struct ipc_buf_wait_info {
	/** Label of the IPC buffer which was successfully waited for. */
	uspace_addr_t buf_label;
	/** How much data is in the IPC buffer. */
	size_t used;
	/** Total size of the IPC buffer. */
	size_t size;
	/** Wait result */
	errno_t result;
} ipc_buf_wait_info_t;

#endif

/** @}
 */
