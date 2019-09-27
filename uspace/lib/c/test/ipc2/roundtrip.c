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
#include <cap/caplist.h>
#include <abi/cap.h>
#include <abi/synch.h>
#include <errno.h>
#include <assert.h>

#include <pcut/pcut.h>

#include <mem.h>

PCUT_INIT;

PCUT_TEST_SUITE(ipc2_roundtrip);

#define TEST_BUF_SMALL_SIZE		16
#define TEST_BUF_LABEL		((void *) 0x1abe1b)
#define TEST_EP_LABEL		((void *) 0x1abe1e)

static struct {
	cap_ipc_buf_handle_t bhandle;
	cap_ipc_ep_handle_t ehandle;
	cap_handle_t handle;
	cap_caplist_handle_t buf_clhandle;
	cap_caplist_handle_t ep_clhandle;
	bool test_after;
	char send_buf[TEST_BUF_SMALL_SIZE];
	char recv_buf[TEST_BUF_SMALL_SIZE];
	char fini_buf[TEST_BUF_SMALL_SIZE];
	char wait_buf[TEST_BUF_SMALL_SIZE];
	ipc_buf_receive_info_t recv_info;
	ipc_buf_wait_info_t wait_info;
} test_data;

PCUT_TEST_BEFORE
{
	test_data.test_after = false;

	PCUT_ASSERT_ERRNO_VAL(EOK, ipc2_buf_alloc(TEST_BUF_SMALL_SIZE,
	    TEST_BUF_LABEL, &test_data.bhandle));
	PCUT_ASSERT_TRUE(cap_handle_valid(test_data.bhandle));
	PCUT_ASSERT_ERRNO_VAL(EOK, ipc2_ep_create(&test_data.ehandle,
	    TEST_EP_LABEL, CAP_NIL));
	PCUT_ASSERT_TRUE(cap_handle_valid(test_data.ehandle));
	PCUT_ASSERT_ERRNO_VAL(EOK, cap_alloc(&test_data.handle));
	PCUT_ASSERT_TRUE(cap_handle_valid(test_data.handle));

	test_data.buf_clhandle = CAP_NIL;
	test_data.ep_clhandle = CAP_NIL;

	const char hello[] = "Hello world!";
	memset(test_data.send_buf, 0, sizeof(test_data.send_buf));
	memcpy(test_data.send_buf, hello, sizeof(hello));
	memset(test_data.recv_buf, 0, sizeof(test_data.recv_buf));
	const char bye[] = "Bye";
	memset(test_data.fini_buf, 0, sizeof(test_data.fini_buf));
	memcpy(test_data.fini_buf, bye, sizeof(bye));
	memset(test_data.wait_buf, 0, sizeof(test_data.wait_buf));
}

PCUT_TEST_AFTER
{
	if (test_data.test_after) {
		PCUT_ASSERT_ERRNO_VAL(EOK, cap_free(test_data.handle));
		PCUT_ASSERT_ERRNO_VAL(EOK, ipc2_ep_destroy(test_data.ehandle));
		PCUT_ASSERT_ERRNO_VAL(EOK, ipc2_buf_free(test_data.bhandle));
	} else {
		(void) cap_free(test_data.handle);
		(void) ipc2_ep_destroy(test_data.ehandle);
		(void) ipc2_buf_free(test_data.bhandle);
	}

	if (cap_handle_valid(test_data.buf_clhandle))
		(void) caplist_destroy(test_data.buf_clhandle);
	if (cap_handle_valid(test_data.ep_clhandle))
		(void) caplist_destroy(test_data.ep_clhandle);
}


static void send_with_handle(cap_handle_t handle)
{
	PCUT_ASSERT_ERRNO_VAL(EOK, ipc2_buf_send(test_data.send_buf,
	    sizeof(test_data.send_buf), test_data.bhandle, test_data.ehandle,
	    handle));
}

static void send_with_cl(void)
{
	send_with_handle(test_data.buf_clhandle);
}

static void send(void)
{
	send_with_handle(CAP_NIL);
}

static void receive_from_handle(cap_handle_t handle)
{
	ipc_buf_receive_info_t *info = &test_data.recv_info;

	PCUT_ASSERT_ERRNO_VAL(EOK, ipc2_buf_receive(test_data.recv_buf,
	    sizeof(test_data.recv_buf), test_data.handle, handle, 0, info));

	PCUT_ASSERT_PTR_EQUALS(TEST_EP_LABEL, (void *) info->ep_label);
	PCUT_ASSERT_UINT_EQUALS(sizeof(test_data.send_buf), info->used);
	PCUT_ASSERT_UINT_EQUALS(TEST_BUF_SMALL_SIZE, info->size);
	PCUT_ASSERT_STR_EQUALS(test_data.send_buf, test_data.recv_buf);
}

static void receive(void)
{
	receive_from_handle(test_data.ehandle);
}

static void receive_from_cl(void)
{
	receive_from_handle(test_data.ep_clhandle);
}

static void finish(void)
{
	PCUT_ASSERT_ERRNO_VAL(EOK, ipc2_buf_finish(test_data.fini_buf,
	    sizeof(test_data.fini_buf), test_data.handle));
}

static void wait_handle(cap_handle_t handle, bool delist)
{
	ipc_buf_wait_info_t *info = &test_data.wait_info;

	PCUT_ASSERT_ERRNO_VAL(EOK, ipc2_buf_wait(test_data.wait_buf,
	    sizeof(test_data.wait_buf), handle, 0, delist, info));

	PCUT_ASSERT_PTR_EQUALS(TEST_BUF_LABEL, (void *) info->buf_label);
	PCUT_ASSERT_UINT_EQUALS(sizeof(test_data.fini_buf), info->used);
	PCUT_ASSERT_UINT_EQUALS(TEST_BUF_SMALL_SIZE, info->size);
	PCUT_ASSERT_STR_EQUALS(test_data.fini_buf, test_data.wait_buf);
}

static void wait(void)
{
	wait_handle(test_data.bhandle, false);
}

static void wait_on_cl(bool delist)
{
	wait_handle(test_data.buf_clhandle, delist);
}

/** IPC buffer round-trip can be made using plain IPC buffer handles. */
PCUT_TEST(single)
{
	send();
	receive();
	finish();
	wait();

	test_data.test_after = true;
}

/** IPC buffer round-trip can be repeated with the same buffer. */
PCUT_TEST(multiple)
{
	for (int i = 0; i < 2; i++) {
		send();
		receive();
		finish();
		wait();
	}

	test_data.test_after = true;
}

/** IPC buffer cannot be sent while pending. */
PCUT_TEST(send_while_pending)
{
	send();

	PCUT_ASSERT_ERRNO_VAL(EBUSY, ipc2_buf_send(test_data.send_buf,
	    sizeof(test_data.send_buf), test_data.bhandle, test_data.ehandle,
	    CAP_NIL));
}

/** IPC buffer cannot be sent while finished. */
PCUT_TEST(send_while_finished)
{
	send();
	receive();
	finish();

	PCUT_ASSERT_ERRNO_VAL(EBUSY, ipc2_buf_send(test_data.send_buf,
	    sizeof(test_data.send_buf), test_data.bhandle, test_data.ehandle,
	    CAP_NIL));
}

/** IPC buffer can be forwarded.
 *
 * The forwarding send unpublishes the used IPC buffer capability handle.
 */
PCUT_TEST(forwarding_send)
{
	send();
	receive();

	PCUT_ASSERT_ERRNO_VAL(EOK, ipc2_buf_send(test_data.send_buf,
	    sizeof(test_data.send_buf), test_data.handle, test_data.ehandle,
	    CAP_NIL));

	/*
	 * The second forward fails because the first one unpublished the
	 * capability handle.
	 */
	PCUT_ASSERT_ERRNO_VAL(ENOENT, ipc2_buf_send(test_data.send_buf,
	    sizeof(test_data.send_buf), test_data.handle, test_data.ehandle,
	    CAP_NIL));
}

/** IPC buffer can be finished just once.
 *
 * The finish unpublished the used IPC buffer capability handle.
 */
PCUT_TEST(double_finish)
{
	send();
	receive();
	finish();

	/*
	 * The second finish fails because the first one unpublished the
	 * capability handle.
	 */
	PCUT_ASSERT_ERRNO_VAL(ENOENT, ipc2_buf_finish(test_data.fini_buf,
	    sizeof(test_data.fini_buf), test_data.handle));
}

static void create_buf_caplist(void)
{
	errno_t rc = caplist_create(&test_data.buf_clhandle,
	    KOBJECT_TYPE_IPC_BUF);
	assert(rc == EOK);
}

/** IPC buffer can be put into a caplist on send. */
PCUT_TEST(inserting_send)
{
	create_buf_caplist();
	send_with_cl();
}

/**
 * IPC buffer cannot be put into a caplist on send if it is already in a
 * caplist.
 */
PCUT_TEST(inserting_send_already_in)
{
	create_buf_caplist();

	errno_t rc = caplist_add(test_data.buf_clhandle, test_data.bhandle);
	assert(rc == EOK);

	/*
	 * The send with insertion fails because the buffer is already a member
	 * of the caplist.
	 */
	PCUT_ASSERT_ERRNO_VAL(EBUSY, ipc2_buf_send(test_data.send_buf,
	    sizeof(test_data.send_buf), test_data.bhandle, test_data.ehandle,
	    test_data.buf_clhandle));
}

/**
 * A non-inserting send of a buffer which is already in a caplist has the same
 * effect as an inserting send of a buffer which is not in any caplist.
 */
PCUT_TEST(non_inserting_roundtrip_member)
{
	create_buf_caplist();

	errno_t rc = caplist_add(test_data.buf_clhandle, test_data.bhandle);
	assert(rc == EOK);

	send();
	receive();
	finish();
	wait_on_cl(true);
}

static void inserting_3_4_roundtrip(void)
{
	create_buf_caplist();
	send_with_cl();
	receive();
	finish();
}

/**
 * A delisting wait can be used for waiting on an IPC buffer which is a member
 * of a caplist.  After the wait, the buffer can be sent with insertion into a
 * caplist again.
 */
PCUT_TEST(wait_on_cl_w_delist)
{
	inserting_3_4_roundtrip();
	wait_on_cl(true);

	/*
	 * The second send with insertion into caplist succeeds because the
	 * buffer was delisted.
	 */
	send_with_cl();
}

/**
 * A non-delisting wait can be used for waiting on an IPC buffer which is a
 * member of a caplist. After the wait, the buffer cannot be sent with
 * re-insertion into a caplist.
 */
PCUT_TEST(wait_on_cl_wo_delist)
{
	inserting_3_4_roundtrip();
	wait_on_cl(false);

	/*
	 * The second send with insertion into caplist fails because the buffer
	 * is still in the caplist.
	 */
	PCUT_ASSERT_ERRNO_VAL(EBUSY, ipc2_buf_send(test_data.send_buf,
	    sizeof(test_data.send_buf), test_data.bhandle, test_data.ehandle,
	    test_data.buf_clhandle));
}

static void create_ep_caplist(void)
{
	errno_t rc = caplist_create(&test_data.ep_clhandle,
	    KOBJECT_TYPE_IPC_EP);
	assert(rc == EOK);
}

/** An IPC buffer can be received from a caplist. */
PCUT_TEST(receive_from_cl)
{
	create_ep_caplist();
	errno_t rc = caplist_add(test_data.ep_clhandle, test_data.ehandle);
	assert(rc == EOK);
	send();
	receive_from_cl();
}

/**
 * An IPC buffer cannot be received from an endpoint if the endpoint to which
 * the buffer was sent is a member of a caplist.
 */
PCUT_TEST(receive_from_ep_member)
{
	create_ep_caplist();
	errno_t rc = caplist_add(test_data.ep_clhandle, test_data.ehandle);
	assert(rc == EOK);
	send();

	PCUT_ASSERT_ERRNO_VAL(EAGAIN, ipc2_buf_receive(test_data.recv_buf,
	    sizeof(test_data.recv_buf), test_data.handle, test_data.ehandle,
	    synch_timeout(0, SYNCH_FLAGS_NON_BLOCKING), &test_data.recv_info));
}

/**
 * An IPC buffer cannot be received from a caplist if the endpoint to which the
 * buffer was sent is not a member of the caplist.
 */
PCUT_TEST(receive_from_cl_non_member)
{
	create_ep_caplist();
	send();

	PCUT_ASSERT_ERRNO_VAL(EAGAIN, ipc2_buf_receive(test_data.recv_buf,
	    sizeof(test_data.recv_buf), test_data.handle, test_data.ep_clhandle,
	    synch_timeout(0, SYNCH_FLAGS_NON_BLOCKING), &test_data.recv_info));
}

PCUT_EXPORT(ipc2_roundtrip);
