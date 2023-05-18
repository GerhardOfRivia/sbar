# SBAR

[![License](https://img.shields.io/:license-mit-blue.svg)](https://badges.mit-license.org)

Written in pure C without any extern programs being executed and only reads from files most of the time. SBAR is meant to be a simple alternative to Bash scripts using the xsetroot -name $status notion (inefficient) and Conky, that leads to big scripts which pull in unneeded dependencies.(bloated and written in C++). C is much more efficient.

dwm’s status bar text can be set using the xsetroot -name $status notion. This very well leads to big scripts, which pull in unneeded dependencies. One solution for this is to write everything in C. C is much more efficient. This page will give you a barebone dwmstatus project and show examples on how to extend it to your needs.

---

![Desktop](0.png)

---

Test it Out on Linux
---

`make`
