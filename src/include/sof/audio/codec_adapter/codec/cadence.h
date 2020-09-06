/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 * Author: Marcin Rajwa <marcin.rajwa@linux.intel.com>
 */

#ifndef __SOF_AUDIO_CADENCE_CODEC__
#define __SOF_AUDIO_CADENCE_CODEC__

#include <sof/audio/cadence/xa_type_def.h>
#include <sof/audio/cadence/xa_apicmd_standards.h>
#include <sof/audio/cadence/xa_error_standards.h>

/*****************************************************************************/
/* Codec API interface							     */
/*****************************************************************************/
extern xa_codec_func_t xa_dap_vlldp;
#define API xa_dap_vlldp
#define API_CALL(cd, cmd, sub_cmd, value, ret) \
		do { \
			ret = API((cd)->self, \
				  (cmd), \
				  (sub_cmd), \
				  (value)); \
		} while (0);
/*****************************************************************************/
/* Codec private data types						     */
/*****************************************************************************/
#define LIB_NAME_MAX_LEN 30
#define LIB_NO_ERROR XA_NO_ERROR

enum cadence_codec_state {
	CADENCE_CODEC_DISABLED,
	CADENCE_CODEC_PREPARED,
	CADENCE_CODEC_RUNNING
};

struct cadence_codec_data {
	enum cadence_codec_state state;
	char name[LIB_NAME_MAX_LEN];
	void *self;
	void *mem_tabs;
};


int cadence_codec_init(struct comp_dev *dev);

#endif /* __SOF_AUDIO_CADENCE_CODEC__ */
