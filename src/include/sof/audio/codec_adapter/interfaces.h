/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2020 Intel Corporation. All rights reserved.
 *
 *
 * \file interfaces.h
 * \brief Description of supported codecs
 * \author Marcin Rajwa <marcin.rajwa@linux.intel.com>
 *
 */
#ifndef __SOF_AUDIO_CODEC_INTERFACES__
#define __SOF_AUDIO_CODEC_INTERFACES__

#if CONFIG_CADENCE_CODEC
#include <sof/audio/codec_adapter/codec/cadence.h>
#endif

/*****************************************************************************/
/* Linked codecs interfaces						     */
/*****************************************************************************/
static struct codec_interface interfaces[] = {
	{
		.id = 0xCADE01, /**< Cadence interface */
		.init  = cadence_codec_init,
		.prepare = cadence_codec_prepare,
	},
};

#endif /* __SOF_AUDIO_CODEC_INTERFACES__ */
