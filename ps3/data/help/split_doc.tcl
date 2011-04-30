#!/usr/bin/tclsh

set in [open output r]

set current_section ""
while {![eof $in] } {
    set line [gets $in]
    if {[string length $line] > 0 && [string index $line 0] != " "} {
        if {[info exists out]} {
            close $out
            unset out
        }
        set current_section [string trim $line]
        set out [open "${current_section}.txt" w]
        puts "Writing $current_section"
    }
    if {[info exists out] } {
        puts $out $line
    }
}
close $in
close $out