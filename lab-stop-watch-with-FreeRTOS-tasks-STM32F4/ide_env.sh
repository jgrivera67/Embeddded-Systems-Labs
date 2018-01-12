base_dir=$HOME/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks-STM32F4
export SRC_TREE_DIR=$base_dir/Sources
export SRC_DIRS="$SRC_TREE_DIR"
export SRC_DB_DIR=$base_dir/my_src_db
export CSCOPE_DB=$SRC_DB_DIR/cscope.out
export CSCOPE_ROOT_DIR=$SRC_TREE_DIR
export CSCOPE_ROOT_DIR_EXPORTS=''
export CTAGS_FILE=tags
export IDE_SHELL_RC_FILE=$base_dir/ide_env.sh
export MAKE_CONCURRENCY=$((2 * $(getconf _NPROCESSORS_ONLN) + 1))
export PATH="$HOME/my_apps/gcc-arm-none-eabi-5_4-2016q3/arm-none-eabi/bin:$HOME/my_apps/gcc-arm-none-eabi-5_4-2016q3/bin:$PATH"
export PLATFORM=STM32F401

function my_build_all
{
    make
}

