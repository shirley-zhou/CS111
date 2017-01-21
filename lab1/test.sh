#!/bin/bash

#test invalid cases for error messages
./simpsh --verbose --rdonly input.txt a.txt --wronly output.txt b.txt --command 0 1 1 invalidcommand
if [ $? != 0 ];
then echo "failed test 1";
fi

#check simple valid case
./simpsh --verbose --profile --rdonly input.txt --wronly output.txt --wronly error.txt --rdonly input2.txt --wronly output2.txt --wronly error2.txt --command 0 1 2 sort --command 3 4 5 cat
if [ $? != 0 ];
then echo "failed test 2";
fi

#check valid case with opt args to command
./simpsh --verbose --rdonly input.txt --wronly output.txt --wronly error.txt --command 0 1 2 tr A-Z a-z
if [ $? != 0 ];
then echo "failed test 3";
fi

#check more complex case with pipes and other file flags, invalid file
./simpsh --verbose --rdonly a --pipe --pipe --creat --trunc --wronly output --creat --append --wronly error --command 0 2 6 tr A-Z a-z --command 1 5 6 cat --wait
#				0	12	34				5				6
if [ $? != 1 ];
then echo "failed test 4";
fi

#valid file
./simpsh --verbose --rdonly input.txt --pipe --pipe --creat --trunc --wronly c --creat --append --wronly d --command 4 5 6 tr A-Z a-z --command 0 2 6 sort --command 1 4 6 cat b - 
if [ $? != 0 ];
then echo "failed test 5";
fi

#close failure
./simpsh --verbose --rdonly input.txt --close 3 --command 0 0 0 echo a
if [ $? != 1 ];
then echo "failed test 6";
fi

#test signal handling
./simpsh --rdonly a --wronly b --wronly c --command 0 1 2 cat a --catch 11 --abort
if [ $? != 11 ];
then echo "failed test 7";
fi

#signal handling
./simpsh --abort
if [ $? != 139 ];
then echo "failed test 8";
fi

#signal handling
./simpsh --verbose --ignore 11 --abort --rdonly input.txt
if [ $? != 0 ];
then echo "failed test 9";
fi

#signal handling
./simpsh --verbose --ignore 11 --default 11 --abort --rdonly input.txt
if [ $? != 139 ];
then echo "failed test 10";
fi

--------------------------------------------------------------
#Timing, for lab 1c
./simpsh --verbose --profile --rdonly /dev/random --pipe --pipe --wronly output.txt --wronly error.txt --command 0 2 6 head -10000 --command 1 4 6 sort --command 3 5 6 cat --wait

./simpsh --verbose --profile --rdonly poem.txt --pipe --trunc --wronly c --creat --append --wronly d --command 0 2 4 tr A-Z a-z --command 1 3 4 cat --wait

./simpsh --verbose --profile --rdonly output.txt --wronly c --wronly error.txt --command 0 1 2 wc -c --wait