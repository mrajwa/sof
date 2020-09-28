// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2020 Intel Corporation. All rights reserved.
//
// Author: Marcin Rajwa <marcin.rajwa@linux.intel.com>

/*
 * \file generic.c
 * \brief Generic Codec API
 * \author Marcin Rajwa <marcin.rajwa@linux.intel.com>
 *
 */

#include <sof/audio/codec_adapter/codec/generic.h>
#include <sof/audio/codec_adapter/interfaces.h>

/*****************************************************************************/
/* Local helper functions						     */
/*****************************************************************************/
static int validate_config(struct codec_config *cfg);

int
codec_load_config(struct comp_dev *dev, void *cfg, size_t size,
		  enum codec_cfg_type type)
{
	int ret;
	struct codec_config *dst;
	struct comp_data *cd = comp_get_drvdata(dev);
	struct codec_data *codec = &cd->codec;

	comp_dbg(dev, "codec_load_config() start");

	if (!dev || !cfg || !size) {
		comp_err(dev, "codec_load_config(): wrong input params! dev %x, cfg %x size %d",
			 (uint32_t)dev, (uint32_t)cfg, size);
		return -EINVAL;
	}

	dst = (type == CODEC_CFG_SETUP) ? &codec->s_cfg :
					  &codec->r_cfg;

	if (!dst->data) {
		dst->data = rballoc(0, SOF_MEM_CAPS_RAM, size);
	} else if (dst->size != size) {
		rfree(dst->data);
		dst->data = rballoc(0, SOF_MEM_CAPS_RAM, size);
	}
	if (!dst->data) {
		comp_err(dev, "codec_load_config(): failed to allocate space for setup config.");
		ret = -ENOMEM;
		goto err;
	}

	ret = memcpy_s(dst->data, size, cfg, size);
	assert(!ret);
	ret = validate_config(dst->data);
	if (ret) {
		comp_err(dev, "codec_load_config(): validation of config failed!");
		ret = -EINVAL;
		goto err;
	}

	/* Config loaded, mark it as valid */
	dst->size = size;
	dst->avail = true;

	comp_dbg(dev, "codec_load_config() done");
	return ret;
err:
	if (dst->data && type == CODEC_CFG_RUNTIME)
		rfree(dst->data);
	dst->data = NULL;
	return ret;
}

int codec_init(struct comp_dev *dev)
{
	int ret;
	struct comp_data *cd = comp_get_drvdata(dev);
	uint32_t codec_id = cd->ca_config.codec_id;
	struct codec_data *codec = &cd->codec;
	struct codec_interface *interface = NULL;
	uint32_t i;
	uint32_t no_of_interfaces = sizeof(interfaces) /
				    sizeof(struct codec_interface);

	comp_info(dev, "codec_init() start");

	if (cd->codec.state == CODEC_INITIALIZED)
		return 0;
	if (cd->codec.state > CODEC_INITIALIZED)
		return -EPERM;

	/* Find proper interface */
	for (i = 0; i < no_of_interfaces; i++) {
		if (interfaces[i].id == codec_id) {
			interface = &interfaces[i];
			break;
		}
	}
	if (!interface) {
		comp_err(dev, "codec_init(): could not find codec interface for codec id %x",
			 codec_id);
		ret = -EIO;
		goto out;
	} else if (!interface->init) {//TODO: verify other interfaces
		comp_err(dev, "codec_init(): codec %x is missing required interfaces",
			 codec_id);
		ret = -EIO;
		goto out;
	}

	comp_info(dev, "codec_init() done");
	codec->state = CODEC_INITIALIZED;
out:
	return ret;
}

static int validate_config(struct codec_config *cfg)
{
	//TODO: validation of codec specifig setup config
	return 0;
}
