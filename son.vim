if exists("b:current_syntax")
  finish
endif

syn match sonComment  ";.*$"
syn match sonMacro    "^#define.*$"
syn match sonSpecial  '!!"__\(c\|asm\|lua\)__"\s*:'
syn match sonIf       '!"if(.\{-})"\s*:'
syn region sonValue   start='"' skip='\\"' end='"'
syn match sonKey      '"[^"]*"\s*:'
syn match sonBrace    '[{}]'

hi def sonComment ctermfg=245 guifg=#8a8a8a
hi def sonMacro   ctermfg=2   guifg=#00af5f
hi def link sonKey     Identifier
hi def link sonValue   String
hi def link sonIf      Statement
hi def sonSpecial ctermfg=124 guifg=#e03050
hi def sonBrace   ctermfg=7   guifg=#ffffff

let b:current_syntax = "son"

function! s:son_macro_highlight()
  for id in get(b:, 'son_macro_ids', [])
    silent! call matchdelete(id)
  endfor
  let ids = []
  for lnum in range(1, line('$'))
    let name = matchstr(getline(lnum), '^#define\s\+\zs\S\+')
    if name != ''
      call add(ids, matchadd('sonMacro', '\V\<' . escape(name, '\') . '\>'))
    endif
  endfor
  let b:son_macro_ids = ids
endfunction

augroup sonMacroHighlight
  autocmd! * <buffer>
  autocmd BufWinEnter,TextChanged,TextChangedI <buffer> call s:son_macro_highlight()
augroup END
call s:son_macro_highlight()
