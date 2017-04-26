echo -e "\e[104mRunning a.ceps\e[0m"
../../x86/ceps ../reporting/style/basic_style.ceps a.ceps --dot_gen --post_processing ../reporting/basic_spec/make_report.ceps && dot out.dot -Tsvg -o a.svg
echo -e "\e[104mRunning b.ceps\e[0m"
../../x86/ceps ../reporting/style/basic_style.ceps b.ceps --dot_gen --post_processing ../reporting/basic_spec/make_report.ceps && dot out.dot -Tsvg -o b.svg
echo -e "\e[104mRunning c.ceps\e[0m"
../../x86/ceps ../reporting/style/basic_style.ceps c.ceps --dot_gen --post_processing ../reporting/basic_spec/make_report.ceps && dot out.dot -Tsvg -o c.svg
