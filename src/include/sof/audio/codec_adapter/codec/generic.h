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

/*****************************************************************************/
/* Codec generic data types						     */
/*****************************************************************************/
enum codec_cfg_type {
	CODEC_CFG_SETUP,
	CODEC_CFG_RUNTIME
};

enum ca_state {
	PP_STATE_DISABLED = 0,
	PP_STATE_CREATED,
	PP_STATE_PREPARED,
	PP_STATE_RUN,
};

struct ca_config {
	uint32_t codec_id;
	uint32_t reserved;
	uint32_t sample_rate;
	uint32_t sample_width;
	uint32_t channels;
};

struct codec_config {
	size_t size;
	bool avail;
	void *data; /* tlv config */
};

struct codec_data {
	struct codec_config s_cfg; /**< setup config */
	struct codec_config r_cfg; /**< runtime config */
};

/* codec_adapter private, runtime data */
struct comp_data {
	enum ca_state state; /**< current state of codec_adapter */
	struct ca_config ca_config;
	struct codec_data codec; /**< codec private data */
};

/*****************************************************************************/
/* Codec generic interfaces						     */
/*****************************************************************************/
int codec_load_config(struct comp_dev *dev, void *cfg, size_t size,
		      enum codec_cfg_type type);

#endif /* __SOF_AUDIO_CODEC_GENERIC__ */
