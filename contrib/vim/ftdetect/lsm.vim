au BufNewFile,BufRead *.lsm set filetype=lsm
au BufNewFile,BufRead * if getline(1) =~'^#lsm' |
\	set filetype=lsm
\| endif
