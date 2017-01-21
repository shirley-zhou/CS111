#!/usr/local/cs/execline-2.1.4.5/bin/execlineb

redirfd -r 0 /dev/random
redirfd -w 1 a
head -10000