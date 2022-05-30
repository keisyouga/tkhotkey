# test hotkey

# load extension
load ./libhotkey.so

# register
after 100 {hotkey::register 38 8 {puts aaa}}; # alt+a
after 100 {hotkey::register 56 8 {puts bbb}}; # alt+b
after 100 {hotkey::register 54 8 {puts ccc}}; # alt+c

# unregister
after 400 {hotkey::unregister 54 8}
after 400 {hotkey::unregister 56 8}

# alt+o to popup window
after 100 {hotkey::register 0x20 8 {wm withdraw . ; after 100 {wm deiconify .}}}
