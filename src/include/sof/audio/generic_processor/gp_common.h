/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 * Author: Marcin Rajwa <marcin.rajwa@linux.intel.com>
 */
#ifndef __SOF_AUDIO_GP_COMMON__
#define __SOF_AUDIO_GP_COMMON__
#include <sof/audio/component.h>

/*****************************************************************************/
/* Post processing shared data						     */
/*****************************************************************************/

enum hifi_codec_state {
	HIFI_ADAPTER_DISABLED,
	HIFI_ADAPTER_PREPARED
};

enum hifi_codec_cfg_type {
	PP_CFG_SETUP,
	PP_CFG_RUNTIME
};

struct generic_processor_config {
	uint32_t codec_id;
	uint32_t reserved;
	uint32_t sample_rate;
	uint32_t sample_width;
	uint32_t channels;
};

struct generic_processor_shared_data {
	uint32_t lib_in_buff_size;
	void *lib_in_buff;
	void *lib_out_buff;
};

/*****************************************************************************/
/* Post processing library public interfaces						     */
/*****************************************************************************/
int hifi_codec_init(struct comp_dev *dev, uint32_t codec_id);
int hifi_codec_get_state(bool *state);
int hifi_codec_load_config(struct comp_dev *dev, void *cfg, size_t size,
		       enum hifi_codec_cfg_type type);
int hifi_codec_prepare(struct comp_dev *dev,
		   struct generic_processor_shared_data *sdata);
int hifi_codec_process_data(struct comp_dev *dev, size_t avail, size_t *produced);
int hifi_codec_apply_config(struct comp_dev *dev,enum hifi_codec_cfg_type type);
int hifi_codec_get_max_blob_size(uint32_t *size);

#endif /* __SOF_AUDIO_GP_COMMON__ */
