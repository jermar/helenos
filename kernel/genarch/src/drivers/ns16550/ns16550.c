/*
 * Copyright (c) 2009 Jakub Jermar
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

/** @addtogroup genarch
 * @{
 */
/**
 * @file
 * @brief NS 16550 serial controller driver.
 */

#include <genarch/drivers/ns16550/ns16550.h>
#include <ddi/irq.h>
#include <arch/asm.h>
#include <console/chardev.h>
#include <mm/slab.h>

#define LSR_DATA_READY  0x01

indev_operations_t kbrdin_ops = {
	.poll = NULL
};

static irq_ownership_t ns16550_claim(irq_t *irq)
{
	ns16550_instance_t *instance = irq->instance;
	ns16550_t *dev = instance->ns16550;
	
	if (pio_read_8(&dev->lsr) & LSR_DATA_READY)
		return IRQ_ACCEPT;
	else
		return IRQ_DECLINE;
}

static void ns16550_irq_handler(irq_t *irq)
{
	ns16550_instance_t *instance = irq->instance;
	ns16550_t *dev = instance->ns16550;
	
	if (pio_read_8(&dev->lsr) & LSR_DATA_READY) {
		uint8_t x = pio_read_8(&dev->rbr);
		chardev_push_character(&instance->kbrdin, x);
	}
}

/** Initialize ns16550.
 *
 * @param dev      Addrress of the beginning of the device in I/O space.
 * @param devno    Device number.
 * @param inr      Interrupt number.
 * @param cir      Clear interrupt function.
 * @param cir_arg  First argument to cir.
 *
 * @return Keyboard device pointer or NULL on failure.
 *
 */
indev_t *ns16550_init(ns16550_t *dev, devno_t devno, inr_t inr, cir_t cir, void *cir_arg)
{
	ns16550_instance_t *instance
	    = malloc(sizeof(ns16550_instance_t), FRAME_ATOMIC);
	if (!instance)
		return NULL;
	
	indev_initialize("ns16550", &instance->kbrdin, &kbrdin_ops);
	
	instance->devno = devno;
	instance->ns16550 = dev;
	
	irq_initialize(&instance->irq);
	instance->irq.devno = devno;
	instance->irq.inr = inr;
	instance->irq.claim = ns16550_claim;
	instance->irq.handler = ns16550_irq_handler;
	instance->irq.instance = instance;
	instance->irq.cir = cir;
	instance->irq.cir_arg = cir_arg;
	irq_register(&instance->irq);
	
	while ((pio_read_8(&dev->lsr) & LSR_DATA_READY))
		(void) pio_read_8(&dev->rbr);
	
	/* Enable interrupts */
	pio_write_8(&dev->ier, IER_ERBFI);
	pio_write_8(&dev->mcr, MCR_OUT2);
	
	return &instance->kbrdin;
}

/** @}
 */
