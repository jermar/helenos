/*
 * Copyright (C) 2003-2004 Jakub Jermar
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

#include <interrupt.h>
#include <arch/interrupt.h>
#include <arch/types.h>
#include <arch.h>
#include <arch/cp0.h>
#include <time/clock.h>
#include <arch/drivers/arc.h>

/** Disable interrupts.
 *
 * @return Old interrupt priority level.
 */
ipl_t interrupts_disable(void)
{
	ipl_t ipl = (ipl_t) cp0_status_read();
	cp0_status_write(ipl & ~cp0_status_ie_enabled_bit);
	return ipl;
}

/** Enable interrupts.
 *
 * @return Old interrupt priority level.
 */
ipl_t interrupts_enable(void)
{
	ipl_t ipl = (ipl_t) cp0_status_read();
	cp0_status_write(ipl | cp0_status_ie_enabled_bit);
	return ipl;
}

/** Restore interrupt priority level.
 *
 * @param ipl Saved interrupt priority level.
 */
void interrupts_restore(ipl_t ipl)
{
	cp0_status_write(cp0_status_read() | (ipl & cp0_status_ie_enabled_bit));
}

/** Read interrupt priority level.
 *
 * @return Current interrupt priority level.
 */
ipl_t interrupts_read(void)
{
	return cp0_status_read();
}

static void timer_exception(int n, istate_t *istate)
{
	cp0_compare_write(cp0_count_read() + cp0_compare_value); 
	clock();
}

static void swint0(int n, istate_t *istate)
{
	cp0_cause_write(cp0_cause_read() & ~(1 << 8)); /* clear SW0 interrupt */
}

static void swint1(int n, istate_t *istate)
{
	cp0_cause_write(cp0_cause_read() & ~(1 << 9)); /* clear SW1 interrupt */
}

/* Initialize basic tables for exception dispatching */
void interrupt_init(void)
{
	int_register(TIMER_IRQ, "timer", timer_exception);
	int_register(0, "swint0", swint0);
	int_register(1, "swint1", swint1);
}
