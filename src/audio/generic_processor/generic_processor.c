// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2020 Intel Corporation. All rights reserved.
//
// Author: Marcin Rajwa <marcin.rajwa@linux.intel.com>

/*
 * A post processing component.
 */

/**
 * \file audio/generic_processor.c
 * \brief Post processing component
 * \author Marcin Rajwa <marcin.rajwa@linux.intel.com>
 */

#include <sof/audio/buffer.h>
#include <sof/audio/component.h>
#include <sof/audio/pipeline.h>
#include <sof/audio/generic_processor/generic_processor.h>
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

static const struct comp_driver comp_generic_processor;

/* d8218443-5ff3-4a4c-b388-6cfe07b9562e */
DECLARE_SOF_RT_UUID("gp", gp_uuid, 0xd8218443, 0x5ff3, 0x4a4c,
		 0xb3, 0x88, 0x6c, 0xfe, 0x07, 0xb9, 0x56, 0xAA);

DECLARE_TR_CTX(gp_tr, SOF_UUID(gp_uuid), LOG_LEVEL_INFO);

static struct comp_dev *generic_processor_new(const struct comp_driver *drv,
					 struct sof_ipc_comp *comp)
{
	int ret;
	struct comp_dev *dev = NULL;
	struct comp_data *cd = NULL;
	struct sof_ipc_comp_process *ipc_generic_processor =
		(struct sof_ipc_comp_process *)comp;
	struct generic_processor_config *cfg;
	size_t bs;
	void *lib_cfg;
	size_t lib_cfg_size;

	comp_cl_info(&comp_generic_processor, "generic_processor_new()");

	dev = comp_alloc(drv, COMP_SIZE(struct sof_ipc_comp_process));
	if (!dev) {
		comp_cl_err(&comp_generic_processor, "generic_processor_new(), failed to allocate memory for comp_dev");
		goto err;
	}

	dev->drv = drv;

	ret = memcpy_s(&dev->comp, sizeof(struct sof_ipc_comp_process),
		       comp, sizeof(struct sof_ipc_comp_process));
	assert(!ret);

	cd = rzalloc(SOF_MEM_ZONE_RUNTIME, 0, SOF_MEM_CAPS_RAM, sizeof(*cd));
	if (!cd) {
		comp_cl_err(&comp_generic_processor, "generic_processor_new(), failed to allocate memory for comp_data");
		goto err;
	}

	comp_set_drvdata(dev, cd);

	/* Copy setup config */
	cfg = (struct generic_processor_config *)ipc_generic_processor->data;
	bs = ipc_generic_processor->size;
	if (bs) {
		if (bs < sizeof(struct generic_processor_config)) {
			comp_info(dev, "generic_processor_new() error: wrong size of post processing config");
			goto err;
		}
		ret = memcpy_s(&cd->gp_config, sizeof(cd->gp_config), cfg,
			       sizeof(struct generic_processor_config));
		assert(!ret);
		ret = validate_config(&cd->gp_config);
		if (ret) {
			comp_err(dev, "generic_processor_new(): error: validation of pp config failed");
			goto err;
		}

		/* Pass config further to the codec */
		lib_cfg = (char *)cfg + sizeof(struct generic_processor_config);
		lib_cfg_size = bs - sizeof(struct generic_processor_config);
		ret = hifi_codec_load_config(dev, lib_cfg, lib_cfg_size, PP_CFG_SETUP);
		if (ret) {
			comp_err(dev, "generic_processor_new(): error %x: failed to load config for codec",
				 ret);

		} else {
			comp_info(dev, "generic_processor_new() codec config load successfully");
		}

	} else {
		comp_err(dev, "generic_processor_new(): no configuration available");
		goto err;
	}

	/* Init processing codec */
        ret = hifi_codec_init(dev, cd->gp_config.codec_id);
        if (ret) {
		comp_err(dev, "generic_processor_new() error %x: lib initialization failed",
			 ret);
		goto err;
        }

	dev->state = COMP_STATE_READY;
        cd->state = PP_STATE_CREATED;

	comp_cl_info(&comp_generic_processor, "generic_processor_new(): component created successfully");

	return dev;
err:
	if (cd)
		rfree(cd);
	if (dev)
		rfree(dev);
	return NULL;
}

static int generic_processor_prepare(struct comp_dev *dev)
{
	int ret;
	struct comp_data *cd = comp_get_drvdata(dev);
	bool lib_state;

	comp_info(dev, "generic_processor_prepare() start");

	/* Init sink & source buffers */
	cd->gp_sink = list_first_item(&dev->bsink_list, struct comp_buffer,
				      source_list);
        cd->gp_source = list_first_item(&dev->bsource_list, struct comp_buffer,
                                        sink_list);

        if (!cd->gp_source) {
                comp_err(dev, "generic_processor_prepare() erro: source buffer not found");
                return -EINVAL;
        } else if (!cd->gp_sink) {
                comp_err(dev, "generic_processor_prepare() erro: sink buffer not found");
                return -EINVAL;
        }

	/* Are we already prepared? */
	ret = comp_set_state(dev, COMP_TRIGGER_PREPARE);
	if (ret < 0)
		return ret;

	if (ret == COMP_STATUS_STATE_ALREADY_SET)
		return PPL_STATUS_PATH_STOP;

	ret = hifi_codec_get_state(&lib_state);
	if (ret) {
		comp_err(dev, "generic_processor_prepare() error %x: could not get lib state",
			 ret);
		return -EIO;
	} else if (lib_state >= HIFI_CODEC_PREPARED) {
		comp_info(dev, "generic_processor_prepare() lib already prepared");
		goto done;
	}

	/* Prepare HiFi codec library */
	ret = hifi_codec_prepare(dev, &cd->sdata);
	if (ret) {
		comp_err(dev, "generic_processor_prepare() error %x: lib prepare failed",
			 ret);

		return -EIO;
	} else {
		comp_info(dev, "generic_processor_prepare() lib prepared successfully");
	}

        /* Do we have runtime config available? */
        if (cd->codec_r_cfg_avail) {
                ret = hifi_codec_apply_config(dev, PP_CFG_RUNTIME);
                if (ret) {
                        comp_err(dev, "generic_processor_prepare() error %x: lib config apply failed",
                                 ret);
                        return -EIO;
                } else {
                        comp_info(dev, "generic_processor_prepare() lib runtime config applied successfully");
                }
        }
done:
	comp_info(dev, "generic_processor_prepare() done");
        cd->state = PP_STATE_PREPARED;

	return 0;
}

static void generic_processor_free(struct comp_dev *dev)
{
	struct comp_data *cd = comp_get_drvdata(dev);

	comp_cl_info(&comp_generic_processor, "generic_processor_free(): start");

	rfree(cd);
	rfree(dev);
	//TODO: call lib API to free its resources

	comp_cl_info(&comp_generic_processor, "generic_processor_free(): component memory freed");

}

static int generic_processor_trigger(struct comp_dev *dev, int cmd)
{
	comp_cl_info(&comp_generic_processor, "generic_processor_trigger(): component got trigger cmd %x",
		     cmd);

	//TODO: ask lib if pp parameters has been aplied and if not log it!
        //likely here change detect COMP_TRIGGER_START cmd and change state to PP_STATE_RUN
	return comp_set_state(dev, cmd);
}

static int generic_processor_reset(struct comp_dev *dev)
{
        struct comp_data *cd = comp_get_drvdata(dev);

	comp_cl_info(&comp_generic_processor, "generic_processor_reset(): resetting");

        cd->state = PP_STATE_CREATED;
        //TODO: reset codec params

	return comp_set_state(dev, COMP_TRIGGER_RESET);
}

static void generic_processor_copy_to_lib(const struct audio_stream *source,
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
				comp_cl_info(&comp_generic_processor, "generic_processor_copy_to_lib(): An attempt to copy not supported format!");
				return;
			}
			j++;
		}
	}
}

static void generic_processor_copy_from_lib_to_sink(void *source, struct audio_stream *sink,
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
				comp_cl_info(&comp_generic_processor, "generic_processor_copy_to_lib(): An attempt to copy not supported format!");
				return;
			}
			j++;
		}
	}
}

static int generic_processor_copy(struct comp_dev *dev)
{
	int ret = 0;
	uint32_t copy_bytes, bytes_to_process, produced, processed = 0;
	struct comp_data *cd = comp_get_drvdata(dev);
	struct comp_buffer *source = cd->gp_source;
	struct comp_buffer *sink = cd->gp_sink;
        uint32_t lib_buff_size = cd->sdata.lib_in_buff_size;

        comp_info(dev, "generic_processor_copy() start");

        bytes_to_process = MIN(sink->stream.free, source->stream.avail);
	copy_bytes = MIN(bytes_to_process, lib_buff_size);

	while (bytes_to_process) {
		if (bytes_to_process < lib_buff_size) {
			comp_dbg(dev, "generic_processor_copy(): skipping processing as we don't have enough data. Only %d bytes available in source buffer",
			        bytes_to_process);
			ret = PPL_STATUS_PATH_STOP;
			goto end;
		}

		/* Fill lib buffer completely. NOTE! If you don't fill whole buffer
		 * the lib won't process it.
		 */
		generic_processor_copy_to_lib(&source->stream,
					 cd->sdata.lib_in_buff, lib_buff_size);

		ret = hifi_codec_process_data(dev, lib_buff_size, &produced);
		if (ret) {
			comp_err(dev, "generic_processor_copy() error %x: lib processing failed",
				 ret);
			ret = PPL_STATUS_PATH_STOP;
			goto end;
		} else if (produced == 0) {
			/* skipping as lib has not produced anything */
                        comp_err(dev, "generic_processor_copy() error %x: lib hasn't processed anything",
                                 ret);
			ret = PPL_STATUS_PATH_STOP;
			goto end;
		}

                generic_processor_copy_from_lib_to_sink(cd->sdata.lib_out_buff,
                					&sink->stream, produced);

		bytes_to_process -= produced;
		processed += produced;
	}

	if (!processed) {
		comp_err(dev, "generic_processor_copy() error: failed to process anything in this call!");
		goto end;
	}
	comp_update_buffer_produce(sink, processed);
	comp_update_buffer_consume(source, processed);
end:
	return ret;
}


static int gp_set_config(struct comp_dev *dev,
			 struct sof_ipc_ctrl_data *cdata) {
	//TODO add load of setup config
	/* At this point the setup config is small enough so it fits
	 * in single ipc therfore will be send in .new()
	 */
	return 0;
}

static int gp_set_runtime_params(struct comp_dev *dev,
			      struct sof_ipc_ctrl_data *cdata) {
	int ret;
	char *dst, *src;
	static uint32_t size;
	uint32_t offset, lib_max_blob_size;
        struct comp_data *cd = comp_get_drvdata(dev);

	/* Stage 1 load whole config locally */
	/* Check that there is no work-in-progress previous request */
	if (cd->gp_lib_runtime_config && cdata->msg_index == 0) {
		comp_err(dev, "gp_set_runtime_params() error: busy with previous request");
		return -EBUSY;
	}

	comp_info(dev, "gp_set_runtime_params(): num_of_elem %d, elem remain %d msg_index %u",
		  cdata->num_elems, cdata->elems_remaining, cdata->msg_index);

	ret = hifi_codec_get_max_blob_size(&lib_max_blob_size);
	if (ret) {
		comp_err(dev, "gp_set_runtime_params() error: could not get blob size limit from the lib");
		goto end;
	}
	if (cdata->num_elems + cdata->elems_remaining > lib_max_blob_size)
	{
		comp_err(dev, "gp_set_runtime_params() error: blob size is too big!");
		ret = -EINVAL;
		goto end;
	}

	if (cdata->msg_index == 0) {
		/* Allocate buffer for new params */
		size = cdata->num_elems + cdata->elems_remaining;
		cd->gp_lib_runtime_config = rballoc(0, SOF_MEM_CAPS_RAM, size);

		if (!cd->gp_lib_runtime_config) {
			comp_err(dev, "gp_set_runtime_params(): space allocation for new params failed");
			ret = -EINVAL;
			goto end;
		}
		memset(cd->gp_lib_runtime_config, 0, size);
	}

	offset = size - (cdata->num_elems + cdata->elems_remaining);
	dst = (char *)cd->gp_lib_runtime_config + offset;
	src = (char *)cdata->data->data;

	ret = memcpy_s(dst,
		       size - offset,
		       src, cdata->num_elems);

	assert(!ret);

	if (cdata->elems_remaining == 0) {
		/* Config has been copied now we can load & apply it
		 * depending on lib status.
		 */
		ret = hifi_codec_load_config(dev, cd->gp_lib_runtime_config, size,
					 PP_CFG_RUNTIME);
		if (ret) {
			comp_err(dev, "gp_set_runtime_params() error %x: lib params load failed",
				 ret);
			goto end;
        	}
		if (cd->state >= PP_STATE_PREPARED) {
			/* Post processing is already prepared so we can apply runtime
			 * config right away.
			 */
			ret = hifi_codec_apply_config(dev, PP_CFG_RUNTIME);
			if (ret) {
				comp_err(dev, "generic_processor_ctrl_set_data() error %x: lib config apply failed",
					 ret);
				goto end;
			}
		} else {
			cd->codec_r_cfg_avail = true;
		}
	}

end:
	if (cd->gp_lib_runtime_config)
		rfree(cd->gp_lib_runtime_config);
	cd->gp_lib_runtime_config = NULL;
	return ret;
}

static int gp_set_binary_data(struct comp_dev *dev,
			      struct sof_ipc_ctrl_data *cdata) {
	int ret;

	 comp_info(dev, "gp_set_binary_data() start, data type %d",
	 	   cdata->data->type);

	switch (cdata->data->type) {
		/* TODO: use enum hifi_codec_cfg_type here */
	case PP_SETUP_CONFIG:
		ret = gp_set_config(dev, cdata);
		break;
	case PP_RUNTIME_PARAMS:
		ret = gp_set_runtime_params(dev, cdata);
		break;
	default:
		comp_err(dev, "gp_set_binary_data() error: unknown binary data type");
		ret = -EIO;
		break;
	}

	return ret;
}

static int generic_processor_ctrl_set_data(struct comp_dev *dev,
                                      struct sof_ipc_ctrl_data *cdata) {
	int ret;

	struct comp_data *cd = comp_get_drvdata(dev);

        comp_info(dev, "generic_processor_ctrl_set_data() start, state %d, cmd %d",
        	  cd->state, cdata->cmd);

	/* Check version from ABI header */
	if (SOF_ABI_VERSION_INCOMPATIBLE(SOF_ABI_VERSION, cdata->data->abi)) {
		comp_err(dev, "generic_processor_ctrl_set_data(): ABI mismatch");
		return -EINVAL;
	}

	switch (cdata->cmd) {
	case SOF_CTRL_CMD_ENUM:
		//TODO
		ret = -EINVAL;
		break;
	case SOF_CTRL_CMD_BINARY:
		ret = gp_set_binary_data(dev, cdata);
		break;
	default:
		comp_err(dev, "generic_processor_ctrl_set_data error: unknown set data command");
		ret = -EINVAL;
		break;
	}
	return ret;
}

/* Used to pass standard and bespoke commands (with data) to component */
static int generic_processor_cmd(struct comp_dev *dev, int cmd, void *data,
			    int max_data_size)
{
	struct sof_ipc_ctrl_data *cdata = data;

	comp_info(dev, "generic_processor_cmd() %d start", cmd);

	switch (cmd) {
	case COMP_CMD_SET_DATA:
		return generic_processor_ctrl_set_data(dev, cdata);
	case COMP_CMD_GET_DATA:
		//TODO
		return -EINVAL;
	default:
		comp_err(dev, "generic_processor_cmd() error: unknown command");
		return -EINVAL;
	}
}

static const struct comp_driver comp_generic_processor = {
	.type = SOF_COMP_GENERIC_PROCESSOR,
	.uid = SOF_RT_UUID(gp_uuid),
	.tctx = &gp_tr,
	.ops = {
		.create = generic_processor_new,
		.free = generic_processor_free,
		.params = NULL,
		.cmd = generic_processor_cmd,
		.trigger = generic_processor_trigger,
		.prepare = generic_processor_prepare,
		.reset = generic_processor_reset,
		.copy = generic_processor_copy,
	},
};


static SHARED_DATA struct comp_driver_info comp_generic_processor_info = {
	.drv = &comp_generic_processor,
};

UT_STATIC void sys_comp_generic_processor_init(void)
{
	comp_register(platform_shared_get(&comp_generic_processor_info,
					  sizeof(comp_generic_processor_info)));
}

DECLARE_MODULE(sys_comp_generic_processor_init);
