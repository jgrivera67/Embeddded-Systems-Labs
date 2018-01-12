let SessionLoad = 1
if &cp | set nocp | endif
let s:so_save = &so | let s:siso_save = &siso | set so=0 siso=0
let v:this_session=expand("<sfile>:p")
silent only
cd ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4
if expand('%') == '' && !&modified && line('$') <= 1 && getline(1) == ''
  let s:wipebuf = bufnr('%')
endif
set shortmess=aoO
badd +15 ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/building-blocks/arm_cmsis.h
badd +12 ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/building-blocks/microcontroller.h
badd +0 Sources/build.mk
badd +291 ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/MCU/stm32f401xe.h
badd +92 ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/building-blocks/pin_config.h
badd +224 ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/application/main.c
badd +31 Sources/MCU/STM32F401VEHx_FLASH.ld
badd +236 ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/sdk-dontuse/board/clock_config.c
badd +71 Sources/building-blocks/system_clocks.c
badd +41 ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/building-blocks/io_utils.h
badd +18 ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/building-blocks/pin_config.c
badd +15 ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/building-blocks/compile_time_checks.h
badd +63 ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/building-blocks/byte_ring_buffer.h
badd +18 Sources/building-blocks/byte_ring_buffer.c
badd +27 ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/building-blocks/hw_timer_driver.h
argglobal
silent! argdel *
argadd .
set lines=68 columns=252
winpos 0 0
set stal=2
edit ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/MCU/stm32f401xe.h
set splitbelow splitright
wincmd _ | wincmd |
split
1wincmd k
wincmd _ | wincmd |
vsplit
1wincmd h
wincmd _ | wincmd |
split
wincmd _ | wincmd |
split
2wincmd k
wincmd w
wincmd w
wincmd w
wincmd _ | wincmd |
split
1wincmd k
wincmd w
wincmd w
set nosplitbelow
set nosplitright
wincmd t
set winheight=1 winwidth=1
exe '1resize ' . ((&lines * 18 + 34) / 68)
exe 'vert 1resize ' . ((&columns * 127 + 126) / 252)
exe '2resize ' . ((&lines * 19 + 34) / 68)
exe 'vert 2resize ' . ((&columns * 127 + 126) / 252)
exe '3resize ' . ((&lines * 16 + 34) / 68)
exe 'vert 3resize ' . ((&columns * 127 + 126) / 252)
exe '4resize ' . ((&lines * 27 + 34) / 68)
exe 'vert 4resize ' . ((&columns * 124 + 126) / 252)
exe '5resize ' . ((&lines * 27 + 34) / 68)
exe 'vert 5resize ' . ((&columns * 124 + 126) / 252)
exe '6resize ' . ((&lines * 10 + 34) / 68)
argglobal
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 4351 - ((9 * winheight(0) + 9) / 18)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
4351
normal! 021|
wincmd w
argglobal
edit ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/building-blocks/pin_config.c
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 16 - ((7 * winheight(0) + 9) / 19)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
16
normal! 012|
wincmd w
argglobal
edit ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/building-blocks/pin_config.h
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 16 - ((15 * winheight(0) + 8) / 16)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
16
normal! 03|
wincmd w
argglobal
edit ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/building-blocks/microcontroller.h
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 125 - ((25 * winheight(0) + 13) / 27)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
125
normal! 0
wincmd w
argglobal
edit ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/building-blocks/compile_time_checks.h
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 17 - ((16 * winheight(0) + 13) / 27)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
17
normal! 0
wincmd w
argglobal
enew
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
wincmd w
exe '1resize ' . ((&lines * 18 + 34) / 68)
exe 'vert 1resize ' . ((&columns * 127 + 126) / 252)
exe '2resize ' . ((&lines * 19 + 34) / 68)
exe 'vert 2resize ' . ((&columns * 127 + 126) / 252)
exe '3resize ' . ((&lines * 16 + 34) / 68)
exe 'vert 3resize ' . ((&columns * 127 + 126) / 252)
exe '4resize ' . ((&lines * 27 + 34) / 68)
exe 'vert 4resize ' . ((&columns * 124 + 126) / 252)
exe '5resize ' . ((&lines * 27 + 34) / 68)
exe 'vert 5resize ' . ((&columns * 124 + 126) / 252)
exe '6resize ' . ((&lines * 10 + 34) / 68)
tabedit ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/building-blocks/hw_timer_driver.h
set splitbelow splitright
wincmd _ | wincmd |
split
1wincmd k
wincmd w
set nosplitbelow
set nosplitright
wincmd t
set winheight=1 winwidth=1
exe '1resize ' . ((&lines * 32 + 34) / 68)
exe '2resize ' . ((&lines * 33 + 34) / 68)
argglobal
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 36 - ((20 * winheight(0) + 16) / 32)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
36
normal! 014|
wincmd w
argglobal
enew
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
wincmd w
exe '1resize ' . ((&lines * 32 + 34) / 68)
exe '2resize ' . ((&lines * 33 + 34) / 68)
tabedit ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/building-blocks/pin_config.h
set splitbelow splitright
set nosplitbelow
set nosplitright
wincmd t
set winheight=1 winwidth=1
argglobal
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 1 - ((0 * winheight(0) + 33) / 67)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
1
normal! 0
tabedit ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/MCU/stm32f401xe.h
set splitbelow splitright
set nosplitbelow
set nosplitright
wincmd t
set winheight=1 winwidth=1
argglobal
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 66 - ((56 * winheight(0) + 33) / 67)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
66
normal! 016|
tabedit Sources/build.mk
set splitbelow splitright
wincmd _ | wincmd |
vsplit
1wincmd h
wincmd w
set nosplitbelow
set nosplitright
wincmd t
set winheight=1 winwidth=1
exe 'vert 1resize ' . ((&columns * 127 + 126) / 252)
exe 'vert 2resize ' . ((&columns * 124 + 126) / 252)
argglobal
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 141 - ((33 * winheight(0) + 33) / 66)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
141
normal! 029|
wincmd w
argglobal
edit Sources/build.mk
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 141 - ((33 * winheight(0) + 33) / 66)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
141
normal! 029|
wincmd w
exe 'vert 1resize ' . ((&columns * 127 + 126) / 252)
exe 'vert 2resize ' . ((&columns * 124 + 126) / 252)
tabnext 5
set stal=1
if exists('s:wipebuf')
  silent exe 'bwipe ' . s:wipebuf
endif
unlet! s:wipebuf
set winheight=1 winwidth=20 shortmess=filnxtToO
let s:sx = expand("<sfile>:p:r")."x.vim"
if file_readable(s:sx)
  exe "source " . fnameescape(s:sx)
endif
let &so = s:so_save | let &siso = s:siso_save
doautoall SessionLoadPost
unlet SessionLoad
" vim: set ft=vim :
