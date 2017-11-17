#!/usr/bin/perl
#
# Tool to print stack traces from raw stack traces
#
# Invocation syntax:
# gen_call_graph.pl <ELF file> <text file with raw stack traces>
#
# Author: German Rivera
#
use strict;
use warnings;
use File::Basename;
use File::Path;

#
# Name of this tool
#
my $PROG_NAME = basename($0);

my $USAGE_STR = "Usage: $PROG_NAME <ELF file> <raw stack trace file>";

#
# Main program
#
{
    my $elf_file;
    my $raw_stack_trace_file;
    my $result;

    if (@ARGV != 2) {
        my $num_args = @ARGV;
        die "*** Error: Invalid number of arguments: $num_args (@ARGV)\n$USAGE_STR\n";
    }

    ($elf_file, $raw_stack_trace_file) = @ARGV;

    open IN_FILE_HANDLE, "<$raw_stack_trace_file" ||
        die "$PROG_NAME: *** Error: opening $raw_stack_trace_file failed";

    while (<IN_FILE_HANDLE>) {
        if ($_ !~ /^\s+0x/) {
            print $_;
            next;
        }

        chomp $_;
        $_ =~ s/^\s+//;     # trim leading white spaces
        my @record = split /[ \t]+/, $_;
        if (@record == 0) {
            next;
        }

        my $call_addr = $record[0];
        print "\t";
        #system "arm-none-eabi-addr2line -e $elf_file -afps $call_addr";
        system "addr2line -e $elf_file -afps $call_addr";
    }

    close IN_FILE_HANDLE;
    exit 0;
}

