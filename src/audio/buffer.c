// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2016 Intel Corporation. All rights reserved.
//
// Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
//         Keyon Jie <yang.jie@linux.intel.com>

#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <sof/sof.h>
#include <sof/lock.h>
#include <sof/list.h>
#include <sof/stream.h>
#include <sof/alloc.h>
#include <sof/debug.h>
#include <sof/ipc.h>
#include <platform/timer.h>
#include <platform/platform.h>
#include <sof/audio/component.h>
#include <sof/audio/pipeline.h>
#include <sof/audio/buffer.h>

/* create a new component in the pipeline */
struct comp_buffer *buffer_new(struct sof_ipc_buffer *desc)
{
	struct comp_buffer *buffer;

	trace_buffer("buffer_new()");

	/* validate request */
	if (desc->size == 0 || desc->size > HEAP_BUFFER_SIZE) {
		trace_buffer_error("buffer_new() error: "
				   "new size = %u is invalid", desc->size);
		return NULL;
	}

	/* allocate new buffer */
	buffer = rzalloc(RZONE_RUNTIME, SOF_MEM_CAPS_RAM, sizeof(*buffer));
	if (!buffer) {
		trace_buffer_error("buffer_new() error: "
				   "could not alloc structure");
		return NULL;
	}

	buffer->addr = rballoc(RZONE_BUFFER, desc->caps, desc->size);
	if (!buffer->addr) {
		rfree(buffer);
		trace_buffer_error("buffer_new() error: "
				   "could not alloc size = %u "
				   "bytes of type = %u",
				   desc->size, desc->caps);
		return NULL;
	}

	assert(!memcpy_s(&buffer->ipc_buffer, sizeof(buffer->ipc_buffer),
		       desc, sizeof(*desc)));

	buffer->id = 0;
	buffer->size = desc->size;
	buffer->alloc_size = desc->size;
	buffer->w_ptr = buffer->addr;
	buffer->r_ptr = buffer->addr;
	buffer->end_addr = buffer->addr + buffer->ipc_buffer.size;
	buffer->free = buffer->ipc_buffer.size;
	buffer->avail = 0;

	buffer_zero(buffer);

	spinlock_init(&buffer->lock);

	return buffer;
}

/* free component in the pipeline */
void buffer_free(struct comp_buffer *buffer)
{
	trace_buffer("buffer_free()");

	list_item_del(&buffer->source_list);
	list_item_del(&buffer->sink_list);
	rfree(buffer->addr);
	rfree(buffer);
}

void comp_update_buffer_produce(struct comp_buffer *buffer, uint32_t bytes)
{
	uint32_t flags;
	uint32_t head = bytes;
	uint32_t tail = 0;
	static size_t produced_total;

	spin_lock_irq(&buffer->lock, flags);

	if (bytes == 0xFEED) {
		bytes = (buffer->last_produce - buffer->last_consume);
		trace_buffer("RAJWA: buffer we try to retransmit %d last consumed %d",
			bytes, buffer->last_consume);
		buffer->r_ptr = buffer->last_r_ptr;
		goto retransmit;
	}

	if (buffer->id == 99) {
		produced_total += bytes;
		trace_buffer("RAJWA: buffer PRODUCE of %d bytes in total %d", bytes,
			      produced_total);
	}
	/* return if no bytes */
	if (!bytes) {
		trace_buffer("comp_update_buffer_produce(), "
			     "no bytes to produce");
		return;
	}

	/* calculate head and tail size for dcache circular wrap ops */
	if (buffer->w_ptr + bytes > buffer->end_addr) {
		head = buffer->end_addr - buffer->w_ptr;
		tail = bytes - head;
	}

	/*
	 * new data produce, handle consistency for buffer and cache:
	 * 1. source(DMA) --> buffer --> sink(non-DMA): invalidate cache.
	 * 2. source(non-DMA) --> buffer --> sink(DMA): write back to memory.
	 * 3. source(DMA) --> buffer --> sink(DMA): do nothing.
	 * 4. source(non-DMA) --> buffer --> sink(non-DMA): do nothing.
	 */
	if (buffer->source->is_dma_connected &&
	    !buffer->sink->is_dma_connected) {
		/* need invalidate cache for sink component to use */
		dcache_invalidate_region(buffer->w_ptr, head);
		if (tail)
			dcache_invalidate_region(buffer->addr, tail);
	} else if (!buffer->source->is_dma_connected &&
		   buffer->sink->is_dma_connected) {
		/* need write back to memory for sink component to use */
		dcache_writeback_region(buffer->w_ptr, head);
		if (tail)
			dcache_writeback_region(buffer->addr, tail);
	}

	buffer->w_ptr += bytes;

	/* check for pointer wrap */
	if (buffer->w_ptr >= buffer->end_addr)
		buffer->w_ptr = buffer->addr +
			(buffer->w_ptr - buffer->end_addr);

	/* calculate available bytes */
	if (buffer->r_ptr < buffer->w_ptr)
		buffer->avail = buffer->w_ptr - buffer->r_ptr;
	else if (buffer->r_ptr == buffer->w_ptr)
		buffer->avail = buffer->size; /* full */
	else
		buffer->avail = buffer->size - (buffer->r_ptr - buffer->w_ptr);

	/* calculate free bytes */
	buffer->free = buffer->size - buffer->avail;

retransmit:
	buffer->last_produce = bytes;
	buffer->transfer_done = false;
	if (buffer->cb && buffer->cb_type & BUFF_CB_TYPE_PRODUCE)
		buffer->cb(buffer->cb_data, &buffer->last_produce);

	spin_unlock_irq(&buffer->lock, flags);

	tracev_buffer("comp_update_buffer_produce(), ((buffer->avail << 16) | "
		      "buffer->free) = %08x, ((buffer->ipc_buffer.comp.id << "
		      "16) | buffer->size) = %08x",
		      (buffer->avail << 16) | buffer->free,
		      (buffer->ipc_buffer.comp.id << 16) | buffer->size);
	tracev_buffer("comp_update_buffer_produce(), ((buffer->r_ptr - buffer"
		      "->addr) << 16 | (buffer->w_ptr - buffer->addr)) = %08x",
		      (buffer->r_ptr - buffer->addr) << 16 |
		      (buffer->w_ptr - buffer->addr));
}

void comp_update_buffer_consume(struct comp_buffer *buffer, uint32_t bytes)
{
	uint32_t flags;
	static size_t consumed_total;
	buffer->last_consume = bytes;

	if (buffer->id == 99 && (bytes != buffer->last_produce)) {
		trace_buffer_error("RAJWA: transmition failed! produced %d but consumed %d",
			buffer->last_produce, bytes);
		//return;
	}
	if (buffer->id == 99) {
		consumed_total += bytes;
		trace_buffer("RAJWA: buffer CONSUME of %d bytes in total %d", bytes,
			      consumed_total);
	}
	/* return if no bytes */
	if (!bytes) {
		trace_buffer("comp_update_buffer_consume(), "
			     "no bytes to consume");
		buffer->last_r_ptr = buffer->r_ptr;
		buffer->transfer_done = true;
		return;
	}

	spin_lock_irq(&buffer->lock, flags);

	buffer->r_ptr += bytes;

	/* check for pointer wrap */
	if (buffer->r_ptr >= buffer->end_addr)
		buffer->r_ptr = buffer->addr +
			(buffer->r_ptr - buffer->end_addr);

	/* calculate available bytes */
	if (buffer->r_ptr < buffer->w_ptr)
		buffer->avail = buffer->w_ptr - buffer->r_ptr;
	else if (buffer->r_ptr == buffer->w_ptr)
		buffer->avail = 0; /* empty */
	else
		buffer->avail = buffer->size - (buffer->r_ptr - buffer->w_ptr);

	/* calculate free bytes */
	buffer->free = buffer->size - buffer->avail;

	if (buffer->sink->is_dma_connected &&
	    !buffer->source->is_dma_connected)
		dcache_writeback_region(buffer->r_ptr, bytes);

	if (buffer->cb && buffer->cb_type & BUFF_CB_TYPE_CONSUME)
		buffer->cb(buffer->cb_data, &bytes);

	buffer->transfer_done = true;
	buffer->last_r_ptr = buffer->r_ptr;

	spin_unlock_irq(&buffer->lock, flags);

	tracev_buffer("comp_update_buffer_consume(), "
		      "(buffer->avail << 16) | buffer->free = %08x, "
		      "(buffer->ipc_buffer.comp.id << 16) | buffer->size = "
		      " %08x, (buffer->r_ptr - buffer->addr) << 16 | "
		      "(buffer->w_ptr - buffer->addr)) = %08x",
		      (buffer->avail << 16) | buffer->free,
		      (buffer->ipc_buffer.comp.id << 16) | buffer->size,
		      (buffer->r_ptr - buffer->addr) << 16 |
		      (buffer->w_ptr - buffer->addr));
}
