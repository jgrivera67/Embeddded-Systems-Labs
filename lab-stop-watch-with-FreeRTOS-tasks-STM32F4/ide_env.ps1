$base_dir = "$env:userprofile/MyProjects/EMBSYS/Embeddded-Systems-Labs"
$project = "lab-stop-watch-with-FreeRTOS-tasks-STM32F4"
$env:src_tree_dir = "$base_dir\$project"
$env:src_db_dir = "$base_dir\$project`_src_database"
$env:src_subdirs = ""
$env:cscope_db = $env:src_db_dir + "\cscope.out"
$env:cscope_root_dir = $env:src_tree_dir
$env:cscope_root_dir_exports = ""

$env:path = "C:\Program Files (x86)\GNU Tools ARM Embedded\5.4 2016q3\bin;C:\Program Files (x86)\GNU Tools ARM Embedded\5.4 2016q3\arm-none-eabi\bin;$env:path"

$env:make = "C:\MinGW\msys\1.0\bin\make.exe"

function update_flash([string]$elf_file, [string]$drive)
{
    $usage_msg = "Usage: update_flash <elf file> <drive letter>"

    if ($elf_file -eq "" -or $drive -eq "") {
        echo "$usage_msg"
        return 1
    }

    if (! (test-path $elf_file)) {
        echo "*** ERROR: file $elf_file does not exist"
        return 1;
    }

    if (test-path "$elf_file.bin") {
        rm "$elf_file.bin"
    }

    echo "Generating $elf_file.bin ..."
    objcopy -O binary -S $elf_file "$elf_file.bin"

    if (! (test-path "$elf_file.bin")) {
        echo "*** ERROR: file $elf_file.bin not created"
        return 1;
    }

    echo "Copying $elf_file.bin to flash (drive $drive`:\) ..."
    copy-item "$elf_file.bin" "$drive`:\"
}

function gen_lst([string]$elf_file)
{
    $usage_msg = "Usage: gen_lst <elf file>"

    if ($elf_file -eq "") {
        echo "$usage_msg"
        return 1
    }

    if (! (test-path $elf_file)) {
        echo "*** ERROR: file $elf_file does not exist"
        return 1;
    }

    if (test-path "$elf_file.lst") {
        rm "$elf_file.lst"
    }

    echo "Generating $elf_file.lst ..."
    objdump -dSstl $elf_file > "$elf_file.lst"
    #objdump -dstl $elf_file > "$elf_file.lst"
    #objdump -dst $elf_file > "$elf_file.lst"

    if (! (test-path "$elf_file.lst")) {
        echo "*** ERROR: file $elf_file.bin not created"
        return 1;
    }
}

function print_stack_trace([string]$elf_file, [string]$raw_stack_trace)
{
    $usage_msg = "Usage: print_stack_trace <elf file> <text file with raw stack trace file>"

    if ($elf_file -eq "" -or $raw_stack_trace -eq "") {
        echo "$usage_msg"
        return 1
    }

    if (! (test-path $elf_file)) {
        echo "*** ERROR: file $elf_file does not exist"
        return 1;
    }

    if (! (test-path $raw_stack_trace)) {
        echo "*** ERROR: file $raw_stack_trace does not exist"
        return 1;
    }

    perl ../scripts/print_stack_trace.pl $elf_file $raw_stack_trace
}

