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
#include <sof/audio/codec_adapter/codec_adapter.h>
#include <sof/audio/pipeline.h>
#include <sof/common.h>

static const struct comp_driver comp_codec_adapter;

/* d8218443-5ff3-4a4c-b388-6cfe07b956aa */
DECLARE_SOF_RT_UUID("codec_adapter", ca_uuid, 0xd8218443, 0x5ff3, 0x4a4c,
		    0xb3, 0x88, 0x6c, 0xfe, 0x07, 0xb9, 0x56, 0xaa);

DECLARE_TR_CTX(ca_tr, SOF_UUID(ca_uuid), LOG_LEVEL_INFO);

/**
 * \brief Create a codec adapter component.
 * \param[in] drv - component driver pointer.
 * \param[in] comp - component ipc descriptor pointer.
 *
 * \return: a pointer to newly created codec adapter component.
 */
static struct comp_dev *codec_adapter_new(const struct comp_driver *drv,
					  struct sof_ipc_comp *comp)
{
	int ret;
	struct comp_dev *dev;
	struct comp_data *cd;
	struct sof_ipc_comp_process *ipc_codec_adapter =
		(struct sof_ipc_comp_process *)comp;

	comp_cl_info(&comp_codec_adapter, "codec_adapter_new() start");

	if (!drv || !comp) {
		comp_cl_err(&comp_codec_adapter, "codec_adapter_new(), wrong input params! drv = %x comp = %x",
			    (uint32_t)drv, (uint32_t)comp);
		return NULL;
	}

	dev = comp_alloc(drv, COMP_SIZE(struct sof_ipc_comp_process));
	if (!dev) {
		comp_cl_err(&comp_codec_adapter, "codec_adapter_new(), failed to allocate memory for comp_dev");
		return NULL;
	}

	dev->drv = drv;

	ret = memcpy_s(&dev->comp, sizeof(struct sof_ipc_comp_process),
		       comp, sizeof(struct sof_ipc_comp_process));
	assert(!ret);

	cd = rzalloc(SOF_MEM_ZONE_RUNTIME, 0, SOF_MEM_CAPS_RAM, sizeof(*cd));
	if (!cd) {
		comp_cl_err(&comp_codec_adapter, "codec_adapter_new(), failed to allocate memory for comp_data");
		rfree(dev);
		return NULL;
	}

	comp_set_drvdata(dev, cd);
	/* Copy setup config */
	ret = load_setup_config(dev, ipc_codec_adapter->data, ipc_codec_adapter->size);
	if (ret) {
		comp_err(dev, "codec_adapter_new() error %d: config loading has failed.",
			 ret);
		goto err;
	}
	/* Init processing codec */
	ret = codec_init(dev);
	if (ret) {
		comp_err(dev, "codec_adapter_new() %d: codec initialization failed",
			 ret);
		goto err;
	}

	dev->state = COMP_STATE_READY;
	cd->state = PP_STATE_CREATED;

	comp_cl_info(&comp_codec_adapter, "codec_adapter_new() done");
	return dev;
err:
	rfree(cd);
	rfree(dev);
	return NULL;
}

static inline int validate_setup_config(struct ca_config *cfg)
{
	/* TODO: validate codec_adapter setup parameters */
	return 0;
}

/**
 * \brief Load setup config for both codec adapter and codec library.
 * \param[in] dev - codec adapter component device pointer.
 * \param[in] cfg - pointer to the configuration data.
 * \param[in] size - size of config.
 *
 * The setup config comprises of two parts - one contains essential data
 * for the initialization of codec_adapter and follows struct ca_config.
 * Second contains codec specific data needed to setup the codec itself.
 * The leter is send in a TLV format organized by struct codec_param.
 *
 * \return integer representing either:
 *	0 -> success
 *	negative value -> failure.
 */
static int load_setup_config(struct comp_dev *dev, void *cfg, uint32_t size)
{
	int ret;
	void *lib_cfg;
	size_t lib_cfg_size;
	struct comp_data *cd = comp_get_drvdata(dev);

	comp_dbg(dev, "load_setup_config() start.");

	if (!dev) {
		comp_err(dev, "load_setup_config(): no component device.");
		ret = -EINVAL;
		goto end;
	} else if (!cfg || !size) {
		comp_err(dev, "load_setup_config(): no config available cfg: %x, size: %d",
			 (uintptr_t)cfg, size);
		ret = -EINVAL;
		goto end;
	} else if (size <= sizeof(struct ca_config)) {
		comp_err(dev, "load_setup_config(): no codec config available.");
		ret = -EIO;
		goto end;
	}
	/* Copy codec_adapter part */
	ret = memcpy_s(&cd->ca_config, sizeof(cd->ca_config), cfg,
		       sizeof(struct ca_config));
	assert(!ret);
	ret = validate_setup_config(&cd->ca_config);
	if (ret) {
		comp_err(dev, "load_setup_config(): validation of setup config for codec_adapter failed.");
		goto end;
	}
	/* Copy codec specific part */
	lib_cfg = (char *)cfg + sizeof(struct ca_config);
	lib_cfg_size = size - sizeof(struct ca_config);
	ret = codec_load_config(dev, lib_cfg, lib_cfg_size, CODEC_CFG_SETUP);
	if (ret) {
		comp_err(dev, "load_setup_config(): %d: failed to load setup config for codec id %x",
			 ret, cd->ca_config.codec_id);
		goto end;
	}

	comp_dbg(dev, "load_setup_config() done.");
end:
	return ret;
}

/*
 * \brief Prepare a codec adapter component.
 * \param[in] dev - component device pointer.
 *
 * \return integer representing either:
 *	0 - success
 *	value < 0 - failure.
 */
static int codec_adapter_prepare(struct comp_dev *dev)
{
	int ret;
	struct comp_data *cd = comp_get_drvdata(dev);

	comp_info(dev, "codec_adapter_prepare() start");

	/* Init sink & source buffers */
	cd->ca_sink = list_first_item(&dev->bsink_list, struct comp_buffer,
				      source_list);
	cd->ca_source = list_first_item(&dev->bsource_list, struct comp_buffer,
					sink_list);

	if (!cd->ca_source) {
		comp_err(dev, "codec_adapter_prepare(): source buffer not found");
		return -EINVAL;
	} else if (!cd->ca_sink) {
		comp_err(dev, "codec_adapter_prepare(): sink buffer not found");
		return -EINVAL;
	}

	/* Are we already prepared? */
	ret = comp_set_state(dev, COMP_TRIGGER_PREPARE);
	if (ret < 0)
		return ret;

	if (ret == COMP_STATUS_STATE_ALREADY_SET) {
		comp_warn(dev, "codec_adapter_prepare(): codec_adapter has already been prepared");
		return PPL_STATUS_PATH_STOP;
	}

	/* Prepare codec */
	ret = codec_prepare(dev);
	if (ret) {
		comp_err(dev, "codec_adapter_prepare() error %x: codec prepare failed",
			 ret);

		return -EIO;
	}

	comp_info(dev, "codec_adapter_prepare() done");
	cd->state = PP_STATE_PREPARED;

	return 0;
}

static int codec_adapter_params(struct comp_dev *dev,
				    struct sof_ipc_stream_params *params)
{
	comp_dbg(dev, "codec_adapter_params(): codec_adapter doesn't support .params() method.");

	return 0;
}

static const struct comp_driver comp_codec_adapter = {
	.type = SOF_COMP_NONE,
	.uid = SOF_RT_UUID(ca_uuid),
	.tctx = &ca_tr,
	.ops = {
		.create = codec_adapter_new,
		.prepare = codec_adapter_prepare,
		.params = codec_adapter_params,
	},
};
