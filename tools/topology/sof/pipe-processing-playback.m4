# Low Latency Passthrough Pipeline and PCM
#
# Pipeline Endpoints for connection are :-
#
#  host PCM_P --> B0 --> CODEC_ADAPTER(PP) --> B1 --> sink DAI0

# codec Post Process setup config
define(`CA_SETUP_CONTROLBYTES',
``	bytes "0x53,0x4f,0x46,0x00,0x00,0x00,0x00,0x00,'
`	0x5C,0x00,0x00,0x00,0x00,0x10,0x00,0x03,'
`	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'
`	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'
`	0x01,0x01,0xDE,0xCA,0x00,0x00,0x00,0x00,'
`	0x01,0x00,0x00,0x00,0x02,0x00,0x00,0x00,'
`	0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'
`	0x0C,0x00,0x00,0x00,0x40,0x00,0x00,0x00,'
`	0x01,0x00,0x00,0x00,0x0C,0x00,0x00,0x00,'
`	0x80,0xBB,0x00,0x00,0x02,0x00,0x00,0x00,'
`	0x0C,0x00,0x00,0x00,0x20,0x00,0x00,0x00,'
`	0x03,0x00,0x00,0x00,0x0C,0x00,0x00,0x00,'
`	0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x00,'
`	0x0C,0x00,0x00,0x00,0x02,0x00,0x00,0x00,'
`	0x05,0x00,0x00,0x00,0x0C,0x00,0x00,0x00,'
`	0x02,0x00,0x00,0x00"''
)
define(`CA_SETUP_CONTROLBYTES_MAX', 300)

# codec Post Process runtime params
define(`CA_RUNTIME_CONTROLBYTES',
``       bytes "0x53,0x4f,0x46,0x00,0x01,0x00,0x00,0x00,'
`       0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x03,'
`       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'
`       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00"''
)
define(`CA_RUNTIME_CONTROLBYTES_MAX', 157)

define(`CA_SCHEDULE_CORE', 1)

define(`CA_SETUP_CONTROLBYTES_NAME', `Post Process Setup Config')
define(`CA_RUNTIME_CONTROLBYTES_NAME', `Post Process Runtime Config')

undefine(`DAI_PERIODS')
define(`DAI_PERIODS', 8)

# Include codec adapter playback topology
include(`sof/pipe-codec-adapter-playback.m4')
