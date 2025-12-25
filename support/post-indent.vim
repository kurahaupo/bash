" GNU indent does a few weird things that we really want to avoid.
"
" To use, open every modified file in vim:
"   shopt -s globstar
"   indent **/*.[ch]
"   vim $( git diff @ --name-only **/*.[ch] )
" and then in each buffer execute
"   :source support/post-indent.vim 

" (1) it reformats «enum foo bar;» by inserting another space between «foo» and
"     «bar», no matter how many spaces are already there.

% s/\<\%(enum\|struct\|union\)\s\w\+\s\zs\s\+//ge
