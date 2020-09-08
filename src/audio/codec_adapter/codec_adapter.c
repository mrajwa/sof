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
	struct comp_dev *dev;
	struct comp_data *cd;
	struct sof_ipc_comp_process *ipc_codec_adapter =
		(struct sof_ipc_comp_process *)comp;
	struct ca_config *cfg;
	size_t bs;
	void *lib_cfg;
	size_t lib_cfg_size;

	comp_cl_info(&comp_codec_adapter, "codec_adapter_new()");

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

	/* Copy setup config of codec_adapter */
	cfg = (struct ca_config *)ipc_codec_adapter->data;
	bs = ipc_codec_adapter->size;
	if (bs) {
		if (bs < sizeof(struct ca_config)) {
			comp_info(dev, "codec_adapter_new() error: wrong size of setup config");
			goto err;
		}
		ret = memcpy_s(&cd->ca_config, sizeof(cd->ca_config), cfg,
			       sizeof(struct ca_config));
		assert(!ret);
		ret = validate_setup_config(&cd->ca_config);
		if (ret) {
			comp_err(dev, "codec_adapter_new(): error: validation of setup config failed");
			goto err;
		}

		/* Pass config further to the codec */
		lib_cfg = (char *)cfg + sizeof(struct ca_config);
		lib_cfg_size = bs - sizeof(struct ca_config);
		ret = codec_load_config(dev, lib_cfg, lib_cfg_size,
					CODEC_CFG_SETUP);
		if (ret) {
			comp_err(dev, "codec_adapter_new(): error %d: failed to load setup config for codec",
				 ret);
		} else {
			comp_dbg(dev, "codec_adapter_new() codec config loaded successfully");
		}
	} else {
		comp_err(dev, "codec_adapter_new(): no configuration available");
		goto err;
	}

	/* Init processing codec */
	ret = codec_init(dev);
	if (ret) {
		comp_err(dev, "codec_adapter_new() error %d: codec initialization failed",
			 ret);
		goto err;
	}

	dev->state = COMP_STATE_READY;
	cd->state = PP_STATE_CREATED;

	return dev;
err:
	//TODO: handle errors
	return NULL;
}

static int codec_adapter_verify_params(struct comp_dev *dev,
			     	struct sof_ipc_stream_params *params)
{
	/* TODO check on GP level. Params to codec shall me sent
	 * in dedicated binary_set ipc
	 */
	return 0;
}

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
                comp_err(dev, "codec_adapter_prepare() erro: source buffer not found");
                return -EINVAL;
        } else if (!cd->ca_sink) {
                comp_err(dev, "codec_adapter_prepare() erro: sink buffer not found");
                return -EINVAL;
        }

	/* Are we already prepared? */
	ret = comp_set_state(dev, COMP_TRIGGER_PREPARE);
	if (ret < 0)
		return ret;

	if (ret == COMP_STATUS_STATE_ALREADY_SET) {
		comp_err(dev, "codec_adapter_prepare() error %x: codec_adapter has already been prepared",
			 ret);
		return 0;
	}

	/* Prepare codec */
	cd->codec.stream_dsc.rate = cd->ca_source->stream.rate;
	cd->codec.stream_dsc.frame_fmt = cd->ca_source->stream.frame_fmt;
	cd->codec.stream_dsc.channels = cd->ca_source->stream.channels;
	ret = codec_prepare(dev);
	if (ret) {
		comp_err(dev, "codec_adapter_prepare() error %x: codec prepare failed",
			 ret);

		return -EIO;
	} else {
		comp_info(dev, "codec_adapter_prepare() codec prepared successfully");
	}

	comp_info(dev, "codec_adapter_prepare() done");
        cd->state = PP_STATE_PREPARED;

	return 0;
}

static int codec_adapter_params(struct comp_dev *dev,
				    struct sof_ipc_stream_params *params)
{
	int ret = 0;

	if (dev->state == COMP_STATE_PREPARE) {
		comp_warn(dev, "codec_adapter_params(): params has already been prepared.");
		goto end;
	}

	ret = codec_adapter_verify_params(dev, params);
	if (ret < 0) {
		comp_err(dev, "codec_adapter_params(): pcm params verification failed");
		goto end;
	}

end:
	return ret;
}

static void codec_adapter_copy_to_lib(const struct audio_stream *source,
			      void *lib_buff, size_t size)
{
	void *src;
	void *dst = lib_buff;
	size_t i;
	size_t j = 0;
	size_t channel;
	size_t channels = source->channels;
	size_t sample_width = source->frame_fmt == SOF_IPC_FRAME_S16_LE ?
			  16 : 32;
	size_t frames = size / (sample_width / 8 * channels);

	for (i = 0; i < frames; i++) {
		for (channel = 0; channel < channels; channel++) {
			switch (sample_width) {
			case 16:
				src = audio_stream_read_frag_s16(source, j);
				*((int16_t *)dst) = *((int16_t *)src);
				dst = ((int16_t *)dst) + 1;
				break;
#if CONFIG_FORMAT_S24LE || CONFIG_FORMAT_S32LE
			case 24:
			case 32:
				src = audio_stream_read_frag_s32(source, j);
				*((int32_t *)dst) = *((int32_t *)src);
				dst = ((int32_t *)dst) + 1;
				break;
#endif /* CONFIG_FORMAT_S24LE || CONFIG_FORMAT_S32LE*/
			default:
				comp_cl_info(&comp_codec_adapter, "codec_adapter_copy_to_lib(): An attempt to copy not supported format!");
				return;
			}
			j++;
		}
	}

}

static void codec_adapter_copy_from_lib_to_sink(void *source, struct audio_stream *sink,
			      size_t size)
{
	void *dst;
	void *src = source;
	size_t i;
	size_t j = 0;
	size_t channel;
	size_t sample_width = sink->frame_fmt == SOF_IPC_FRAME_S16_LE ?
			  16 : 32;
	size_t channels = sink->channels;
	size_t frames = size / (sample_width / 8 * channels);


	for (i = 0; i < frames; i++) {
		for (channel = 0; channel < channels; channel++) {
			switch (sample_width) {
#if CONFIG_FORMAT_S16LE
			case 16:
				dst = audio_stream_write_frag_s16(sink, j);
				*((int16_t *)dst) = *((int16_t *)src);
				src = ((int16_t *)src) + 1;
				break;
#endif /* CONFIG_FORMAT_S16LE */
#if CONFIG_FORMAT_S24LE || CONFIG_FORMAT_S32LE
			case 24:
			case 32:
				dst = audio_stream_write_frag_s32(sink, j);
				*((int32_t *)dst) = *((int32_t *)src);
				src = ((int32_t *)src) + 1;
				break;
#endif /* CONFIG_FORMAT_S24LE || CONFIG_FORMAT_S32LE */
			default:
				comp_cl_info(&comp_codec_adapter, "generic_processor_copy_to_lib(): An attempt to copy not supported format!");
				return;
			}
			j++;
		}
	}

}

static int codec_adapter_copy(struct comp_dev *dev)
{
	int ret = 0;
	uint32_t copy_bytes, bytes_to_process, processed = 0;
	struct comp_data *cd = comp_get_drvdata(dev);
	struct codec_data *codec = &cd->codec;
	struct comp_buffer *source = cd->ca_source;
	struct comp_buffer *sink = cd->ca_sink;
        uint32_t lib_buff_size = codec->cpd.in_buff_size;


        bytes_to_process = MIN(sink->stream.free, source->stream.avail);
	copy_bytes = MIN(sink->stream.free, source->stream.avail);

        comp_info(dev, "codec_adapter_copy() start lib_buff_size: %d, copy_bytes: %d",
        	  lib_buff_size, copy_bytes);

	while (bytes_to_process) {
		if (bytes_to_process < lib_buff_size) {
			comp_info(dev, "codec_adapter_copy(): processed %d in this call %d bytes left for next period",
			        processed, bytes_to_process);
			break;
		}

		/* Fill lib buffer completely. NOTE! If you don't fill whole buffer
		 * the lib won't process it.
		 */
		codec_adapter_copy_to_lib(&source->stream,
					  codec->cpd.in_buff,
					  lib_buff_size);
		codec->cpd.avail = lib_buff_size;
		ret = codec_process(dev);
		if (ret) {
			comp_err(dev, "codec_adapter_copy() error %x: lib processing failed",
				 ret);
			break;
		} else if (codec->cpd.produced == 0) {
			/* skipping as lib has not produced anything */
                        comp_err(dev, "codec_adapter_copy() error %x: lib hasn't processed anything",
                                 ret);
			break;
		}

                codec_adapter_copy_from_lib_to_sink(codec->cpd.out_buff,
                				    &sink->stream, codec->cpd.produced);

		bytes_to_process -= codec->cpd.produced;
		processed += codec->cpd.produced;
	}

	if (!processed) {
		comp_err(dev, "codec_adapter_copy() error: failed to process anything in this call!");
		goto end;
	} else {
		comp_info(dev, "codec_adapter_copy: codec processed %d bytes", processed);
	}


	comp_update_buffer_produce(sink, processed);
	comp_update_buffer_consume(source, processed);
end:
        comp_info(dev, "codec_adapter_copy() end processed: %d", processed);
	return ret;
}

static void codec_adapter_free(struct comp_dev *dev)
{
	struct comp_data *cd = comp_get_drvdata(dev);

	comp_cl_info(&comp_codec_adapter, "codec_adapter_free(): start");

	rfree(cd);
	rfree(dev);
	//TODO: call lib API to free its resources

	comp_cl_info(&comp_codec_adapter, "codec_adapter_free(): component memory freed");

}

static int codec_adapter_trigger(struct comp_dev *dev, int cmd)
{
	comp_cl_info(&comp_codec_adapter, "codec_adapter_trigger(): component got trigger cmd %x",
		     cmd);

	//TODO: ask lib if pp parameters has been aplied and if not log it!
        //likely here change detect COMP_TRIGGER_START cmd and change state to PP_STATE_RUN
	return comp_set_state(dev, cmd);
}

static int codec_adapter_reset(struct comp_dev *dev)
{
        struct comp_data *cd = comp_get_drvdata(dev);

	comp_cl_info(&comp_codec_adapter, "codec_adapter_reset(): resetting");

        cd->state = PP_STATE_CREATED;
        //TODO: reset codec params

	return comp_set_state(dev, COMP_TRIGGER_RESET);
}

static const struct comp_driver comp_codec_adapter = {
	.type = SOF_COMP_NONE,
	.uid = SOF_RT_UUID(ca_uuid),
	.tctx = &ca_tr,
	.ops = {
		.create = codec_adapter_new,
		.params = codec_adapter_params,
		.prepare = codec_adapter_prepare,
		.copy = codec_adapter_copy,
		.free = codec_adapter_free,
		.trigger = codec_adapter_trigger,
		.reset = codec_adapter_reset,
	},
};

static SHARED_DATA struct comp_driver_info comp_codec_adapter_info = {
	.drv = &comp_codec_adapter,
};

UT_STATIC void sys_comp_codec_adapter_init(void)
{
	comp_register(platform_shared_get(&comp_codec_adapter_info,
					  sizeof(comp_codec_adapter_info)));
}

DECLARE_MODULE(sys_comp_codec_adapter_init);
