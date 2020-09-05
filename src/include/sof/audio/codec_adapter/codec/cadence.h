/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 * Author: Marcin Rajwa <marcin.rajwa@linux.intel.com>
 */

#ifndef __SOF_AUDIO_CADENCE_CODEC__
#define __SOF_AUDIO_CADENCE_CODEC__

//api_obj = xa_dap_vlldp;
/*****************************************************************************/
/* Codec API interface							     */
/*****************************************************************************/
/*#define CADENCE_CODEC_API_CALL(cd, sub_cmd, value, ret) \
				ret = api_obj((cd.self), \
				  (cmd), (sub_cmd), (value));*/

int cadence_codec_init(struct comp_dev *dev);

#endif /* __SOF_AUDIO_CADENCE_CODEC__ */
