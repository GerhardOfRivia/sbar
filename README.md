# SBAR

Written in pure C without any extern programs being executed and only reads from files most of the time. SBAR is meant to be a simple alternative to Bash scripts using the xsetroot -name $status notion (inefficient) and Conky, that leads to big scripts which pull in unneeded dependencies.(bloated and written in C++). C is much more efficient.

Test it Out on Linux
---

`git clone https://github.com/AsynchronousGillz/SBAR.git`
`cd SBAR`
`make`

Something similar
---
slstatus is a suckless and lightweight status monitor for window managers which use WM_NAME as statusbar (e.g. DWM). [slststus](https://git.nulltime.net/slstatus)
