/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2020 Intel Corporation. All rights reserved.
 *
 * Author: Marcin Rajwa <marcin.rajwa@linux.intel.com>
 */

#ifndef __SOF_AUDIO_CODEC_ADAPTER__
#define __SOF_AUDIO_CODEC_ADAPTER__

enum ca_state {
	PP_STATE_DISABLED = 0,
	PP_STATE_CREATED,
	PP_STATE_PREPARED,
	PP_STATE_RUN,
};

/* codec_adapter private, runtime data */
struct comp_data {
	enum ca_state state; /* current state of codec_adapter */
};

#endif /* __SOF_AUDIO_CODEC_ADAPTER__ */
