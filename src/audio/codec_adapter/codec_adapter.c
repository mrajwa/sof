// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2020 Intel Corporation. All rights reserved.
//
// Author: Marcin Rajwa <marcin.rajwa@linux.intel.com>

/*
 * A codec adapter component.
 */

/**
 * \file audio/codec_adapter.c
 * \brief Processing compoent aimed to work with external codec libraries
 * \author Marcin Rajwa <marcin.rajwa@linux.intel.com>
 */

#include <sof/audio/buffer.h>
#include <sof/audio/component.h>
#include <sof/audio/pipeline.h>
#include <sof/audio/codec_adapter/codec_adapter.h>
#include <sof/common.h>
#include <sof/debug/panic.h>
#include <sof/drivers/ipc.h>
#include <sof/lib/alloc.h>
#include <sof/lib/clk.h>
#include <sof/lib/memory.h>
#include <sof/list.h>
#include <sof/platform.h>
#include <sof/string.h>
#include <sof/ut.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <user/trace.h>

static const struct comp_driver comp_codec_adapter;

/* d8218443-5ff3-4a4c-b388-6cfe07b956aa */
DECLARE_SOF_RT_UUID("codec_adapter", ca_uuid, 0xd8218443, 0x5ff3, 0x4a4c,
		    0xb3, 0x88, 0x6c, 0xfe, 0x07, 0xb9, 0x56, 0xaa);

DECLARE_TR_CTX(ca_tr, SOF_UUID(ca_uuid), LOG_LEVEL_INFO);

static struct comp_dev *codec_adapter_new(const struct comp_driver *drv,
					      struct sof_ipc_comp *comp)
{
	int ret;
	struct comp_dev *dev = NULL;
	struct comp_data *cd = NULL;
	struct sof_ipc_comp_process *ipc_codec_adapter =
		(struct sof_ipc_comp_process *)comp;
	struct ca_config *cfg;
	size_t bs;
	void *lib_cfg;
	size_t lib_cfg_size;

	comp_cl_info(&comp_codec_adapter, "codec_adapter_new()");

	if (!drv || !comp) {
		comp_cl_err(&comp_codec_adapter, "codec_adapter_new(), wrong input params! drv = %p comp = %p",
			    drv, comp);
		goto err;
	}

	dev = comp_alloc(drv, COMP_SIZE(struct sof_ipc_comp_process));
	if (!dev) {
		comp_cl_err(&comp_codec_adapter, "codec_adapter_new(), failed to allocate memory for comp_dev");
		goto err;
	}

	dev->drv = drv;

	ret = memcpy_s(&dev->comp, sizeof(struct sof_ipc_comp_process),
		       comp, sizeof(struct sof_ipc_comp_process));
	assert(!ret);

	cd = rzalloc(SOF_MEM_ZONE_RUNTIME, 0, SOF_MEM_CAPS_RAM, sizeof(*cd));
	if (!cd) {
		comp_cl_err(&comp_codec_adapter, "codec_adapter_new(), failed to allocate memory for comp_data");
		goto err;
	}

	comp_set_drvdata(dev, cd);

	/* Copy setup config */
	cfg = (struct ca_config *)ipc_codec_adapter->data;
	bs = ipc_codec_adapter->size;
	if (bs) {
		if (bs < sizeof(struct ca_config)) {
			comp_info(dev, "generic_processor_new() error: wrong size of setup config");
			goto err;
		}
		ret = memcpy_s(&cd->ca_config, sizeof(cd->ca_config), cfg,
			       sizeof(struct ca_config));
		assert(!ret);
		ret = validate_setup_config(&cd->gp_config);
		if (ret) {
			comp_err(dev, "codec_adapter_new(): error: validation of setup config failed");
			goto err;
		}

		/* Pass config further to the codec */
		lib_cfg = (char *)cfg + sizeof(struct ca_config);
		lib_cfg_size = bs - sizeof(struct ca_config);
		ret = codec_load_config(dev, lib_cfg, lib_cfg_size, CODEC_CFG_SETUP);
		if (ret) {
			comp_err(dev, "codec_adapter_new(): error %x: failed to load setup config for codec",
				 ret);
		} else {
			comp_dbg(dev, "codec_adapter_new() codec config loaded successfully");
		}
	} else {
		comp_err(dev, "generic_processor_new(): no configuration available");
		goto err;
	}

	dev->state = COMP_STATE_READY;
	cd->state = PP_STATE_CREATED;

	return dev;
err:
	if (cd)
		rfree(cd);
	if (dev)
		rfree(dev);
	return NULL;
}

static const struct comp_driver comp_codec_adapter = {
	.type = SOF_COMP_GENERIC_PROCESSOR,
	.uid = SOF_RT_UUID(ca_uuid),
	.tctx = &ca_tr,
	.ops = {
		.create = codec_adapter_new,
	},
};
