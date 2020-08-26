/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2020 Intel Corporation. All rights reserved.
 *
 * Author: Marcin Rajwa <marcin.rajwa@linux.intel.com>
 */

#include <sof/audio/generic_processor/gp_common.h>

#define PP_SETUP_CONFIG		0
#define PP_RUNTIME_PARAMS	1

enum gp_state {
	PP_STATE_DISABLED = 0,
	PP_STATE_CREATED,
	PP_STATE_PREPARED,
	PP_STATE_RUN,
};

struct generic_processor_config {
	uint32_t codec_id;
	uint32_t reserved;
	uint32_t sample_rate;
	uint32_t sample_width;
	uint32_t channels;
};

/* generic_processor private, runtime data */
struct comp_data {
	enum gp_state state;
	struct generic_processor_config gp_config;
	struct generic_processor_shared_data sdata;
	struct comp_buffer *gp_sink;
	struct comp_buffer *gp_source;
	bool codec_r_cfg_avail;
	void *gp_lib_runtime_config;
};

static  inline int validate_config(struct generic_processor_config *cfg)
{
	//TODO: custom validation of post processing paramters
	return 0;
}
