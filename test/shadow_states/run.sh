echo -e "\e[104mRunning a.ceps\e[0m"
../../x86/ceps ../reporting/style/basic_style.ceps a.ceps --dot_gen --post_processing ../reporting/basic_spec/make_report.ceps && dot out.dot -Tsvg -o a.svg
echo -e "\e[104mRunning b.ceps\e[0m"
../../x86/ceps ../reporting/style/basic_style.ceps b.ceps --dot_gen --post_processing ../reporting/basic_spec/make_report.ceps && dot out.dot -Tsvg -o b.svg
echo -e "\e[104mRunning c.ceps\e[0m"
../../x86/ceps ../reporting/style/basic_style.ceps c.ceps --dot_gen --post_processing ../reporting/basic_spec/make_report.ceps && dot out.dot -Tsvg -o c.svg
echo -e "\e[104mRunning d.ceps\e[0m"
../../x86/ceps ../reporting/style/basic_style.ceps d.ceps --dot_gen --post_processing ../reporting/basic_spec/make_report.ceps && dot out.dot -Tsvg -o d.svg
echo -e "\e[104mRunning e.ceps\e[0m"
../../x86/ceps ../reporting/style/basic_style.ceps e.ceps --dot_gen --post_processing ../reporting/basic_spec/make_report.ceps && dot out.dot -Tsvg -o e.svg
echo -e "\e[104mRunning f.ceps\e[0m"
../../x86/ceps ../reporting/style/basic_style.ceps f.ceps --dot_gen --post_processing ../reporting/basic_spec/make_report.ceps && dot out.dot -Tsvg -o f.svg
echo -e "\e[104mRunning g.ceps\e[0m"
../../x86/ceps ../reporting/style/basic_style.ceps g.ceps --dot_gen --post_processing make_dependency_info.ceps ../reporting/basic_spec/make_report.ceps && dot out.dot -Tsvg -o g.svg
echo -e "\e[104mRunning h.ceps\e[0m"
../../x86/ceps ../reporting/style/basic_style.ceps h.ceps --dot_gen --post_processing make_dependency_info.ceps ../reporting/basic_spec/make_report.ceps && dot out.dot -Tsvg -o h.svg
echo -e "\e[104mRunning i.ceps\e[0m"
../../x86/ceps ../reporting/style/basic_style.ceps i.ceps --dot_gen --post_processing make_dependency_info.ceps ../reporting/basic_spec/make_report.ceps && dot out.dot -Tsvg -o i.svg
