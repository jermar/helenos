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

#include <cap/cap.h>
#include <cap/caplist.h>
#include <abi/cap.h>
#include <ipc2/ipc.h>
#include <errno.h>
#include <assert.h>

#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(caplist);

static struct {
	cap_caplist_handle_t clhandle;
	cap_ipc_buf_handle_t bhandle;
	bool test_after;
} test_data;

static cap_handle_t create_kobject(void)
{
	errno_t rc = ipc2_buf_alloc(0, 0, &test_data.bhandle);
	assert(rc == EOK);
	return test_data.bhandle;
}

PCUT_TEST_BEFORE
{
	PCUT_ASSERT_ERRNO_VAL(EOK, caplist_create(&test_data.clhandle,
	    KOBJECT_TYPE_IPC_BUF));
	/* Successful create must not leave the output handle unset. */
	PCUT_ASSERT_TRUE(cap_handle_valid(test_data.clhandle));

	test_data.bhandle = CAP_NIL;
	test_data.test_after = true;
}

PCUT_TEST_AFTER
{
	if (test_data.test_after) {
		PCUT_ASSERT_ERRNO_VAL(EOK, caplist_destroy(test_data.clhandle));
		/* Caplist can be destroyed only once. */
		PCUT_ASSERT_ERRNO_VAL(ENOENT,
		    caplist_destroy(test_data.clhandle));
	} else {
		(void) caplist_destroy(test_data.clhandle);
		(void) caplist_destroy(test_data.clhandle);
	}

	if (cap_handle_valid(test_data.bhandle))
		(void) ipc2_buf_free(test_data.bhandle);
}

/** A capability list can be created. */
PCUT_TEST(caplist_create)
{
	test_data.test_after = false;
}

/** A capability list can be destroyed. */
PCUT_TEST(caplist_destroy)
{
	/* nothing to do */
}

static void add(cap_handle_t handle)
{
	PCUT_ASSERT_ERRNO_VAL(EOK, caplist_add(test_data.clhandle, handle));
}

static void del(cap_handle_t handle)
{
	PCUT_ASSERT_ERRNO_VAL(EOK, caplist_del(test_data.clhandle, handle));
}

/** A kernel object can be added to a capability list. */
PCUT_TEST(caplist_add)
{
	cap_handle_t handle = create_kobject();
	add(handle);
}

/** Second insertion of an object to a capability list fails. */
PCUT_TEST(caplist_add_twice)
{
	cap_handle_t handle = create_kobject();
	add(handle);
	PCUT_ASSERT_ERRNO_VAL(EBUSY, caplist_add(test_data.clhandle, handle));
}

/** Cannot add to a kernel object which is not a capability list. */
PCUT_TEST(caplist_add_non_list)
{
	cap_handle_t handle = create_kobject();
	PCUT_ASSERT_ERRNO_VAL(ENOENT, caplist_add(handle, handle));
}

/** Cannot add a non-existant object to a capability list. */
PCUT_TEST(caplist_add_non_object)
{
	PCUT_ASSERT_ERRNO_VAL(ENOENT, caplist_add(test_data.clhandle, CAP_NIL));
}

/** Cannot add an object of a wrong type into a capability list. */
PCUT_TEST(caplist_add_bad_type)
{
	PCUT_ASSERT_ERRNO_VAL(ENOENT, caplist_add(test_data.clhandle,
	    test_data.clhandle));
}

/** A kernel object can be removed from a capability list. */
PCUT_TEST(caplist_del)
{
	cap_handle_t handle = create_kobject();
	add(handle);
	del(handle);
}

/** Second removal of an object from a capability list fails. */
PCUT_TEST(caplist_del_twice)
{
	cap_handle_t handle = create_kobject();
	add(handle);
	del(handle);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, caplist_del(test_data.clhandle, handle));
}

/** Cannot remove from an object which is not a capability list. */
PCUT_TEST(caplist_del_non_list)
{
	cap_handle_t handle = create_kobject();
	add(handle);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, caplist_del(handle, handle));
}

/** Cannot remove a non-existant object from a capability list. */
PCUT_TEST(caplist_del_non_object)
{
	cap_handle_t handle = create_kobject();
	add(handle);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, caplist_del(test_data.clhandle, CAP_NIL));
}

/** Cannot remove an object of a wrong type from a capability list. */
PCUT_TEST(caplist_del_bad_type)
{
	cap_handle_t handle = create_kobject();
	add(handle);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, caplist_del(test_data.clhandle,
	    test_data.clhandle));
}

/** Removal of an object that is not inside of the capability list fails. */
PCUT_TEST(caplist_del_non_member)
{
	cap_handle_t handle = create_kobject();
	PCUT_ASSERT_ERRNO_VAL(ENOENT, caplist_del(test_data.clhandle, handle));
}

PCUT_EXPORT(caplist);
