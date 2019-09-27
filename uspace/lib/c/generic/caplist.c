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

/** @file
 */

#include <cap/caplist.h>
#include <abi/cap.h>
#include <abi/syscall.h>
#include <libc.h>
#include <errno.h>

/** @copydoc sys_caplist_create */
errno_t caplist_create(cap_caplist_handle_t *clhandle, kobject_type_t type)
{
	return __SYSCALL2(SYS_CAPLIST_CREATE, cap_handle_raw(clhandle),
	    (sysarg_t) type);
}

/** @copydoc sys_caplist_destroy */
errno_t caplist_destroy(cap_caplist_handle_t clhandle)
{
	return __SYSCALL1(SYS_CAPLIST_DESTROY, cap_handle_raw(clhandle));
}

/** @copydoc sys_caplist_add */
errno_t caplist_add(cap_caplist_handle_t clhandle, cap_handle_t handle)
{
	return __SYSCALL2(SYS_CAPLIST_ADD, cap_handle_raw(clhandle),
	    cap_handle_raw(handle));
}

/** @copydoc sys_caplist_del */
errno_t caplist_del(cap_caplist_handle_t clhandle, cap_handle_t handle)
{
	return __SYSCALL2(SYS_CAPLIST_DEL, cap_handle_raw(clhandle),
	    cap_handle_raw(handle));
}

/** @}
 */
