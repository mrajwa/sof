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
#include <sof/audio/codec_adapter/codec/interfaces.h>

static int validate_config(struct codec_config *cfg)
{
	//TODO:
	return 0;
}

int codec_load_config(struct comp_dev *dev, void *cfg, size_t size,
		       enum codec_cfg_type type)
{
	int ret;
	struct codec_config *dst;
	struct comp_data *cd = comp_get_drvdata(dev);
	/* wyciagnij cd z dev i skopiuj config do pola "data", na tym etapie nie
	trzeba wiedziec jaki to jest codec id
	*/
	comp_dbg(dev, "codec_load_config() start");

	dst = (type == CODEC_CFG_SETUP) ? cd->codec.s_cfg :
					  cd->codec.r_cfg;

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

int codec_init(struct comp_dev *dev)
{
	int ret;
	struct comp_data *cd = comp_get_drvdata(dev);
	uint32_t codec_id = cd->ca_config.codec_id;
	struct codec_interface *interface = NULL;
	uint32_t i;
	uint32_t no_of_interfaces = sizeof(interfaces) /
				    sizeof(struct codec_interface);

	comp_info(dev, "codec_init() start");

	/* Find proper interface */
	for (i = 0; i < no_of_interfaces; i++) {
		if (interfaces[i].id == codec_id)
			interface = interfaces[i];
			break;
	}
	if (!interface) {
		comp_err(dev, "codec_init() error: could not find codec interface for codec id %x",
			 codec_id);
		ret = -EIO;
		goto out;
	} else if (!interface->init) {//TODO: verify other interfaces
		comp_err(dev, "codec_init() error: codec %x is missing required interfaces",
			 codec_id);
		ret = -EIO;
		goto out;
	}

	/* Assign interface */
	cd->codec.interface = interface;

	/* Now we can proceed with codec specific initialization */
	interface->init(dev);

	//codec_data.api = codec_lib[codec_id].api;

	//codec_data[codec_id].init(...);

	//codec->interface.init(...);

	comp_info(dev, "codec_init() done");
out:
	return ret;
}
