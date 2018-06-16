base_dir=$HOME/projects/Embeddded-Systems-Labs/lab-stop-watch-with-FreeRTOS-tasks
export SRC_TREE_DIR=$base_dir/Sources
export SRC_DIRS="$SRC_TREE_DIR"
export SRC_DB_DIR=$base_dir/my_src_db
export CSCOPE_DB=$SRC_DB_DIR/cscope.out
export CSCOPE_ROOT_DIR=$SRC_TREE_DIR
export CSCOPE_ROOT_DIR_EXPORTS=''
export CTAGS_FILE=tags
export IDE_SHELL_RC_FILE=$base_dir/ide_env.sh
export MAKE_CONCURRENCY=$((2 * $(getconf _NPROCESSORS_ONLN) + 1))
#export PATH="$HOME/my_apps/gcc-arm-none-eabi-5_4-2016q3/arm-none-eabi/bin:$HOME/my_apps/gcc-arm-none-eabi-5_4-2016q3/bin:$PATH"
export PATH=$HOME/my_apps/GNAT/2018-arm-elf/arm-eabi/bin:$HOME/my_apps/GNAT/2018-arm-elf/bin:$HOME/my_apps/GNAT/2018/bin:$HOME/projects/ada/scripts:$PATH

function my_build_all
{
    make
}

function update_flash
{
    typeset elf_file
    typeset target_dir
    typeset usage_msg

    usage_msg="Usage: update_flash <elf file> <target dir>"

    if [ $# != 2 ]; then
        echo $usage_msg
        return 1
    fi

    elf_file=$1
    target_dir=$2

    if [ ! -f $elf_file ]; then
        echo "*** ERROR: file $elf_file does not exist"
        return 1
    fi

    rm -f $elf_file.bin

    echo "Generating $elf_file.bin ..."
    objcopy -O binary -S $elf_file "$elf_file.bin"

    if [ ! -f $elf_file.bin ]; then
        echo "*** ERROR: file $elf_file.bin not created"
        return 1;
    fi

    echo "Copying $elf_file.bin to flash ($target_dir) ..."
    sudo cp $elf_file.bin $target_dir
    sync
}

function gen_lst
{
    typeset elf_file
    typeset usage_msg

    usage_msg="Usage: gen_lst <elf file>"

    if [ $# != 1 ]; then
        echo $usage_msg
        return 1
    fi

    if [ ! -f $elf_file ]; then
        echo "*** ERROR: file $elf_file does not exist"
        return 1
    fi

    rm -f $elf_file.lst

    echo "Generating $elf_file.lst ..."
    objdump -dSstl $elf_file > $elf_file.lst
    if [ ! -f $elf_file.lst ]; then
        echo "*** ERROR: file $elf_file.lst not created"
        return 1;
    fi
}


