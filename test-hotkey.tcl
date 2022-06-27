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

# register2
hotkey::register2 C-a {puts aaa}; # control+a
hotkey::register2 S-b {puts bbb}; # shift-b
hotkey::register2 A-c {puts ccc}; # alt-c
hotkey::register2 A-space {puts ddd}; # alt-space
hotkey::register2 S-BackSpace {puts eee}; # Shift+backspace
hotkey::register2 s-F1 {puts fff}; # Super+f1

# unregister2
hotkey::unregister2 Control-a
hotkey::unregister2 Shift-b
hotkey::unregister2 Alt-c


