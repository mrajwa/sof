// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2020 Intel Corporation. All rights reserved.
//
// Author: Marcin Rajwa <marcin.rajwa@linux.intel.com>

/*
 * A post processing component.
 */

/**
 * \file audio/post_process.c
 * \brief Post processing component
 * \author Marcin Rajwa <marcin.rajwa@linux.intel.com>
 */

#include <sof/audio/buffer.h>
#include <sof/audio/component.h>
#include <sof/audio/pipeline.h>
#include <sof/audio/post_process/post_process.h>
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

static const struct comp_driver comp_post_process;

/* d8218443-5ff3-4a4c-b388-6cfe07b9562e */
DECLARE_SOF_RT_UUID("pp", pp_uuid, 0xd8218443, 0x5ff3, 0x4a4c,
		 0xb3, 0x88, 0x6c, 0xfe, 0x07, 0xb9, 0x56, 0xAA);

DECLARE_TR_CTX(pp_tr, SOF_UUID(pp_uuid), LOG_LEVEL_INFO);


/* Private functions declarations */

static struct comp_dev *post_process_new(const struct comp_driver *drv,
					 struct sof_ipc_comp *comp)
{
	int ret;
	struct comp_dev *dev;
	struct sof_ipc_comp_process *ipc_post_process =
		(struct sof_ipc_comp_process *)comp;
	struct comp_data *cd;
	struct post_process_config *cfg;
	size_t bs;
	void *lib_cfg;
	size_t lib_cfg_size;

	comp_cl_info(&comp_post_process, "post_process_new()");

	dev = comp_alloc(drv, COMP_SIZE(struct sof_ipc_comp_process));
	if (!dev) {
		comp_cl_err(&comp_post_process, "post_process_new(), failed to allocate memory for comp_dev");
		return NULL;
	}

	dev->drv = drv;

	ret = memcpy_s(&dev->comp, sizeof(struct sof_ipc_comp_process),
		       comp, sizeof(struct sof_ipc_comp_process));

	assert(!ret);

	cd = rzalloc(SOF_MEM_ZONE_RUNTIME, 0, SOF_MEM_CAPS_RAM, sizeof(*cd));
	if (!cd) {
		comp_cl_err(&comp_post_process, "post_process_new(), failed to allocate memory for comp_data");
		rfree(dev);
		return NULL;
	}

	comp_set_drvdata(dev, cd);

	/* Init post processing lib */
        ret = pp_init_lib(dev);
        if (ret) {
		comp_err(dev, "post_process_new() error %x: lib initialization failed",
			 ret);
        }

	/* Load post processing runtime config from the topology. */
	//TODO: if you return below, free allocated cd and dev!!!!
	/*
	Jak przekazac setup config zarówno dla compoentu generycznego PP oraz dla LIB?
	wydaje sie, ze najmadrzej zrobic to tak, że w topologii tworzymy jednej config ale
	dodajemy labelki na COMP/LIB i teraz zczytujemy to co dla lib po czym przesoamy tutaj
	pointer i dalej config speczny dla lib'a. Jest pokusa, zeby zrobic to elegancko i zrobic
	pewna czesc wspolna ale to utrudni caly proces. Dzieki temu rozwiazniu powyzej mozemy w tym
	configu dla liba zakodowac rowniez parametr z jakim ma on byc wołany dzieki czemu
	*/
	cfg = (struct post_process_config *)ipc_post_process->data;
	bs = ipc_post_process->size;

	//przecieci przez ten config byte after byte co tam siedzi
	comp_cl_info(&comp_post_process, "RAJWA: size of config data is %d", bs);
	/*uint32_t *debug = (void *)0x9e008000;
	int i = 0;
	uint32_t *ptr = (uint32_t *)cfg;
	*debug = 0xFEED0;
	*(debug+i++) = sizeof(uint32_t);

	while (bs > 0) {
		*(debug+i++) = 0xFEED;
		*(debug+i++) = *ptr;
		ptr++;
		bs -= sizeof(uint32_t);
	}*/

	if (bs) {
		if (bs < sizeof(struct post_process_config)) {
			comp_info(dev, "post_process_new() error: wrong size of post processing config");
			return NULL;
		}
		ret = memcpy_s(&cd->pp_config, sizeof(cd->pp_config), cfg,
			       sizeof(struct post_process_config));
		assert(!ret);
		comp_cl_info(&comp_post_process, "RAJWA: sample rate: %d width %d, channels %d",
			cd->pp_config.sample_rate,
			cd->pp_config.sample_width,
			cd->pp_config.channels);

		ret = validate_config(&cd->pp_config);
		if (ret) {
			comp_err(dev, "post_process_new(): error: validation of pp config failed");
			return NULL;
		}

		/* Pass config further to the library */
		/* move this part to prepare - so in real timne use case it will work like this
		somebody creates the component and than loads the config as they wish */
		lib_cfg = (char *)cfg + sizeof(struct post_process_config);
		lib_cfg_size = bs - sizeof(struct post_process_config);
		comp_cl_info(&comp_post_process, "RAJWA: size of lib_cfg is %d, first byte %d",
			      lib_cfg_size, *((char *)lib_cfg));
		ret = pp_lib_load_config(dev, lib_cfg, lib_cfg_size, PP_CFG_SETUP);
		if (ret) {
			comp_err(dev, "post_process_new(): error %x: failed to set config for lib",
				 ret);

		} else {
			comp_info(dev, "post_process_new() lib conffig set successfully");
		}

	} else {
		comp_err(dev, "post_process_new(): no configuration available");
		return NULL;
	}

	dev->state = COMP_STATE_READY;
        cd->state = PP_STATE_CREATED;

	comp_cl_info(&comp_post_process, "post_process_new(): component created successfully");

	return dev;
}

static int post_process_prepare(struct comp_dev *dev)
{
	int ret;
	struct comp_data *cd = comp_get_drvdata(dev);
	bool lib_state;

	comp_info(dev, "post_process_prepare() start");

	/* Init sink & source buffers */
	cd->pp_sink = list_first_item(&dev->bsink_list, struct comp_buffer,
				      source_list);
        cd->pp_source = list_first_item(&dev->bsource_list, struct comp_buffer,
                                        sink_list);

        if (!cd->pp_source) {
                comp_err(dev, "post_process_prepare() erro: source buffer not found");
                return -EINVAL;
        } else if (!cd->pp_sink) {
                comp_err(dev, "post_process_prepare() erro: sink buffer not found");
                return -EINVAL;
        }

	ret = comp_set_state(dev, COMP_TRIGGER_PREPARE);
	if (ret < 0)
		return ret;

	if (ret == COMP_STATUS_STATE_ALREADY_SET)
		return PPL_STATUS_PATH_STOP;

	/* TODO: check if paramerers has changed, and if so,
	 * reset the library and start over.
	 */
	ret = pp_get_lib_state(&lib_state);
	if (ret) {
		comp_err(dev, "post_process_prepare() error %x: could not get lib state",
			 ret);
		return -EIO;
	} else if (lib_state >= PP_LIB_PREPARED) {
		comp_info(dev, "post_process_prepare() lib already prepared");
		goto done;
	}

	/* Prepare post processing library */
	ret = pp_lib_prepare(dev, &cd->sdata);
	if (ret) {
		comp_err(dev, "post_process_prepare() error %x: lib prepare failed",
			 ret);

		return -EIO;
	} else {
		comp_info(dev, "post_process_prepare() lib prepared successfully");
	}

        /* Do we have runtime config available? */
        if (cd->lib_r_cfg_avail) {
                ret = pp_codec_apply_config(dev, PP_CFG_RUNTIME);
                if (ret) {
                        comp_err(dev, "post_process_prepare() error %x: lib config apply failed",
                                 ret);
                        return -EIO;
                } else {
                        comp_info(dev, "post_process_prepare() lib runtime config applied successfully");
                }
        }
done:
	comp_info(dev, "post_process_prepare() done");
        cd->state = PP_STATE_PREPARED;

	return 0;
}

static void post_process_free(struct comp_dev *dev)
{
	struct comp_data *cd = comp_get_drvdata(dev);

	comp_cl_info(&comp_post_process, "post_process_free(): start");

	rfree(cd);
	rfree(dev);
	//TODO: call lib API to free its resources

	comp_cl_info(&comp_post_process, "post_process_free(): component memory freed");

}

static int post_process_trigger(struct comp_dev *dev, int cmd)
{
	comp_cl_info(&comp_post_process, "post_process_trigger(): component got trigger cmd %x",
		     cmd);

	//TODO: ask lib if pp parameters has been aplied and if not log it!
        //likely here change detect COMP_TRIGGER_START cmd and change state to PP_STATE_RUN
	return comp_set_state(dev, cmd);
}

static int post_process_reset(struct comp_dev *dev)
{
        struct comp_data *cd = comp_get_drvdata(dev);

	comp_cl_info(&comp_post_process, "post_process_reset(): resetting");

        cd->state = PP_STATE_CREATED;

	return comp_set_state(dev, COMP_TRIGGER_RESET);
}


// static void post_process_copy_to_sink(struct audio_stream *sink,
// 			       const struct audio_stream *source,
// 			       size_t size)
// {
//     void *dst;
//     void *src;
//     size_t i;
//     size_t j = 0;
//     size_t channel;
//     size_t channels = source->channels;
//     size_t sample_width = source->frame_fmt == SOF_IPC_FRAME_S16_LE ?
// 			  16 : 32;
//     size_t frames = size / (sample_width / 8 * channels);

//     for (i = 0; i < frames; i++) {
// 	for (channel = 0; channel < channels; channel++) {
// 	    switch (sample_width) {
// #if CONFIG_FORMAT_S16LE
// 	    case 16:
// 		dst = audio_stream_write_frag_s16(sink, j);
// 		src = audio_stream_read_frag_s16(source, j);
// 		*((int16_t *)dst) = *((int16_t *)src);
// 		break;
// #endif /* CONFIG_FORMAT_S16LE
// #if CONFIG_FORMAT_S24LE || CONFIG_FORMAT_S32LE
// 	    case 24:
// 		/* FALLTHROUGH */
// 	    case 32:
// 		dst = audio_stream_write_frag_s32(sink, j);
// 		src = audio_stream_read_frag_s32(source, j);
// 		*((int32_t *)dst) = *((int32_t *)src);
// 		break;
// #endif /* CONFIG_FORMAT_S24LE || CONFIG_FORMAT_S32LE*/
// 	    default:
// 		comp_cl_info(&comp_post_process, "KPB: An attempt to copy "
// 			"not supported format!");
// 		return;
// 	    }
// 	    j++;
// 	}
//     }
// }*/

static void post_process_copy_to_lib(const struct audio_stream *source,
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
				comp_cl_info(&comp_post_process, "post_process_copy_to_lib(): An attempt to copy not supported format!");
				return;
			}
			j++;
		}
	}
}

static void post_process_copy_from_lib_to_sink(void *source, struct audio_stream *sink,
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
				comp_cl_info(&comp_post_process, "post_process_copy_to_lib(): An attempt to copy not supported format!");
				return;
			}
			j++;
		}
	}
}


static inline int read_prid(void)
{
	int reg;
    __asm__ __volatile__("rsr.prid %0\n\t"
            : : "a" (reg));
    return reg;
}

static inline int read_ps(void)
{
	int reg;
    __asm__ __volatile__("rsr.ps %0\n\t"
            : : "a" (reg));
    return reg;
}

static int post_process_copy(struct comp_dev *dev)
{
	int ret;
	uint32_t copy_bytes, bytes_to_process, produced, processed = 0;
	struct comp_data *cd = comp_get_drvdata(dev);
	struct comp_buffer *source = cd->pp_source;
	struct comp_buffer *sink = cd->pp_sink;
        uint32_t lib_buff_size = cd->sdata.lib_in_buff_size;
	/*TODO: recognize if error was FATAL or non fatal end react accoringly */

        comp_dbg(dev, "RAJWA: post_process_copy() start. Core %d PS %x ",
        	read_prid(), read_ps());

        bytes_to_process = MIN(sink->stream.free, source->stream.avail);
	copy_bytes = MIN(bytes_to_process, lib_buff_size);

	while (bytes_to_process) {
		if (bytes_to_process < lib_buff_size) {
			comp_dbg(dev, "post_process_copy(): skipping processing as we don't have enough data. Only %d bytes available in source buffer",
			        bytes_to_process);
			break;
		}

		/* Fill lib buffer completely. NOTE! If you don't fill whole buffer
		 * the lib will not process the buffer!
		 */
		post_process_copy_to_lib(&source->stream,
					 cd->sdata.lib_in_buff, lib_buff_size);

		ret = pp_lib_process_data(dev, lib_buff_size, &produced);
		if (ret) {
			comp_err(dev, "post_process_copy() error %x: lib processing failed",
				 ret);
		} else if (produced == 0) {
			/* skipping as lib has not produced anything */
                        comp_err(dev, "post_process_copy() error %x: lib hasn't processed anything",
                                 ret);
			ret = 0;
			goto end;
		}

                post_process_copy_from_lib_to_sink(cd->sdata.lib_out_buff,
                                                   &sink->stream, produced);

		bytes_to_process -= produced;
		processed += produced;
	}
	if (!processed) {
                comp_err(dev, "post_process_copy() error: failed to process anything in this call!");
	       goto end;
        }

       comp_update_buffer_produce(sink, processed);
       comp_update_buffer_consume(source, processed);

end:
	return 0;
}


static int pp_set_config(struct comp_dev *dev,
			 struct sof_ipc_ctrl_data *cdata) {
	//TODO add load of setup config
	/* At this point the setup config is small enough so it fits
	in single ipc therfore will be send in .new()
	*/
	return 0;
}

static int pp_set_runtime_params(struct comp_dev *dev,
			      struct sof_ipc_ctrl_data *cdata) {
	int ret;
	char *dst, *src;
	static uint32_t size;
	uint32_t offset, lib_max_blob_size;
        struct comp_data *cd = comp_get_drvdata(dev);

	/* Stage 1 load whole config locally */
	/* Check that there is no work-in-progress previous request */
	if (cd->pp_lib_runtime_config && cdata->msg_index == 0) {
		comp_err(dev, "pp_set_runtime_params() error: busy with previous request");
		return -EBUSY;
	}

	comp_info(dev, "pp_set_runtime_params(): num_of_elem %d, elem remain %d msg_index %u",
		  cdata->num_elems, cdata->elems_remaining, cdata->msg_index);

	ret = pp_lib_get_max_blob_size(&lib_max_blob_size);
	if (ret) {
		comp_err(dev, "pp_set_runtime_params() error: could not get blob size limit from the lib");
		goto end;
	}
	if (cdata->num_elems + cdata->elems_remaining > lib_max_blob_size)
	{
		comp_err(dev, "pp_set_runtime_params() error: blob size is too big!");
		ret = -EINVAL;
		goto end;
	}

	if (cdata->msg_index == 0) {
		/* Allocate buffer for new params */
		size = cdata->num_elems + cdata->elems_remaining;
		cd->pp_lib_runtime_config = rballoc(0, SOF_MEM_CAPS_RAM, size);

		if (!cd->pp_lib_runtime_config) {
			comp_err(dev, "pp_set_runtime_params(): space allocation for new params failed");
			ret = -EINVAL;
			goto end;
		}
		memset(cd->pp_lib_runtime_config, 0, size);
	}

	offset = size - (cdata->num_elems + cdata->elems_remaining);
	dst = (char *)cd->pp_lib_runtime_config + offset;
	src = (char *)cdata->data->data;

	ret = memcpy_s(dst,
		       size - offset,
		       src, cdata->num_elems);

	assert(!ret);

	if (cdata->elems_remaining == 0) {
		/* Config has been copied now we can load & apply it
		 * depending on lib status.
		 */
		ret = pp_lib_load_config(dev, cd->pp_lib_runtime_config, size,
					 PP_CFG_RUNTIME);
		if (ret) {
			comp_err(dev, "pp_set_runtime_params() error %x: lib params load failed",
				 ret);
			goto end;
        	}
		if (cd->state >= PP_STATE_PREPARED) {
			/* Post processing is already prepared so we can apply runtime
			 * config right away.
			 */
			ret = pp_codec_apply_config(dev, PP_CFG_RUNTIME);
			if (ret) {
				comp_err(dev, "post_process_ctrl_set_data() error %x: lib config apply failed",
					 ret);
				goto end;
			}
		} else {
			cd->lib_r_cfg_avail = true;
		}


	}

end:
	if (cd->pp_lib_runtime_config)
		rfree(cd->pp_lib_runtime_config);
	cd->pp_lib_runtime_config = NULL;
	return ret;
}

static int pp_set_binary_data(struct comp_dev *dev,
			      struct sof_ipc_ctrl_data *cdata) {
	int ret;

	 comp_info(dev, "pp_set_binary_data() start, data type %d",
	 	   cdata->data->type);

	switch (cdata->data->type) {
		/* TODO: use enum pp_cfg_type instead of defines */
	case PP_SETUP_CONFIG:
		ret = pp_set_config(dev, cdata);
		break;
	case PP_RUNTIME_PARAMS:
		ret = pp_set_runtime_params(dev, cdata);
		break;
	default:
		comp_err(dev, "pp_set_binary_data() error: unknown binary data type");
		ret = -EIO;
		break;
	}

	return ret;
}


static int post_process_ctrl_set_data(struct comp_dev *dev,
                                      struct sof_ipc_ctrl_data *cdata) {
	int ret;

	struct comp_data *cd = comp_get_drvdata(dev);

        comp_info(dev, "post_process_ctrl_set_data() start, state %d, cmd %d",
        	  cd->state, cdata->cmd);

	/* Check version from ABI header */
	if (SOF_ABI_VERSION_INCOMPATIBLE(SOF_ABI_VERSION, cdata->data->abi)) {
		comp_err(dev, "post_process_ctrl_set_data(): ABI mismatch");
		return -EINVAL;
	}

	switch (cdata->cmd) {
	case SOF_CTRL_CMD_ENUM:
		//TODO
		ret = -EINVAL;
		break;
	case SOF_CTRL_CMD_BINARY:
		ret = pp_set_binary_data(dev, cdata);
		break;
	default:
		comp_err(dev, "post_process_ctrl_set_data error: unknown set data command");
		ret = -EINVAL;
		break;
	}
	return ret;
}

/* used to pass standard and bespoke commands (with data) to component */
static int post_process_cmd(struct comp_dev *dev, int cmd, void *data,
			    int max_data_size)
{
	struct sof_ipc_ctrl_data *cdata = data;

	comp_info(dev, "post_process_cmd() %d start", cmd);

	switch (cmd) {
	case COMP_CMD_SET_DATA:
		return post_process_ctrl_set_data(dev, cdata);
	case COMP_CMD_GET_DATA:
		//TODO
		return -EINVAL;
	default:
		comp_err(dev, "post_process_cmd() error: unknown command");
		return -EINVAL;
	}
}

static const struct comp_driver comp_post_process = {
	.type = SOF_COMP_POST_PROCESS,
	.uid = SOF_RT_UUID(pp_uuid),
	.tctx = &pp_tr,
	.ops = {
		.create = post_process_new,
		.free = post_process_free,
		.params = NULL,
		.cmd = post_process_cmd,
		.trigger = post_process_trigger,
		.prepare = post_process_prepare,
		.reset = post_process_reset,
		.copy = post_process_copy,
	},
};


static SHARED_DATA struct comp_driver_info comp_post_process_info = {
	.drv = &comp_post_process,
};

UT_STATIC void sys_comp_post_process_init(void)
{
	comp_register(platform_shared_get(&comp_post_process_info,
					  sizeof(comp_post_process_info)));
}

DECLARE_MODULE(sys_comp_post_process_init);
