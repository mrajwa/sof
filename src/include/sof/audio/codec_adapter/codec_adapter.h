/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2020 Intel Corporation. All rights reserved.
 *
 * Author: Marcin Rajwa <marcin.rajwa@linux.intel.com>
 */

#ifndef __SOF_AUDIO_CODEC_ADAPTER__
#define __SOF_AUDIO_CODEC_ADAPTER__

#include <sof/audio/codec_adapter/codec/generic.h>

/*****************************************************************************/
/* Codec adapter data structures							     */
/*****************************************************************************/


static  inline int validate_setup_config(struct ca_config *cfg)
{
	/* TODO: custom validation of codec_adapter setup parameters */
	return 0;
}

#endif /* __SOF_AUDIO_CODEC_ADAPTER__ */
