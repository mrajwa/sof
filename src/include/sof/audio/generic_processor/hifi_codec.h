/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 * Author: Marcin Rajwa <marcin.rajwa@linux.intel.com>
 */

#ifndef __SOF_AUDIO_HIFI_CODEC_API__
#define __SOF_AUDIO_HIFI_CODEC_API__

#include <sof/audio/generic_processor/gp_common.h>
#include <sof/audio/generic_processor/xa_apicmd_standards.h>
#include <sof/audio/generic_processor/xa_error_standards.h>
#include <sof/audio/generic_processor/xa_memory_standards.h>
#include <sof/audio/generic_processor/xa_type_def.h>
#include <sof/audio/generic_processor/xa_type_def.h>

/*****************************************************************************/
/* Generic API interface						     */
/*****************************************************************************/

#define HIFI_LIB_MAX_BLOB_SIZE	4096 /**< Max config size in bytes */
#define LIB_NO_ERROR XA_NO_ERROR
#define MAX_NO_OF_CHANNELS 8
#define LIB_NAME_MAX_LEN 30

#define HIFI_CODEC_API_CALL(cmd, sub_cmd, value) \
	hifi_codec_data.api((hifi_codec_data.self), (cmd), (sub_cmd), (value));\
	if (ret != LIB_NO_ERROR) \
		handle_error(dev, ret);

extern xa_codec_func_t xa_dap_vlldp;

struct hifi_processing_codec {
	char id;
	char *name;
	char *version;
	xa_codec_func_t *api;
};

static struct hifi_processing_codec hifi_codec_lib[] = {
	{
		.id = 0,
		.name = "",
		.version = "",
		.api = xa_dap_vlldp
	},
	{
		.id = 1,
		.name = NULL,
		.version = NULL,
		.api = NULL
	},
};

/*****************************************************************************/
/* Lib data structures							     */
/*****************************************************************************/
struct hifi_codec_param {
	uint32_t id;
	uint32_t size;
	int32_t data[];
};

struct hifi_codec_config {
	size_t size;
	bool avail;
	void *data; /* tlv config */
};

struct hifi_codec_data {
	void *self;
	xa_codec_func_t *api;
	enum hifi_codec_state state;
	char name[LIB_NAME_MAX_LEN];
	void *mem_tabs;
	//TODO: add linked list of memory blocks for mem_tabs
	void *in_buff;
	void *out_buff;
	size_t in_buff_size;
	size_t out_buff_size;
	struct hifi_codec_config s_cfg;
	struct hifi_codec_config r_cfg;
};

#endif /* __SOF_AUDIO_HIFI_CODEC_API__ */
