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
	/* Assign interface */
	codec->call = interface;
	/* Now we can proceed with codec specific initialization */
	ret = codec->call->init(dev);
	if (ret) {
		comp_err(dev, "codec_init() error %d: codec specific init failed, codec_id %x",
			 ret, codec_id);
		goto out;
	}

	comp_info(dev, "codec_init() done");
	codec->state = CODEC_INITIALIZED;
out:
	return ret;
}

void *codec_allocate_memory(struct comp_dev *dev, uint32_t size,
			    uint32_t alignment)
{
	struct codec_memory *container;
	void *ptr;
	struct comp_data *cd = comp_get_drvdata(dev);

	if (!size) {
		comp_err(dev, "codec_allocate_memory: requested allocation of 0 bytes.");
		return NULL;
	}

	/* Allocate memory container */
	container = rzalloc(SOF_MEM_ZONE_RUNTIME, 0, SOF_MEM_CAPS_RAM,
			    sizeof(struct codec_memory));
	if (!container) {
		comp_err(dev, "codec_allocate_memory: failed to allocate memory container.");
		return NULL;
	}

	/* Allocate memory for codec */
	if (alignment)
		ptr = rballoc_align(0, SOF_MEM_CAPS_RAM, size, alignment);
	else
		ptr = rballoc(0, SOF_MEM_CAPS_RAM, size);

	if (!ptr) {
		comp_err(dev, "codec_allocate_memory: failed to allocate memory for codec %x.",
			 cd->ca_config.codec_id);
		return NULL;
	}
	/* Store reference to allocated memory */
	container->ptr = ptr;
	if (!cd->codec.memory) {
		cd->codec.memory = container;
		container->prev = NULL;
		container->next = NULL;
	} else {
		container->prev = cd->codec.memory;
		container->next = NULL;
		cd->codec.memory->next = container;
		cd->codec.memory = container;
	}

	return ptr;
}

int codec_free_memory(struct comp_dev *dev, void *ptr)
{
	struct comp_data *cd = comp_get_drvdata(dev);
	struct codec_memory *mem = cd->codec.memory;
	struct codec_memory *_mem;

	if (!ptr) {
		comp_err(dev, "codec_free_memory: error: NULL pointer passed.");
		return -EINVAL;
	}
	/* Find which container keeps this memory */
	do {
		if (mem->ptr == ptr) {
			rfree(ptr);
			mem->ptr = NULL;
			if (mem->prev && mem->next) {
				mem->prev->next = mem->next;
				mem->next->prev = mem->prev;
				_mem = mem->prev;
				rfree(mem);
				mem = _mem;
			} else if (mem->prev) {
				mem->prev->next = NULL;
				cd->codec.memory = mem->prev;
				rfree(mem);
			} else {
				rfree(mem);
				cd->codec.memory = NULL;
			}

			return 0;
		}
		mem = mem->prev;
	} while (mem);

	comp_err(dev, "codec_free_memory: error: could not find memory pointed by %p",
		 (uint32_t)ptr);

	return -EINVAL;
}

static int validate_config(struct codec_config *cfg)
{
	//TODO: validation of codec specifig setup config
	return 0;
}

int codec_prepare(struct comp_dev *dev)
{
	int ret;
	struct comp_data *cd = comp_get_drvdata(dev);
	uint32_t codec_id = cd->ca_config.codec_id;
	struct codec_data *codec = &cd->codec;

	comp_dbg(dev, "codec_prepare() start");

	if (cd->codec.state == CODEC_PREPARED)
		return 0;
	if (cd->codec.state < CODEC_INITIALIZED)
		return -EPERM;

	ret = codec->call->prepare(dev);
	if (ret) {
		comp_err(dev, "codec_prepare() error %d: codec specific prepare failed, codec_id %x",
			 ret, codec_id);
		goto end;
	}

	codec->s_cfg.avail = false;
	codec->r_cfg.avail = false;
	codec->r_cfg.data = NULL;
	if (codec->r_cfg.data)
		rfree(codec->r_cfg.data);

	codec->state = CODEC_PREPARED;
	comp_dbg(dev, "codec_prepare() done");
end:
	return ret;
}

int codec_process(struct comp_dev *dev)
{
	int ret;
	struct comp_data *cd = comp_get_drvdata(dev);
	uint32_t codec_id = cd->ca_config.codec_id;
	struct codec_data *codec = &cd->codec;

	comp_dbg(dev, "codec_process() start");

	if (cd->codec.state < CODEC_PREPARED) {
		comp_err(dev, "codec_prepare(): wrong state of codec %x, state %d",
			 cd->ca_config.codec_id, codec->state);
		return -EPERM;
	}

	ret = codec->call->process(dev);
	if (ret) {
		comp_err(dev, "codec_prepare() error %d: codec process failed for codec_id %x",
			 ret, codec_id);
		goto out;
	}

	comp_dbg(dev, "codec_process() done");
out:
	return ret;
}

int codec_apply_runtime_config(struct comp_dev *dev)
{
	int ret;
	struct comp_data *cd = comp_get_drvdata(dev);
	uint32_t codec_id = cd->ca_config.codec_id;
	struct codec_data *codec = &cd->codec;

	comp_dbg(dev, "codec_apply_config() start");

	if (cd->codec.state < CODEC_PREPARED)
		return -EPERM;

	if (cd->codec.state < CODEC_PREPARED) {
		comp_err(dev, "codec_prepare() wrong state of codec %x, state %d",
			 cd->ca_config.codec_id, codec->state);
		return -EPERM;
	}

	ret = codec->call->apply_config(dev);
	if (ret) {
		comp_err(dev, "codec_apply_config() error %d: codec process failed for codec_id %x",
			 ret, codec_id);
		goto out;
	}

	cd->codec.r_cfg.avail = false;
	rfree(codec->r_cfg.data);
	codec->r_cfg.data = NULL;

	comp_dbg(dev, "codec_apply_config() end");
out:
	return ret;
}
