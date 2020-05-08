/******************************************************************************
 * Copyright (c) 2019-2020, Texas Instruments Incorporated - http://www.ti.com/
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *       * Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 *       * Redistributions in binary form must reproduce the above copyright
 *         notice, this list of conditions and the following disclaimer in the
 *         documentation and/or other materials provided with the distribution.
 *       * Neither the name of Texas Instruments Incorporated nor the
 *         names of its contributors may be used to endorse or promote products
 *         derived from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *   THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/
 
#include "cmem_buf.h"
#include <stdio.h>
#include <unistd.h>
#include <ti/cmem.h>
#include <linux/dma-buf.h>
#include <sys/ioctl.h>
#include <common/error.h>

#define CMEM_BLOCKID CMEM_CMABLOCKID

CMEM_AllocParams cmem_alloc_params = {
	CMEM_HEAP,	/* type */
	CMEM_CACHED,	/* flags */
	1		/* alignment */
};

void init_cmem()
{
	CMEM_init();
}

int alloc_cmem_buffer(unsigned int size, unsigned int align, void **cmem_buf)
{
	cmem_alloc_params.alignment = align;

	*cmem_buf = CMEM_alloc2(CMEM_BLOCKID, size,
		&cmem_alloc_params);

	if(*cmem_buf == NULL){
		DBG("CMEM allocation failed");
		return -1;
	}
	DBG("CMEM buffer pointer is 0x%x\n",(uint32_t) *cmem_buf );
	return CMEM_export_dmabuf(*cmem_buf);
}

void free_cmem_buffer(void *cmem_buffer)
{
	CMEM_free(cmem_buffer, &cmem_alloc_params);
}

int dma_buf_do_cache_operation(int dma_buf_fd, uint32_t cache_operation)
{
	int ret;
	struct dma_buf_sync sync;
	sync.flags = cache_operation;

	ret = ioctl(dma_buf_fd, DMA_BUF_IOCTL_SYNC, &sync);

	return ret;
}
