// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2020 Intel Corporation. All rights reserved.
//
// Author: Marcin Rajwa <marcin.rajwa@linux.intel.com>

/*
 * \file cadence.c
 * \brief Cadence Codec API
 * \author Marcin Rajwa <marcin.rajwa@linux.intel.com>
 *
 */

#include <sof/audio/codec_adapter/codec/generic.h>
#include <sof/audio/codec_adapter/codec/cadence.h>

int cadence_codec_init(struct comp_dev *dev)
{
	struct codec_data *codec = comp_get_codec(dev);
	struct cadence_codec_data *cd = NULL;

	comp_dbg(dev, "cadence_codec_init() start");

	cd = codec_allocate_memory(dev, sizeof(struct cadence_codec_data), 0);
	if (!cd) {
		comp_err(dev, "cadence_codec_init(): failed to allocate memory for cadence codec data");
		return -ENOMEM;
	}

	codec->private = cd;
	cd->self = NULL;
	cd->mem_tabs = NULL;

	comp_dbg(dev, "cadence_codec_init() done");
	return 0;
}
