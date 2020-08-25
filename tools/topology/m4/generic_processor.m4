divert(-1)

dnl Define macro for GENERIC_PROCESSOR widget

dnl GENERIC_PROCESSOR name)

define(`N_GENERIC_PROCESSOR', `GENERIC_PROCESSOR'PIPELINE_ID`.'$1)

dnl W_GENERIC_PROCESSOR(name, format, periods_sink, periods_source, core, kcontrol0. kcontrol1...etc)
define(`W_GENERIC_PROCESSOR',
`SectionVendorTuples."'N_GENERIC_PROCESSOR($1)`_tuples_w" {'
`	tokens "sof_comp_tokens"'
`	tuples."word" {'
`		SOF_TKN_COMP_PERIOD_SINK_COUNT'		STR($3)
`		SOF_TKN_COMP_PERIOD_SOURCE_COUNT'	STR($4)
`		SOF_TKN_COMP_CORE_ID'			STR($5)
`	}'
`}'
`SectionData."'N_GENERIC_PROCESSOR($1)`_data_w" {'
`	tuples "'N_GENERIC_PROCESSOR($1)`_tuples_w"'
`}'
`SectionVendorTuples."'N_GENERIC_PROCESSOR($1)`_tuples_str" {'
`	tokens "sof_comp_tokens"'
`	tuples."string" {'
`		SOF_TKN_COMP_FORMAT'	STR($2)
`	}'
`}'
`SectionData."'N_GENERIC_PROCESSOR($1)`_data_str" {'
`	tuples "'N_GENERIC_PROCESSOR($1)`_tuples_str"'
`	tuples "'N_GENERIC_PROCESSOR($1)`_process_tuples_str"'
`}'
`SectionVendorTuples."'N_GENERIC_PROCESSOR($1)`_process_tuples_str" {'
`	tokens "sof_process_tokens"'
`	tuples."string" {'
`		SOF_TKN_PROCESS_TYPE'	"GENERIC_PROCESSOR"
`	}'
`}'
`SectionWidget.ifdef(`GENERIC_PROCESSOR_NAME', "`GENERIC_PROCESSOR_NAME'", "`N_GENERIC_PROCESSOR($1)'") {'
`	index "'PIPELINE_ID`"'
`	type "effect"'
`	no_pm "true"'
`	data ['
`		"'N_GENERIC_PROCESSOR($1)`_data_w"'
`		"'N_GENERIC_PROCESSOR($1)`_data_str"'
`	]'
`	bytes ['
		$6
`	]'

`}')

divert(0)dnl
