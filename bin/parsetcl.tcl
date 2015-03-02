#!/usr/bin/parsetcl



proc parsetcl::command {comment cmdtext args} {
  puts "=== [list $comment $cmdtext $args] ==="
  puts "<<< [array get ::parsetcl::hint] >>>"
}

proc parsetcl::bracket {comment cmdtext args} {
  puts "--- [list $comment $cmdtext $args] ---"
  puts "<<< [array get ::parsetcl::hint] >>>"

  set buffer $comment
  append buffer [join $args ""]
  return "\[$buffer\]"
}

parsetcl::config -hint 0 -debug 0
parsetcl::config -raw 1

puts [array get parsetcl::config]

parsetcl -code {
 set a 123
 set d1 "I love you"
 set d2 {I love you}
 set d3 [I love you]
 set d4 "I love\nyou"
 set b $a    ;# variable

 set c def[set a]
 # ---- #
 concat {*}[lsort -index 0 $result_check_clock_tree]
 # ---- #
 set result_check_clock_tree [concat {*}[lsort -index 0 $result_check_clock_tree]]
}

puts [array get parsetcl::config]
