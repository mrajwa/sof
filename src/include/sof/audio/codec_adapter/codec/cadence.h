/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 * Author: Marcin Rajwa <marcin.rajwa@linux.intel.com>
 */

#ifndef __SOF_AUDIO_CADENCE_CODEC__
#define __SOF_AUDIO_CADENCE_CODEC__

#define LIB_NAME_MAX_LEN 30
/*****************************************************************************/
/* Codec private data types						     */
/*****************************************************************************/
struct cadence_codec_data {
	char name[LIB_NAME_MAX_LEN];
	void *self;
	void *mem_tabs;
};

/*****************************************************************************/
/* Codec interfaces							     */
/*****************************************************************************/
int cadence_codec_init(struct comp_dev *dev);

#endif /* __SOF_AUDIO_CADENCE_CODEC__ */
