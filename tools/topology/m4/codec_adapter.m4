divert(-1)

dnl Define macro for CODEC_ADAPTER widget

dnl CODEC_ADAPTER name)

define(`N_CODEC_ADAPTER', `CODEC_ADAPTER'PIPELINE_ID`.'$1)

dnl W_CODEC_ADAPTER(name, format, periods_sink, periods_source, core, kcontrol0. kcontrol1...etc)
define(`W_CODEC_ADAPTER',
`SectionVendorTuples."'N_CODEC_ADAPTER($1)`_tuples_w" {'
`	tokens "sof_comp_tokens"'
`	tuples."word" {'
`		SOF_TKN_COMP_PERIOD_SINK_COUNT'		STR($3)
`		SOF_TKN_COMP_PERIOD_SOURCE_COUNT'	STR($4)
`		SOF_TKN_COMP_CORE_ID'			STR($5)
`	}'
`}'
`SectionData."'N_CODEC_ADAPTER($1)`_data_w" {'
`	tuples "'N_CODEC_ADAPTER($1)`_tuples_w"'
`}'
`SectionVendorTuples."'N_CODEC_ADAPTER($1)`_tuples_str" {'
`	tokens "sof_comp_tokens"'
`	tuples."string" {'
`		SOF_TKN_COMP_FORMAT'	STR($2)
`	}'
`}'
`SectionData."'N_CODEC_ADAPTER($1)`_data_str" {'
`	tuples "'N_CODEC_ADAPTER($1)`_tuples_str"'
`	tuples "'N_CODEC_ADAPTER($1)`_process_tuples_str"'
`}'
`SectionVendorTuples."'N_CODEC_ADAPTER($1)`_process_tuples_str" {'
`	tokens "sof_process_tokens"'
`	tuples."string" {'
`		SOF_TKN_PROCESS_TYPE'	"CODEC_ADAPTER"
`	}'
`}'
`SectionWidget.ifdef(`CODEC_ADAPTER_NAME', "`CODEC_ADAPTER_NAME'", "`N_CODEC_ADAPTER($1)'") {'
`	index "'PIPELINE_ID`"'
`	type "effect"'
`	no_pm "true"'
`	data ['
`		"'N_CODEC_ADAPTER($1)`_data_w"'
`		"'N_CODEC_ADAPTER($1)`_data_str"'
`	]'
`	bytes ['
		$6
`	]'

`}')

divert(0)dnl
