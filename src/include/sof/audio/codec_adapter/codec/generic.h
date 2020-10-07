/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2020 Intel Corporation. All rights reserved.
 *
 *
 * \file generic.h
 * \brief Generic Codec API header file
 * \author Marcin Rajwa <marcin.rajwa@linux.intel.com>
 *
 */

#ifndef __SOF_AUDIO_CODEC_GENERIC__
#define __SOF_AUDIO_CODEC_GENERIC__

#include <sof/audio/component.h>

#define comp_get_codec(d) (&(((struct comp_data *)(d->priv_data))->codec))

/*****************************************************************************/
/* Codec generic data types						     */
/*****************************************************************************/
/** codec specific interfaces */
struct codec_interface {
	uint32_t id;
	int (*init)(struct comp_dev *dev);
	int (*prepare)(struct comp_dev *dev);
	int (*process)(struct comp_dev *dev);
	int (*apply_config)(struct comp_dev *dev);
	int (*reset)(struct comp_dev *dev);
	int (*free)(struct comp_dev *dev);
};

/* CODEC configuration types */
enum codec_cfg_type {
	CODEC_CFG_SETUP,
	CODEC_CFG_RUNTIME
};

/** codec_adapter states */
enum ca_state {
	PP_STATE_DISABLED = 0,
	PP_STATE_CREATED,
	PP_STATE_PREPARED,
	PP_STATE_RUN,
};

/** codec states */
enum codec_state {
	CODEC_DISABLED,
	CODEC_INITIALIZED,
	CODEC_PREPARED,
	CODEC_RUNNING
};

/** codec adapter setup config parameters */
struct ca_config {
	uint32_t codec_id;
	uint32_t reserved;
	uint32_t sample_rate;
	uint32_t sample_width;
	uint32_t channels;
};

/** codec TLV parameters container - used for both config types */
struct codec_param {
	uint32_t id;
	uint32_t size;
	int32_t data[];
};

/** codec config container - used for both config types */
struct codec_config {
	size_t size;
	bool avail;
	void *data; /* tlv config */
};

/** codec memory block - used for every memory allocated by codec */
struct codec_memory {
	void *ptr;
	struct list_item mem_list; /**< memory allocated by codec */
};

/** processing data shared between particular codec & codec_adapter */
struct codec_processing_data {
	uint32_t in_buff_size;
	uint32_t out_buff_size;
	uint32_t avail;
	uint32_t produced;
	void *in_buff;
	void *out_buff;
};

/** private, runtime codec data */
struct codec_data {
	enum codec_state state;
	struct codec_config s_cfg; /**< setup config */
	struct codec_config r_cfg; /**< runtime config */
	struct codec_interface *ops;
	void *private; /**< self object, memory tables etc here */
	struct codec_memory memory;
	struct codec_processing_data cpd; /**< shared data comp <-> codec */
};

/* codec_adapter private, runtime data */
struct comp_data {
	enum ca_state state; /**< current state of codec_adapter */
	struct ca_config ca_config;
	struct codec_data codec; /**< codec private data */
	struct comp_buffer *ca_sink;
	struct comp_buffer *ca_source;
	void *runtime_params;
};

#endif /* __SOF_AUDIO_CODEC_GENERIC__ */
