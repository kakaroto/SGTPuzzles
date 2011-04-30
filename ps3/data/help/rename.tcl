#!/usr/bin/tclsh

foreach file [glob *.txt] {
    set new_file [string tolower $file]
    set new_file [string map {" " ""} $new_file]
    if {$file == "Introduction.txt" || $file == "License.txt" } {
        continue
    }
    if {$file == "Common features.txt" } {
        file delete $file
        continue
    }
    if {$new_file == "rectangles.txt"} {
        set new_file "rect.txt"
    }
    if {$file != $new_file} {
        exec git mv $file $new_file
    }
}