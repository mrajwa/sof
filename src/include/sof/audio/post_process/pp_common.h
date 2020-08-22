/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 * Author: Marcin Rajwa <marcin.rajwa@linux.intel.com>
 */

#include <sof/audio/component.h>

/*****************************************************************************/
/* Post processing shared data						     */
/*****************************************************************************/

enum pp_lib_state {
	PP_LIB_DISABLED,
	PP_LIB_PREPARED
};

enum pp_cfg_type {
	PP_CFG_SETUP,
	PP_CFG_RUNTIME
};

struct post_process_config {
	uint32_t codec_id;
	uint32_t reserved;
	uint32_t sample_rate;
	uint32_t sample_width;
	uint32_t channels;
};

struct post_process_shared_data {
	uint32_t lib_in_buff_size;
	void *lib_in_buff;
	void *lib_out_buff;
};

/*****************************************************************************/
/* Post processing library public interfaces						     */
/*****************************************************************************/
int pp_get_lib_state(bool *state);
int pp_init_lib(struct comp_dev *dev, uint32_t codec_id);
int pp_lib_load_config(struct comp_dev *dev, void *cfg, size_t size,
		       enum pp_cfg_type type);
int pp_lib_prepare(struct comp_dev *dev,
		   struct post_process_shared_data *sdata);
int pp_lib_process_data(struct comp_dev *dev, size_t avail, size_t *produced);
int pp_codec_apply_config(struct comp_dev *dev,enum pp_cfg_type type);
int pp_lib_get_max_blob_size(uint32_t *size);
