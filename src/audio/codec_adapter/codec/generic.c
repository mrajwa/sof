/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2020 Intel Corporation. All rights reserved.
 *
 *
 * \file generic.c
 * \brief Generic Codec API
 * \author Marcin Rajwa <marcin.rajwa@linux.intel.com>
 *
 */

#include <sof/audio/codec_adapter/codec/generic.h>

static struct codec_data codec_data;

static int validate_config(struct hifi_codec_config *cfg)
{
	//TODO:
	return 0;
}

int codec_load_config(struct comp_dev *dev, void *cfg, size_t size,
		       enum codec_cfg_type type)
{
	int ret;
	struct codec_config *dst;

	comp_dbg(dev, "codec_load_config() start");

	dst = (type == CODEC_CFG_SETUP) ? &codec_data.s_cfg :
				       &codec_data.r_cfg;

	if (!cfg) {
		comp_err(dev, "codec_load_config() error: NULL config passed!");
		ret = -EINVAL;
		goto err;
	}

	dst->data = rballoc(0, SOF_MEM_CAPS_RAM, size);

	if (!dst->data) {
		comp_err(dev, "codec_load_config() error: failed to allocate space for setup config.");
		ret = -ENOMEM;
		goto err;
	}

	ret = memcpy_s(dst->data, size, cfg, size);
	assert(!ret);

	ret = validate_config(dst->data);
	if (ret) {
		comp_err(dev, "codec_load_config() error: validation of config failed!");
		ret = -EINVAL;
		goto err;
	}

	/* Config loaded, mark it as valid */
	dst->size = size;
	dst->avail = true;

	return ret;
err:
	if (dst->data)
		rfree(dst->data);
	dst->data = NULL;
	return ret;
}
