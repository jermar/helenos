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

#include <ipc2/ipc.h>
#include <cap/cap.h>
#include <abi/cap.h>
#include <errno.h>

#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(ipc2);

#define TEST_BUF_SMALL_SIZE		16
#define TEST_BUF_IMPOSSIBLE_SIZE	-1UL
#define TEST_BUF_LABEL		((void *) 0x1abe1b)
#define TEST_EP_LABEL		((void *) 0x1abe1e)

/** IPC buffers can be allocated and freed. */
PCUT_TEST(ipc_buf_alloc_free)
{
	cap_ipc_buf_handle_t bhandle = CAP_NIL;

	PCUT_ASSERT_ERRNO_VAL(EOK, ipc2_buf_alloc(TEST_BUF_SMALL_SIZE,
	    TEST_BUF_LABEL, &bhandle));
	PCUT_ASSERT_FALSE(bhandle == CAP_NIL);
	PCUT_ASSERT_ERRNO_VAL(EOK, ipc2_buf_free(bhandle));
	PCUT_ASSERT_ERRNO_VAL(ENOENT, ipc2_buf_free(bhandle));
}

/** Allocations of zero-sized buffers are possible. */
PCUT_TEST(ipc_buf_zero_size)
{
	cap_ipc_buf_handle_t bhandle = CAP_NIL;

	PCUT_ASSERT_ERRNO_VAL(EOK, ipc2_buf_alloc(0, TEST_BUF_LABEL, &bhandle));
	PCUT_ASSERT_FALSE(bhandle == CAP_NIL);
	PCUT_ASSERT_ERRNO_VAL(EOK, ipc2_buf_free(bhandle));
}

/** Allocations of impossibly large buffers fail. */
PCUT_TEST(ipc_buf_impossibly_large)
{
	cap_ipc_buf_handle_t bhandle = CAP_NIL;

	PCUT_ASSERT_ERRNO_VAL(ENOMEM, ipc2_buf_alloc(TEST_BUF_IMPOSSIBLE_SIZE,
	    TEST_BUF_LABEL, &bhandle));
}

/** IPC endpoints can be created and destroyed. */
PCUT_TEST(ipc_ep_create_destroy)
{
	cap_ipc_ep_handle_t ehandle = CAP_NIL;

	PCUT_ASSERT_ERRNO_VAL(EOK, ipc2_ep_create(&ehandle, TEST_EP_LABEL,
	    CAP_NIL));
	PCUT_ASSERT_FALSE(ehandle == CAP_NIL);
	PCUT_ASSERT_ERRNO_VAL(EOK, ipc2_ep_destroy(ehandle));
	PCUT_ASSERT_ERRNO_VAL(ENOENT, ipc2_ep_destroy(ehandle));
}

PCUT_EXPORT(ipc2);
