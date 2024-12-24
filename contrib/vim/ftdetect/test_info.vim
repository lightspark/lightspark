au BufNewFile,BufRead test_info if getline(1) =~'^#test' |
\	set filetype=test_info
\| endif
