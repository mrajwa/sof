/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2020 Intel Corporation. All rights reserved.
 *
 *
 * \file cadence.c
 * \brief Cadence Codec API
 * \author Marcin Rajwa <marcin.rajwa@linux.intel.com>
 *
 */

#include <sof/audio/codec_adapter/codec/generic.h>
#include <sof/audio/codec_adapter/codec/cadence.h>


int cadence_codec_init(struct comp_dev *dev) {
	int ret;
	struct codec_data *codec = comp_get_codec(dev);
	struct cadence_codec_data *cd = NULL;
	uint32_t obj_size;

	comp_info(dev, "cadence_codec_init() start");

	cd = rballoc(0, SOF_MEM_CAPS_RAM, sizeof(struct cadence_codec_data));
	if (!cd) {
		comp_err(dev, "cadence_codec_init() error: failed to allocate memory for cadence codec data");
		return -ENOMEM;
	}
	codec->private = cd;
	cd->self = NULL;
	cd->mem_tabs = NULL;

	API_CALL(cd, XA_API_CMD_GET_LIB_ID_STRINGS,
		 XA_CMD_TYPE_LIB_NAME, cd->name, ret);
	if (ret != LIB_NO_ERROR) {
		comp_err(dev, "cadence_codec_init() error %x: failed to get lib name",
			 ret);
		goto out;
	}

	API_CALL(cd, XA_API_CMD_GET_API_SIZE, 0, &obj_size, ret);
	if (ret != LIB_NO_ERROR) {
		comp_err(dev, "cadence_codec_init() error %x: failed to get lib object size",
			 ret);
		goto out;
	}
	cd->self = rballoc(0, SOF_MEM_CAPS_RAM, obj_size);
	if (!cd->self) {
		comp_err(dev, "cadence_codec_init() error: failed to allocate space for lib object");
		goto out;
	} else {
		comp_dbg(dev, "cadence_codec_init(): allocated %d bytes for lib object",
			 obj_size);
	}

	API_CALL(cd, XA_API_CMD_INIT, XA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS,
		 NULL, ret);
	if (ret != LIB_NO_ERROR) {
		comp_err(dev, "cadence_codec_init(): error %x: failed to set default config",
			 ret);
		goto out;
	}

	comp_info(dev, "cadence_codec_init() done");
	cd->state = CADENCE_CODEC_INITIALIZED;
out:
	return ret;
}
