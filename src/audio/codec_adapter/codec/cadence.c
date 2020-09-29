// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2020 Intel Corporation. All rights reserved.
//
// Author: Marcin Rajwa <marcin.rajwa@linux.intel.com>

/*
 * \file cadence.c
 * \brief Cadence Codec API
 * \author Marcin Rajwa <marcin.rajwa@linux.intel.com>
 *
 */

#include <sof/audio/codec_adapter/codec/generic.h>
#include <sof/audio/codec_adapter/codec/cadence.h>

int cadence_codec_init(struct comp_dev *dev)
{
	int ret;
	struct codec_data *codec = comp_get_codec(dev);
	struct cadence_codec_data *cd = NULL;
	uint32_t obj_size;

	comp_dbg(dev, "cadence_codec_init() start");

	cd = codec_allocate_memory(dev, sizeof(struct cadence_codec_data), 0);
	if (!cd) {
		comp_err(dev, "cadence_codec_init(): failed to allocate memory for cadence codec data");
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
		codec_free_memory(dev, cd);
		goto out;
	}
	API_CALL(cd, XA_API_CMD_GET_API_SIZE, 0, &obj_size, ret);
	if (ret != LIB_NO_ERROR) {
		comp_err(dev, "cadence_codec_init() error %x: failed to get lib object size",
			 ret);
		codec_free_memory(dev, cd);
		goto out;
	}
	cd->self = codec_allocate_memory(dev, obj_size, 0);
	if (!cd->self) {
		comp_err(dev, "cadence_codec_init(): failed to allocate space for lib object");
		codec_free_memory(dev, cd);
		goto out;
	} else {
		comp_dbg(dev, "cadence_codec_init(): allocated %d bytes for lib object",
			 obj_size);
	}

	comp_dbg(dev, "cadence_codec_init() done");
out:
	return ret;
}

static int apply_config(struct comp_dev *dev, enum codec_cfg_type type)
{
	int ret;
	int size;
	struct codec_config *cfg;
	void *data;
	struct codec_param *param;
	struct codec_data *codec = comp_get_codec(dev);
	struct cadence_codec_data *cd = codec->private;

	comp_dbg(dev, "apply_config() start");

	cfg = (type == CODEC_CFG_SETUP) ? &codec->s_cfg :
					  &codec->r_cfg;
	data = cfg->data;
	size = cfg->size;

	if (!cfg->avail || !size) {
		comp_err(dev, "apply_config() error: no config available, requested conf. type %d",
			 type);
		ret = -EIO;
		goto ret;
	}

	while (size > 0) {
		param = data;
		comp_dbg(dev, "apply_config() applying param %d value %d",
			 param->id, param->data[0]);
		API_CALL(cd, XA_API_CMD_SET_CONFIG_PARAM, param->id,
			 param->data, ret);
		if (ret != LIB_NO_ERROR) {
			comp_err(dev, "apply_config() error %x: failed to apply parameter %d value %d",
				 ret, param->id, *(int32_t *)param->data);
			goto ret;
		}
		data = (char *)data + param->size;
		size -= param->size;
	}

	comp_dbg(dev, "apply_config() done");
ret:
	return ret;
}

int cadence_codec_prepare(struct comp_dev *dev)
{
	int ret;
	struct codec_data *codec = comp_get_codec(dev);
	struct cadence_codec_data *cd = codec->private;

	comp_dbg(dev, "cadence_codec_prepare() start");

	API_CALL(cd, XA_API_CMD_INIT, XA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS,
		 NULL, ret);
	if (ret != LIB_NO_ERROR) {
		comp_err(dev, "cadence_codec_prepare(): error %x: failed to set default config",
			 ret);
		goto err;
	}

	if (!codec->s_cfg.avail && !codec->s_cfg.size) {
		comp_err(dev, "cadence_codec_prepare() no setup configuration available!");
		ret = -EIO;
		goto err;
	} else if (!codec->s_cfg.avail) {
		comp_warn(dev, "cadence_codec_prepare(): no new setup configuration available, using the old one");
		codec->s_cfg.avail = true;
	}
	ret = apply_config(dev, CODEC_CFG_SETUP);
	if (ret) {
		comp_err(dev, "cadence_codec_prepare() error %x: failed to applay setup config",
			 ret);
		goto err;
	}
	/* Do not reset nor free codec setup config "size" so we can use it
	 * later on in case there is no new one upon reset.
	 */
	codec->s_cfg.avail = false;

	comp_dbg(dev, "cadence_codec_prepare() done");
	return 0;
err:
	return ret;
}
