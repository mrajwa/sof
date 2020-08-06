# Low Latency Passthrough Pipeline and PCM
#
# Pipeline Endpoints for connection are :-
#
#  host PCM_P --> B0 --> sink DAI0

# Include topology builder
include(`utils.m4')
include(`buffer.m4')
include(`pcm.m4')
include(`dai.m4')
include(`pipeline.m4')
include(`post_process.m4')
include(`bytecontrol.m4')

#
# Controls
#

# Post process setup config
CONTROLBYTES_PRIV(PP_SETUP_CONFIG,
`       bytes "0x53,0x4f,0x46,0x00,0x00,0x00,0x00,0x00,'
`       0x14,0x00,0x00,0x00,0x00,0x10,0x00,0x03,'
`       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'
`       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'
`       0x80,0xBB,0x00,0x00,0x10,0x00,0x00,0x00,'
`       0x02,0x00,0x00,0x00,0x40,0x00,0x00,0x00,'
`       0x01,0x00,0x00,0x00"'
)

# Post process Bytes control for setup config
C_CONTROLBYTES(Post Process Setup Config, PIPELINE_ID,
        CONTROLBYTES_OPS(bytes),
        CONTROLBYTES_EXTOPS(void, 258, 258),
        , , ,
        CONTROLBYTES_MAX(void, 144),
        ,
        PP_SETUP_CONFIG)


# Post process runtime params
# NOTE! Below config only sets general part of post_process_runtime_config
# struct. This is indicated by the apply_mask which always comes as first
# four bytes.
CONTROLBYTES_PRIV(PP_RUNTIME_PARAMS,
`       bytes "0x53,0x4f,0x46,0x00,0x01,0x00,0x00,0x00,'
`       0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x03,'
`       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'
`       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00"'
)

# Post process Bytes control for runtime config
C_CONTROLBYTES(Post Process Runtime Params, PIPELINE_ID,
        CONTROLBYTES_OPS(bytes),
       	CONTROLBYTES_EXTOPS(void, 258, 258),
        , , ,
        CONTROLBYTES_MAX(void, 157),
        ,
        PP_RUNTIME_PARAMS)

#
# Components and Buffers
#

# Host "Playback with post process" PCM
# with 2 sink and 0 source periods
W_PCM_PLAYBACK(PCM_ID, Passthrough Playback, DAI_PERIODS, 0, 0)


W_POST_PROCESS(0, PIPELINE_FORMAT, 0, 2, 1, LIST(`             ', "Post Process Setup Config", "Post Process Runtime Params"))

# Playback Buffers
W_BUFFER(0, COMP_BUFFER_SIZE(DAI_PERIODS,
	COMP_SAMPLE_SIZE(PIPELINE_FORMAT), PIPELINE_CHANNELS, COMP_PERIOD_FRAMES(PCM_MAX_RATE, SCHEDULE_PERIOD)),
	PLATFORM_PASS_MEM_CAP)

W_BUFFER(1, COMP_BUFFER_SIZE(DAI_PERIODS,
	COMP_SAMPLE_SIZE(PIPELINE_FORMAT), PIPELINE_CHANNELS, COMP_PERIOD_FRAMES(PCM_MAX_RATE, SCHEDULE_PERIOD)),
	PLATFORM_PASS_MEM_CAP)

#
# Pipeline Graph
#
#  host PCM_P --> B0 --> POST_PROCESS -> B1 --> sink DAI0

P_GRAPH(pipe-pass-playback-PIPELINE_ID, PIPELINE_ID,
	LIST(`		',
	`dapm(N_BUFFER(0), N_PCMP(PCM_ID))',
	`dapm(N_POST_PROCESS(0), N_BUFFER(0))',
	`dapm(N_BUFFER(1), N_POST_PROCESS(0))'))

#
# Pipeline Source and Sinks
#
indir(`define', concat(`PIPELINE_SOURCE_', PIPELINE_ID), N_BUFFER(1))
indir(`define', concat(`PIPELINE_PCM_', PIPELINE_ID), Passthrough Playback PCM_ID)

#
# PCM Configuration
#

PCM_CAPABILITIES(Passthrough Playback PCM_ID, `S32_LE,S24_LE,S16_LE', PCM_MIN_RATE, PCM_MAX_RATE, 2, PIPELINE_CHANNELS, 2, 16, 192, 16384, 65536, 65536)