/*
* The components of this software package from Cadence Design Systems,
* Inc. are subject to the licenses below. By using the software package,
* you agree to the legal terms of each license.
*
* Copyright (c) 1999-2018 Cadence Design Systems, Inc.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef __XA_DAP_VLLDP_API_H__
#define __XA_DAP_VLLDP_API_H__

#define XA_CODEC_DAP_VLLDP 3

/* codec specific configuration parameters */
enum xa_config_param_dap_vlldp
{
    XA_DAP_VLLDP_CONFIG_PARAM_BLOCK_SIZE                               = 0
    , XA_DAP_VLLDP_CONFIG_PARAM_SAMPLE_RATE                            = 1
    , XA_DAP_VLLDP_CONFIG_PARAM_PCM_WDSZ                               = 2
    , XA_DAP_VLLDP_CONFIG_PARAM_INTERLEAVED_MODE                       = 3
    , XA_DAP_VLLDP_CONFIG_PARAM_NUM_CHANNELS                           = 4
    , XA_DAP_VLLDP_CONFIG_PARAM_MAX_NUM_CHANNELS                       = 5
    , XA_DAP_VLLDP_CONFIG_PARAM_FILTER_CONFIG                          = 6


#define XA_DAP_VLLDP_CONFIG_PARAM_BASE 20
    , XA_DAP_VLLDP_CONFIG_PARAM_SYSTEM_GAIN                            = XA_DAP_VLLDP_CONFIG_PARAM_BASE + 0
    , XA_DAP_VLLDP_CONFIG_PARAM_POSTGAIN                               = XA_DAP_VLLDP_CONFIG_PARAM_BASE + 1
    , XA_DAP_VLLDP_CONFIG_PARAM_AUDIO_OPTIMIZER_ENABLE                 = XA_DAP_VLLDP_CONFIG_PARAM_BASE + 2
    , XA_DAP_VLLDP_CONFIG_PARAM_AUDIO_OPTIMIZER_GAINS                  = XA_DAP_VLLDP_CONFIG_PARAM_BASE + 3
    , XA_DAP_VLLDP_CONFIG_PARAM_REGULATOR_SPEAKER_DISTORTION_ENABLE    = XA_DAP_VLLDP_CONFIG_PARAM_BASE + 4
    , XA_DAP_VLLDP_CONFIG_PARAM_REGULATOR_OVERDRIVE                    = XA_DAP_VLLDP_CONFIG_PARAM_BASE + 5
    , XA_DAP_VLLDP_CONFIG_PARAM_REGULATOR_THRESHOLDS                   = XA_DAP_VLLDP_CONFIG_PARAM_BASE + 6
    , XA_DAP_VLLDP_CONFIG_PARAM_REGULATOR_RELAXATION_AMOUNT            = XA_DAP_VLLDP_CONFIG_PARAM_BASE + 7
    , XA_DAP_VLLDP_CONFIG_PARAM_REGULATOR_TIMBRE_PRESERVATION          = XA_DAP_VLLDP_CONFIG_PARAM_BASE + 8
    , XA_DAP_VLLDP_CONFIG_PARAM_REGULATOR_ISOLATED_BANDS               = XA_DAP_VLLDP_CONFIG_PARAM_BASE + 9

    , XA_DAP_VLLDP_CONFIG_PARAM_REGULATOR_TUNING_INFO                  = XA_DAP_VLLDP_CONFIG_PARAM_BASE + 10
    , XA_DAP_VLLDP_CONFIG_PARAM_VIS_BANDS                              = XA_DAP_VLLDP_CONFIG_PARAM_BASE + 11
    , XA_DAP_VLLDP_CONFIG_PARAM_LATENCY                                = XA_DAP_VLLDP_CONFIG_PARAM_BASE + 12
};

/* Codec specific error codes */
#include "xa_error_standards.h"

/*****************************************************************************/
/* Class 0: API Errors                                                       */
/*****************************************************************************/
/* Non Fatal Errors */
enum xa_error_nonfatal_api_dap_vlldp
{
    XA_DAP_VLLDP_API_NONFATAL_INVALID_API_SEQ    = XA_ERROR_CODE(xa_severity_nonfatal, xa_class_api, XA_CODEC_DAP_VLLDP, 0)
};

/* Fatal Errors */
enum xa_error_fatal_api_dap_vlldp
{
    XA_DAP_VLLDP_API_FATAL_INVALID_API_SEQ       = XA_ERROR_CODE(xa_severity_fatal, xa_class_api, XA_CODEC_DAP_VLLDP, 4)
};

/*****************************************************************************/
/* Class 1: Configuration Errors                                             */
/*****************************************************************************/
/* Nonfatal Errors */
enum xa_error_nonfatal_config_dap_vlldp
{
      XA_DAP_VLLDP_CONFIG_NONFATAL_INVALID_PARAM                     = XA_ERROR_CODE(xa_severity_nonfatal, xa_class_config, XA_CODEC_DAP_VLLDP, 0)
    , XA_DAP_VLLDP_CONFIG_NONFATAL_READONLY_PARAM                    = XA_ERROR_CODE(xa_severity_nonfatal, xa_class_config, XA_CODEC_DAP_VLLDP, 1)
    , XA_DAP_VLLDP_CONFIG_NONFATAL_INVALID_GEN_STRM_POS              = XA_ERROR_CODE(xa_severity_nonfatal, xa_class_config, XA_CODEC_DAP_VLLDP, 2)
    , XA_DAP_VLLDP_CONFIG_NONFATAL_INVALID_BLK_SAMP_RATE_COMBINATION = XA_ERROR_CODE(xa_severity_nonfatal, xa_class_config, XA_CODEC_DAP_VLLDP, 3)
};

/* Fatal Errors */
enum xa_error_fatal_config_dap_vlldp
{
	dummy1
};

/*****************************************************************************/
/* Class 2: Execution Class Errors                                           */
/*****************************************************************************/
/* Nonfatal Errors */
enum xa_error_nonfatal_execute_dap_vlldp
{
    XA_DAP_VLLDP_EXECUTE_NONFATAL_INSUFFICIENT_DATA       = XA_ERROR_CODE(xa_severity_nonfatal, xa_class_execute, XA_CODEC_DAP_VLLDP, 0)
};

/* Fatal Errors */
enum xa_error_fatal_execute_dap_vlldp
{
	dummy2
};

/*****************************************************************************/
/* Codec Specific enumurations and structures                                */
/*****************************************************************************/
#include "xa_type_def.h"

#define XA_DAP_VLLDP_NUM_BANDS                 (20)    /** The number of frequency bands used to modify the frequency reponse. */
#define XA_DAP_VLLDP_NUM_CHANNELS_MIN          (2)     /** The minimum number of channels supported by DAP-VLLDP. */
#define XA_DAP_VLLDP_NUM_CHANNELS_MAX          (8)     /** The maximum number of channels supported by DAP-VLLDP. */
#define XA_DAP_VLLDP_LATENCY                   (48)    /** Samples of latency introduced when processing audio through DAP-VLLDP. */

/** Block size which the library will be operating on. This must be 48 or 64. */
#define XA_DAP_VLLDP_BLOCK_SIZE_48        (48)
#define XA_DAP_VLLDP_BLOCK_SIZE_64        (64)
#define XA_DAP_VLLDP_BLOCK_SIZE_96        (96)
#define XA_DAP_VLLDP_BLOCK_SIZE_128       (128)
#define XA_DAP_VLLDP_BLOCK_SIZE_192       (192)
#define XA_DAP_VLLDP_BLOCK_SIZE_256       (256)
#define XA_DAP_VLLDP_BLOCK_SIZE_DEFAULT   XA_DAP_VLLDP_BLOCK_SIZE_64

/** Sample rate of the audio which the library will be operating on. The value is in hertz, and must be 44100 or 48000. */
#define XA_DAP_VLLDP_SAMPLE_RATE_44100       (44100)
#define XA_DAP_VLLDP_SAMPLE_RATE_48000       (48000)
#define XA_DAP_VLLDP_SAMPLE_RATE_96000       (96000)
#define XA_DAP_VLLDP_SAMPLE_RATE_192000      (192000)
#define XA_DAP_VLLDP_SAMPLE_RATE_DEFAULT     XA_DAP_VLLDP_SAMPLE_RATE_48000

#define XA_DAP_VLLDP_PCM_WDSZ_16          (16)
#define XA_DAP_VLLDP_PCM_WDSZ_32          (32)
#define XA_DAP_VLLDP_PCM_WDSZ_DEFAULT     XA_DAP_VLLDP_PCM_WDSZ_32

#define XA_DAP_VLLDP_INTERLEAVED_MODE_OFF  (0)
#define XA_DAP_VLLDP_INTERLEAVED_MODE_ON   (1)
#define XA_DAP_VLLDP_INTERLEAVED_MODE_DEFAULT XA_DAP_VLLDP_INTERLEAVED_MODE_ON

#define XA_DAP_VLLDP_MAX_FILTER_CONFIG         (512)

#define XA_DAP_VLLDP_SYSTEM_GAIN_MIN           (-2080) /**< Minimum System Gain            */
#define XA_DAP_VLLDP_SYSTEM_GAIN_MAX           (480)   /**< Maximum System Gain            */
#define XA_DAP_VLLDP_SYSTEM_GAIN_DEFAULT       (0)     /**< Default System Gain            */
#define XA_DAP_VLLDP_SYSTEM_GAIN_FRAC_BITS     (4u)    /**< Fractional Bits of System Gain */

#define XA_DAP_VLLDP_POSTGAIN_MIN              (-2080) /**< Minimum Post Gain */
#define XA_DAP_VLLDP_POSTGAIN_MAX              (480)   /**< Maximum Post Gain */
#define XA_DAP_VLLDP_POSTGAIN_DEFAULT          (0)     /**< Default Post Gain */
#define XA_DAP_VLLDP_POSTGAIN_FRAC_BITS        (4u)    /**< Fractional Bits of Post  Gain */

#define XA_DAP_VLLDP_AUDIO_OPTIMIZER_GAIN_MIN  (-30 * 16)  /**< Minimum Audio Optimizer band gain */
#define XA_DAP_VLLDP_AUDIO_OPTIMIZER_GAIN_MAX  ( 30 * 16)  /**< Maximum Audio Optimizer band gain */

#define XA_DAP_VLLDP_REGULATOR_THRESHOLD_HIGH_DEFAULT  (0)     /**< Default High Threshold Value */
#define XA_DAP_VLLDP_REGULATOR_THRESHOLD_LOW_DEFAULT   (-192)  /**< Default Low Threshold Value */
#define XA_DAP_VLLDP_REGULATOR_THRESHOLD_MIN           (-2080) /**< Minimum Threshold Value */
#define XA_DAP_VLLDP_REGULATOR_THRESHOLD_MAX           (0)     /**< Maximum Threshold Value */
#define XA_DAP_VLLDP_REGULATOR_THRESHOLD_FRAC_BITS     (4u)    /**< Fractional Bits in Thresholds */

#define XA_DAP_VLLDP_REGULATOR_ISOLATED_DEFAULT        (0)     /**< Default Isolated Band Value */

#define XA_DAP_VLLDP_REGULATOR_OVERDRIVE_DEFAULT       (0)     /**< Default Overdrice Value */
#define XA_DAP_VLLDP_REGULATOR_OVERDRIVE_MIN           (0)     /**< Minimum Overdrive Value */
#define XA_DAP_VLLDP_REGULATOR_OVERDRIVE_MAX           (192)   /**< Maximum Overdrive Value */
#define XA_DAP_VLLDP_REGULATOR_OVERDRIVE_FRAC_BITS     (4u)    /**< Fractional Bits in Overdrive */

#define XA_DAP_VLLDP_REGULATOR_TP_AMOUNT_DEFAULT       (16)    /**< Default Timbre Preservation Value */
#define XA_DAP_VLLDP_REGULATOR_TP_AMOUNT_MIN           (0)     /**< Minimum Timbre Preservation Value */
#define XA_DAP_VLLDP_REGULATOR_TP_AMOUNT_MAX           (16)    /**< Maximum Timbre Preservation Value */
#define XA_DAP_VLLDP_REGULATOR_TP_AMOUNT_FRAC_BITS     (4u)    /**< Fractional Bits in Timbre Preservation */

#define XA_DAP_VLLDP_REGULATOR_RELAXATION_AMOUNT_DEFAULT              (96)     /**< Default Distortion Relaxation Value */
#define XA_DAP_VLLDP_REGULATOR_RELAXATION_AMOUNT_MIN                  (0)      /**< Minimum Distortion Relaxation Value */
#define XA_DAP_VLLDP_REGULATOR_RELAXATION_AMOUNT_MAX                  (144)    /**< Maximum Distortion Relaxation Value */
#define XA_DAP_VLLDP_REGULATOR_RELAXATION_AMOUNT_FRAC_BITS            (4u)     /**< Fractional Bits in Relaxation Value */

#define XA_DAP_VLLDP_REGULATOR_TUNING_INFO_GAINS_FRAC_BITS            (4u)     /**< Fractional bits in gain values */
#define XA_DAP_VLLDP_REGULATOR_TUNING_INFO_EXCITATIONS_FRAC_BITS      (4u)     /**< Fractional bits in excitation values */
#define XA_DAP_VLLDP_REGULATOR_TUNING_INFO_GAINS_MIN                  (-2080)  /**< Minimum value that will be returned as a gain */
#define XA_DAP_VLLDP_REGULATOR_TUNING_INFO_GAINS_MAX                  (0)      /**< Maximum value that will be returned as a gain */
#define XA_DAP_VLLDP_REGULATOR_TUNING_INFO_EXCITATIONS_MIN            (-2080)  /**< Minimum value that will be returned as an excitation */
#define XA_DAP_VLLDP_REGULATOR_TUNING_INFO_EXCITATIONS_MAX            (0)      /**< Maximum value that will be returned as an excitation */

#define XA_DAP_VLLDP_VIS_EXCITATION_FRAC_BITS                         (4u)     /**< Fractional bits in Visualizer excitations */
#define XA_DAP_VLLDP_VIS_EXCITATION_MIN                               (-576)   /**< Minimum Visualizer excitation */
#define XA_DAP_VLLDP_VIS_EXCITATION_MAX                               (576)    /**< Maximum Visualizer excitation */

/** Samples of latency introduced when processing audio through DAP-VLLDP. */
#define XA_DAP_VLLDP_LATENCY_48K (48)
#define XA_DAP_VLLDP_LATENCY_96K (96)
#define XA_DAP_VLLDP_LATENCY_192K (192)

/** The maximum block size supported by DAP_VLLDP */
#define XA_DAP_VLLDP_MAX_BLOCKSIZE (256)

typedef struct {
    UWORD32 num_elements;
    WORD32 config[XA_DAP_VLLDP_MAX_FILTER_CONFIG];
 } xa_dap_vlldp_prefilter_t;

typedef struct {
    UWORD32 num_gains;
    WORD32 gains[XA_DAP_VLLDP_NUM_CHANNELS_MAX * XA_DAP_VLLDP_NUM_BANDS];
 } xa_dap_vlldp_optimizer_gains_t;

typedef struct {
    WORD32 high_thresholds[XA_DAP_VLLDP_NUM_BANDS];
    WORD32 low_thresholds[XA_DAP_VLLDP_NUM_BANDS];
} xa_dap_vlldp_regulator_thresholds_t;

typedef struct {
    WORD32 gains[XA_DAP_VLLDP_NUM_BANDS];
    WORD32 excitations[XA_DAP_VLLDP_NUM_BANDS];
} xa_dap_vlldp_regulator_tuning_info_t;

#if defined(USE_DLL) && defined(_WIN32)
#define DLL_SHARED __declspec(dllimport)
#elif defined (_WINDLL)
#define DLL_SHARED __declspec(dllexport)
#else
#define DLL_SHARED
#endif

#if defined(__cplusplus)
extern "C" {
#endif
    DLL_SHARED xa_codec_func_t xa_dap_vlldp;
#if defined(__cplusplus)
}
#endif

#endif /* __XA_DAP_VLLDP_API_H__ */
