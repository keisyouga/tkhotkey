# test hotkey

# load extension
load ./libhotkey.so

# register
hotkey::register 38 8 {puts aaa}; # alt+a
hotkey::register 56 8 {puts bbb}; # alt+b
hotkey::register 54 8 {puts ccc}; # alt+c

# unregister
hotkey::unregister 54 8
hotkey::unregister 56 8

# alt+o to popup window
hotkey::register 0x20 8 {wm withdraw . ; after 100 {wm deiconify .}}
