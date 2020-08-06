divert(-1)

dnl Define macro for POST_PROCESS widget

dnl POST_PROCESS name)

define(`N_POST_PROCESS', `POST_PROCESS'PIPELINE_ID`.'$1)

dnl W_POST_PROCESS(name, format, periods_sink, periods_source, core, kcontrol0. kcontrol1...etc)
define(`W_POST_PROCESS',
`SectionVendorTuples."'N_POST_PROCESS($1)`_tuples_w" {'
`	tokens "sof_comp_tokens"'
`	tuples."word" {'
`		SOF_TKN_COMP_PERIOD_SINK_COUNT'		STR($3)
`		SOF_TKN_COMP_PERIOD_SOURCE_COUNT'	STR($4)
`		SOF_TKN_COMP_CORE_ID'			STR($5)
`	}'
`}'
`SectionData."'N_POST_PROCESS($1)`_data_w" {'
`	tuples "'N_POST_PROCESS($1)`_tuples_w"'
`}'
`SectionVendorTuples."'N_POST_PROCESS($1)`_tuples_str" {'
`	tokens "sof_comp_tokens"'
`	tuples."string" {'
`		SOF_TKN_COMP_FORMAT'	STR($2)
`	}'
`}'
`SectionData."'N_POST_PROCESS($1)`_data_str" {'
`	tuples "'N_POST_PROCESS($1)`_tuples_str"'
`	tuples "'N_POST_PROCESS($1)`_process_tuples_str"'
`}'
`SectionVendorTuples."'N_POST_PROCESS($1)`_process_tuples_str" {'
`	tokens "sof_process_tokens"'
`	tuples."string" {'
`		SOF_TKN_PROCESS_TYPE'	"POST_PROCESS"
`	}'
`}'
`SectionWidget.ifdef(`POST_PROCESS_NAME', "`POST_PROCESS_NAME'", "`N_POST_PROCESS($1)'") {'
`	index "'PIPELINE_ID`"'
`	type "effect"'
`	no_pm "true"'
`	data ['
`		"'N_POST_PROCESS($1)`_data_w"'
`		"'N_POST_PROCESS($1)`_data_str"'
`	]'
`	bytes ['
		$6
`	]'

`}')

divert(0)dnl
