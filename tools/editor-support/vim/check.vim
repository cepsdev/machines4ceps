" check.vim -- assert that ceps.vim's syntax groups land.
"
" Run from the repository root:
"
"     vim -es -u NONE -N -i NONE \
"       --cmd 'set rtp^=tools/editor-support/vim' \
"       -c 'syntax enable' -c 'set filetype=ceps' \
"       -c 'source tools/editor-support/vim/check.vim' \
"       tools/editor-support/sample.ceps
"
" The wrapper is tools/editor-support/vim/check.sh. Exits non-zero if any
" expected group is missing.

let s:expectations = [
      \ ['m',                     'cepsSIUnit'],
      \ ['s',                     'cepsSIUnit'],
      \ ['kg',                    'cepsSIUnit'],
      \ ['mol',                   'cepsSIUnit'],
      \ ['cd',                    'cepsSIUnit'],
      \ ['A',                     'cepsDeclName'],
      \ ['K',                     'cepsDeclName'],
      \ ['g',                     'cepsDeclName'],
      \ ['kind',                  'cepsKeyword'],
      \ ['Event',                 'cepsKindName'],
      \ ['val',                   'cepsKeyword'],
      \ ['static_for',            'cepsKeyword'],
      \ ['ldi32',                 'cepsOpcode'],
      \ ['addi32',                'cepsOpcode'],
      \ ['OblectamentaDataLabel', 'cepsDeclKind'],
      \ ['sm',                    'cepsSMKeyword'],
      \ ['t',                     'cepsSMKeyword'],
      \ ['100',                   'cepsNumber'],
      \ ['3.14',                  'cepsFloat'],
      \ ]

" Every syntax group seen on a standalone occurrence of a:word.
function! s:GroupsOf(word) abort
  let l:acc = {}
  let l:pattern = '\<' . escape(a:word, '.\*[]~/') . '\>'
  call cursor(1, 1)
  let l:flags = 'cW'
  while 1
    let l:pos = searchpos(l:pattern, l:flags)
    let l:flags = 'W'
    if l:pos == [0, 0]
      break
    endif
    let l:name = synIDattr(synID(l:pos[0], l:pos[1], 1), 'name')
    if l:name !=# ''
      let l:acc[l:name] = 1
    endif
  endwhile
  return l:acc
endfunction

" Silent ex mode discards :echo, so results are written to stdout directly.
let s:out = []
let s:failures = 0
let s:verbose = exists('g:ceps_check_verbose')

for s:e in s:expectations
  let s:got = s:GroupsOf(s:e[0])
  let s:desc = empty(s:got) ? '(token never seen)' : join(sort(keys(s:got)), ' ')
  if s:verbose
    call add(s:out, printf('%-24s %s', s:e[0], s:desc))
  endif
  if !has_key(s:got, s:e[1])
    call add(s:out, printf('FAIL %-24s expected %s, got %s',
          \ s:e[0], s:e[1], s:desc))
    let s:failures += 1
  endif
endfor

if s:failures > 0
  call add(s:out, 'check.vim: ' . s:failures . ' expectation(s) failed')
  call writefile(s:out, '/dev/stdout')
  cquit 1
else
  call add(s:out, 'check.vim: ' . len(s:expectations) . ' expectations hold')
  call writefile(s:out, '/dev/stdout')
  qall!
endif
