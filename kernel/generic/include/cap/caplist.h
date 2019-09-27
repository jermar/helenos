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

#ifndef KERN_CAPLIST_H_
#define KERN_CAPLIST_H_

#include <abi/cap.h>
#include <cap/cap.h>
#include <typedefs.h>
#include <adt/list.h>
#include <synch/mutex.h>
#include <synch/condvar.h>

typedef struct caplist {
	/**
	 * Immutable type of the caplist. All listed kernel objects are required
	 * to be of this type.
	 */
	kobject_type_t type;

	mutex_t mutex;
	/** Member kernel objects. */
	list_t objects;

	list_t queue;
	condvar_t cv;
} caplist_t;

extern kobject_ops_t caplist_kobject_ops;

extern void caplist_init(void);

extern errno_t caplist_add(caplist_t *, kobject_t *);
extern errno_t caplist_del(caplist_t *, kobject_t *);

extern sys_errno_t sys_caplist_create(uspace_ptr_cap_caplist_handle_t,
    kobject_type_t);
extern sys_errno_t sys_caplist_destroy(cap_caplist_handle_t);
extern sys_errno_t sys_caplist_add(cap_caplist_handle_t, cap_handle_t);
extern sys_errno_t sys_caplist_del(cap_caplist_handle_t, cap_handle_t);

#endif

/** @}
 */
