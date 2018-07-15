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
badd +1 Sources/application/FreeRTOSConfig.h
badd +2 Sources/application/FreeRTOSConfig.h.cube4
badd +1 Sources/application/FreeRTOSConfig.h.new
badd +1 Sources/application/FreeRTOSConfig.h.v10.0.1
badd +32 Sources/building-blocks/system_clocks.c
badd +1 NERD_tree_2
badd +1 Sources/build.mk
badd +56 Sources/building-blocks/STM32F401/system_clocks.c
badd +113 ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/sdk-dontuse/MCU/system_MKL28Z7.c
badd +125 ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/sdk-dontuse/MCU/system_MKL28Z7.h
badd +1 Sources/building-blocks/system_clocks.h
badd +1 Sources/building-blocks/microcontroller.h
badd +1 lab_stopwatch_with_freertos_tasks.gpr
badd +6648 ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/MCU/STM32F401/stm32f401xe.h
badd +1 ../Sources/building-blocks/arm_cmsis.h
badd +22 Sources/building-blocks/arm_cmsis.h
badd +14 Sources/building-blocks/uart_driver.h
badd +68 Sources/building-blocks/uart_driver.c
badd +41 ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/building-blocks/io_utils.h
badd +1 Makefile
badd +1 Sources/building-blocks/pin_config.h
badd +0 NERD_tree_6
argglobal
silent! argdel *
$argadd Sources/application/FreeRTOSConfig.h
$argadd Sources/application/FreeRTOSConfig.h.cube4
$argadd Sources/application/FreeRTOSConfig.h.new
$argadd Sources/application/FreeRTOSConfig.h.v10.0.1
set lines=68 columns=252
winpos 0 0
edit Sources/application/FreeRTOSConfig.h
set splitbelow splitright
wincmd _ | wincmd |
vsplit
1wincmd h
wincmd w
wincmd _ | wincmd |
split
1wincmd k
wincmd w
set nosplitbelow
set nosplitright
wincmd t
set winminheight=1 winheight=1 winminwidth=1 winwidth=1
exe 'vert 1resize ' . ((&columns * 92 + 126) / 252)
exe '2resize ' . ((&lines * 17 + 34) / 68)
exe 'vert 2resize ' . ((&columns * 159 + 126) / 252)
exe '3resize ' . ((&lines * 48 + 34) / 68)
exe 'vert 3resize ' . ((&columns * 159 + 126) / 252)
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
let s:l = 80 - ((48 * winheight(0) + 33) / 66)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
80
normal! 049|
wincmd w
argglobal
2argu
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 96 - ((0 * winheight(0) + 8) / 17)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
96
normal! 021|
wincmd w
argglobal
4argu
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 62 - ((22 * winheight(0) + 24) / 48)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
62
normal! 021|
wincmd w
exe 'vert 1resize ' . ((&columns * 92 + 126) / 252)
exe '2resize ' . ((&lines * 17 + 34) / 68)
exe 'vert 2resize ' . ((&columns * 159 + 126) / 252)
exe '3resize ' . ((&lines * 48 + 34) / 68)
exe 'vert 3resize ' . ((&columns * 159 + 126) / 252)
tabedit Sources/building-blocks/microcontroller.h
set splitbelow splitright
wincmd _ | wincmd |
vsplit
1wincmd h
wincmd w
set nosplitbelow
set nosplitright
wincmd t
set winminheight=1 winheight=1 winminwidth=1 winwidth=1
exe '1resize ' . ((&lines * 47 + 34) / 68)
exe 'vert 1resize ' . ((&columns * 159 + 126) / 252)
exe '2resize ' . ((&lines * 47 + 34) / 68)
exe 'vert 2resize ' . ((&columns * 92 + 126) / 252)
argglobal
1argu
if bufexists('Sources/building-blocks/microcontroller.h') | buffer Sources/building-blocks/microcontroller.h | else | edit Sources/building-blocks/microcontroller.h | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 106 - ((6 * winheight(0) + 23) / 47)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
106
normal! 020|
wincmd w
argglobal
1argu
if bufexists('Sources/building-blocks/arm_cmsis.h') | buffer Sources/building-blocks/arm_cmsis.h | else | edit Sources/building-blocks/arm_cmsis.h | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 23 - ((22 * winheight(0) + 23) / 47)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
23
normal! 021|
wincmd w
exe '1resize ' . ((&lines * 47 + 34) / 68)
exe 'vert 1resize ' . ((&columns * 159 + 126) / 252)
exe '2resize ' . ((&lines * 47 + 34) / 68)
exe 'vert 2resize ' . ((&columns * 92 + 126) / 252)
tabedit NERD_tree_6
set splitbelow splitright
set nosplitbelow
set nosplitright
wincmd t
set winminheight=1 winheight=1 winminwidth=1 winwidth=1
argglobal
if bufexists('NERD_tree_6') | buffer NERD_tree_6 | else | edit NERD_tree_6 | endif
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
tabedit Sources/building-blocks/microcontroller.h
set splitbelow splitright
wincmd _ | wincmd |
vsplit
1wincmd h
wincmd w
wincmd _ | wincmd |
split
wincmd _ | wincmd |
split
wincmd _ | wincmd |
split
wincmd _ | wincmd |
split
4wincmd k
wincmd w
wincmd w
wincmd w
wincmd w
set nosplitbelow
set nosplitright
wincmd t
set winminheight=1 winheight=1 winminwidth=1 winwidth=1
exe 'vert 1resize ' . ((&columns * 87 + 126) / 252)
exe '2resize ' . ((&lines * 9 + 34) / 68)
exe 'vert 2resize ' . ((&columns * 164 + 126) / 252)
exe '3resize ' . ((&lines * 8 + 34) / 68)
exe 'vert 3resize ' . ((&columns * 164 + 126) / 252)
exe '4resize ' . ((&lines * 2 + 34) / 68)
exe 'vert 4resize ' . ((&columns * 164 + 126) / 252)
exe '5resize ' . ((&lines * 16 + 34) / 68)
exe 'vert 5resize ' . ((&columns * 164 + 126) / 252)
exe '6resize ' . ((&lines * 27 + 34) / 68)
exe 'vert 6resize ' . ((&columns * 164 + 126) / 252)
argglobal
if bufexists('Sources/building-blocks/microcontroller.h') | buffer Sources/building-blocks/microcontroller.h | else | edit Sources/building-blocks/microcontroller.h | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 130 - ((46 * winheight(0) + 33) / 66)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
130
normal! 03|
wincmd w
argglobal
if bufexists('Sources/building-blocks/system_clocks.h') | buffer Sources/building-blocks/system_clocks.h | else | edit Sources/building-blocks/system_clocks.h | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 15 - ((5 * winheight(0) + 4) / 9)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
15
normal! 0
wincmd w
argglobal
if bufexists('~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/MCU/STM32F401/stm32f401xe.h') | buffer ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/MCU/STM32F401/stm32f401xe.h | else | edit ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/MCU/STM32F401/stm32f401xe.h | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 3891 - ((6 * winheight(0) + 4) / 8)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
3891
normal! 024|
wincmd w
argglobal
if bufexists('~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/MCU/STM32F401/stm32f401xe.h') | buffer ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/MCU/STM32F401/stm32f401xe.h | else | edit ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/MCU/STM32F401/stm32f401xe.h | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 3966 - ((1 * winheight(0) + 1) / 2)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
3966
normal! 0
wincmd w
argglobal
if bufexists('Sources/building-blocks/STM32F401/system_clocks.c') | buffer Sources/building-blocks/STM32F401/system_clocks.c | else | edit Sources/building-blocks/STM32F401/system_clocks.c | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 52 - ((15 * winheight(0) + 8) / 16)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
52
normal! 04|
wincmd w
argglobal
if bufexists('Sources/building-blocks/STM32F401/system_clocks.c') | buffer Sources/building-blocks/STM32F401/system_clocks.c | else | edit Sources/building-blocks/STM32F401/system_clocks.c | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 88 - ((26 * winheight(0) + 13) / 27)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
88
normal! 0
wincmd w
exe 'vert 1resize ' . ((&columns * 87 + 126) / 252)
exe '2resize ' . ((&lines * 9 + 34) / 68)
exe 'vert 2resize ' . ((&columns * 164 + 126) / 252)
exe '3resize ' . ((&lines * 8 + 34) / 68)
exe 'vert 3resize ' . ((&columns * 164 + 126) / 252)
exe '4resize ' . ((&lines * 2 + 34) / 68)
exe 'vert 4resize ' . ((&columns * 164 + 126) / 252)
exe '5resize ' . ((&lines * 16 + 34) / 68)
exe 'vert 5resize ' . ((&columns * 164 + 126) / 252)
exe '6resize ' . ((&lines * 27 + 34) / 68)
exe 'vert 6resize ' . ((&columns * 164 + 126) / 252)
tabedit Sources/building-blocks/pin_config.h
set splitbelow splitright
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
set nosplitbelow
set nosplitright
wincmd t
set winminheight=1 winheight=1 winminwidth=1 winwidth=1
exe '1resize ' . ((&lines * 21 + 34) / 68)
exe 'vert 1resize ' . ((&columns * 127 + 126) / 252)
exe '2resize ' . ((&lines * 24 + 34) / 68)
exe 'vert 2resize ' . ((&columns * 127 + 126) / 252)
exe '3resize ' . ((&lines * 19 + 34) / 68)
exe 'vert 3resize ' . ((&columns * 127 + 126) / 252)
exe '4resize ' . ((&lines * 32 + 34) / 68)
exe 'vert 4resize ' . ((&columns * 124 + 126) / 252)
exe '5resize ' . ((&lines * 33 + 34) / 68)
exe 'vert 5resize ' . ((&columns * 124 + 126) / 252)
argglobal
if bufexists('Sources/building-blocks/pin_config.h') | buffer Sources/building-blocks/pin_config.h | else | edit Sources/building-blocks/pin_config.h | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 48 - ((20 * winheight(0) + 10) / 21)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
48
normal! 0
wincmd w
argglobal
if bufexists('Sources/building-blocks/uart_driver.c') | buffer Sources/building-blocks/uart_driver.c | else | edit Sources/building-blocks/uart_driver.c | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 15 - ((14 * winheight(0) + 12) / 24)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
15
normal! 0
wincmd w
argglobal
if bufexists('Makefile') | buffer Makefile | else | edit Makefile | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 2 - ((1 * winheight(0) + 9) / 19)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
2
normal! 012|
wincmd w
argglobal
if bufexists('~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/MCU/STM32F401/stm32f401xe.h') | buffer ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/MCU/STM32F401/stm32f401xe.h | else | edit ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/MCU/STM32F401/stm32f401xe.h | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 527 - ((17 * winheight(0) + 16) / 32)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
527
normal! 04|
wincmd w
argglobal
if bufexists('~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/MCU/STM32F401/stm32f401xe.h') | buffer ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/MCU/STM32F401/stm32f401xe.h | else | edit ~/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4/Sources/MCU/STM32F401/stm32f401xe.h | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 775 - ((15 * winheight(0) + 16) / 33)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
775
normal! 09|
wincmd w
exe '1resize ' . ((&lines * 21 + 34) / 68)
exe 'vert 1resize ' . ((&columns * 127 + 126) / 252)
exe '2resize ' . ((&lines * 24 + 34) / 68)
exe 'vert 2resize ' . ((&columns * 127 + 126) / 252)
exe '3resize ' . ((&lines * 19 + 34) / 68)
exe 'vert 3resize ' . ((&columns * 127 + 126) / 252)
exe '4resize ' . ((&lines * 32 + 34) / 68)
exe 'vert 4resize ' . ((&columns * 124 + 126) / 252)
exe '5resize ' . ((&lines * 33 + 34) / 68)
exe 'vert 5resize ' . ((&columns * 124 + 126) / 252)
tabedit Sources/building-blocks/pin_config.h
set splitbelow splitright
set nosplitbelow
set nosplitright
wincmd t
set winminheight=1 winheight=1 winminwidth=1 winwidth=1
argglobal
if bufexists('Sources/building-blocks/pin_config.h') | buffer Sources/building-blocks/pin_config.h | else | edit Sources/building-blocks/pin_config.h | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 25 - ((0 * winheight(0) + 33) / 67)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
25
normal! 0
tabedit lab_stopwatch_with_freertos_tasks.gpr
set splitbelow splitright
set nosplitbelow
set nosplitright
wincmd t
set winminheight=1 winheight=1 winminwidth=1 winwidth=1
argglobal
if bufexists('lab_stopwatch_with_freertos_tasks.gpr') | buffer lab_stopwatch_with_freertos_tasks.gpr | else | edit lab_stopwatch_with_freertos_tasks.gpr | endif
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
tabnext 7
if exists('s:wipebuf')
  silent exe 'bwipe ' . s:wipebuf
endif
unlet! s:wipebuf
set winheight=1 winwidth=1 shortmess=aoO
set winminheight=1 winminwidth=1
let s:sx = expand("<sfile>:p:r")."x.vim"
if file_readable(s:sx)
  exe "source " . fnameescape(s:sx)
endif
let &so = s:so_save | let &siso = s:siso_save
doautoall SessionLoadPost
unlet SessionLoad
" vim: set ft=vim :
