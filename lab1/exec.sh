#!/usr/local/cs/execline-2.1.4.5/bin/execlineb

redirfd -r 0 /dev/random
pipeline { head -10000 }
pipeline { sort }
redirfd -w 1 output.txt
cat