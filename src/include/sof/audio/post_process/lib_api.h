/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 * Author: Marcin Rajwa <marcin.rajwa@linux.intel.com>
 */

#ifndef __SOF_AUDIO_PP_LIB_API__
#define __SOF_AUDIO_PP_LIB_API__

#include <sof/audio/post_process/pp_common.h>
#include <sof/audio/post_process/xa_apicmd_standards.h>
#include <sof/audio/post_process/xa_dap_vlldp_api.h>
#include <sof/audio/post_process/xa_error_standards.h>
#include <sof/audio/post_process/xa_memory_standards.h>
#include <sof/audio/post_process/xa_type_def.h>
#include <sof/audio/post_process/xa_type_def.h>

/*****************************************************************************/
/* Generic API interface						     */
/*****************************************************************************/

#define PP_LIB_MAX_BLOB_SIZE	4096 /**< Max config size in bytes */
#define LIB_NO_ERROR XA_NO_ERROR
#define MAX_NO_OF_CHANNELS 8
#define LIB_NAME_MAX_LEN 30

#define PP_LIB_API_CALL(cmd, sub_cmd, value) \
	xa_dap_vlldp((pp_lib_data.self), (cmd), (sub_cmd), (value));\
	if (ret != LIB_NO_ERROR) \
		handle_error(dev, ret);
#define PP_LIB_API_SET_CONFIG(XA_API_CMD_SET_CONFIG_PARAM, idx, pvalue, context_str) \
	err_code = (*(p_xa_process_api))(p_xa_process_api_obj, (cmd), (idx), (pvalue));\
	lib_handle_error(err_code);
#define PP_LIB_API_GET_CONFIG(XA_API_CMD_GET_CONFIG_PARAM, idx, pvalue, context_str) \
	err_code = (*(p_xa_process_api))(p_xa_process_api_obj, (cmd), (idx), (pvalue));\
	lib_handle_error(err_code);

#define PP_MEM_TYPE_IN_BUFFER	XA_MEMTYPE_INPUT
#define PP_MEM_TYPE_OUT_BUFFER	XA_MEMTYPE_OUTPUT
#define PP_MEM_TYPE_SCRATCH	XA_MEMTYPE_SCRATCH
#define PP_MEM_TYPE_PERSISTENT	XA_MEMTYPE_PERSIST

enum config_apply_mask {
	APPLY_GENERAL_CONFIG = 0,
	APPLY_PREFILTER_CONFIG,
	APPLY_OPTIMIZER_CONFIG,
	APPLY_TRESH_REG_CONFIG,
	APPLY_REG_TUN_CONFIG,
	APPLY_CONFIG_STOP,
};

/*****************************************************************************/
/* Lib data structures							     */
/*****************************************************************************/

struct pp_general_config {
	uint32_t system_gain;
	uint32_t post_gain;
	uint32_t optimizer_enable;
	uint32_t speaker_distortion_mode;
	uint32_t overdrive_regulator;
	uint32_t relaxation_amount;
	uint32_t timbre_regulation;
	uint32_t isolated_bands_regulation[XA_DAP_VLLDP_NUM_BANDS];
};

struct post_process_runtime_config {
	struct pp_general_config general;
	xa_dap_vlldp_prefilter_t prefilter;
	xa_dap_vlldp_optimizer_gains_t optimizer;
	xa_dap_vlldp_regulator_thresholds_t regulator_thresholds;
	xa_dap_vlldp_regulator_tuning_info_t regulator_tuning;
};

struct lib_config {
	uint32_t block_size;
	uint32_t interleaved_mode;
};

struct post_process_setup_config {
	struct post_process_config common;
	struct lib_config specific;
};

struct post_process_lib_data {
	enum pp_lib_state state;
	char name[LIB_NAME_MAX_LEN];
	void *self;
	void *mem_tabs;
	void *in_buff;
	void *out_buff;
	size_t in_buff_size;
	size_t out_buff_size;
	struct post_process_setup_config s_cfg;
	struct post_process_runtime_config r_cfg;
};

#endif /* __SOF_AUDIO_PP_LIB_API__ */
