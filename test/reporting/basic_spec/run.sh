../../../x86/ceps ../style/basic_style.ceps dot_props.ceps canopen_network_management.ceps $1  --dot_gen --dot_gen_one_file_per_top_level_statemachine --post_processing make_report.ceps #--report_format_sexpression #--print_transition_tables #--print_evaluated_postprocessing_tree
for file in ./*dot
do
 dot $file -Tsvg -o $(basename "$file" .dot).svg
done
