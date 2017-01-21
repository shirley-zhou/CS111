#!/usr/local/cs/execline-2.1.4.5/bin/execlineb

redirfd -r 0 poem.txt
pipeline { tr A-Z a-z }
redirfd -w 1 c
redirfd -w 2 d
cat
