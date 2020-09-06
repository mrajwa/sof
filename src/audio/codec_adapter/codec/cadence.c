/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2020 Intel Corporation. All rights reserved.
 *
 *
 * \file cadence.c
 * \brief Cadence Codec API
 * \author Marcin Rajwa <marcin.rajwa@linux.intel.com>
 *
 */

#include <sof/audio/codec_adapter/codec/generic.h>
#include <sof/audio/codec_adapter/codec/cadence.h>


int cadence_codec_init(struct comp_dev *dev) {
	int ret;
	struct codec_data *codec = comp_get_codec_data(dev);
	struct cadence_codec_data *cd = codec->private;

	comp_info(dev, "cadence_codec_init() start");

	API_CALL(cd, XA_API_CMD_GET_LIB_ID_STRINGS,
		 XA_CMD_TYPE_LIB_NAME, cd->name, ret);
	if (ret != LIB_NO_ERROR) {
		comp_err(dev, "hifi_codec_init() error %x: failed to get lib name",
			 ret);
		goto out;
	}

	comp_info(dev, "cadence_codec_init() done");
out:
	return ret;
}
