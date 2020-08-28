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
/* Codec API interface							     */
/*****************************************************************************/
#if CONFIG_CADENCE_CODEC
#define HIFI_CODEC_API_CALL(cmd, sub_cmd, value, ret) \
	ret = hifi_codec_data.api((hifi_codec_data.self), \
				  (cmd), (sub_cmd), (value));
#endif

struct processing_codec {
	char id;
	char *name;
	char *version;
	void *api;
};

static struct processing_codec codec_lib[] = {
	{
		.id = 0,
		.name = NULL,
		.version = NULL,
		.api = NULL
	},
	{
		.id = 1,
		.name = NULL,
		.version = NULL,
		.api = NULL
	},
};

/*****************************************************************************/
/* Codec generic data types						     */
/*****************************************************************************/
enum codec_cfg_type {
	CODEC_CFG_SETUP,
	PP_CFG_RUNTIME
};

struct codec_config {
	size_t size;
	bool avail;
	void *data; /* tlv config */
};

struct codec_data {
	void *self;
	struct codec_config s_cfg;
	struct codec_config r_cfg;
};

/*****************************************************************************/
/* Codec generic interfaces						     */
/*****************************************************************************/
int codec_load_config(struct comp_dev *dev, void *cfg, size_t size,
			   enum codec_cfg_type type);
int codec_init(struct comp_dev *dev, uint32_t codec_id);
#endif /* __SOF_AUDIO_CODEC_GENERIC__ */
