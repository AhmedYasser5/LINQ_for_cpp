#!/bin/bash
C=0; D=0; R=0; A=0
while [ $# -gt 0 -a $A -eq 0 -a "${1:0:1}" == "-" ]
do
	i=1
	while [ "${1:${i}:1}" ]; do
		CHAR=${1:${i}:1}
		if [ "$CHAR" == "c" ]; then C=1
		elif [ "$CHAR" == "d" ]; then D=1
		elif [ "$CHAR" == "r" ]; then R=1
		elif [ "$CHAR" == "a" ]; then A=1
		else
			printf "This program compiles c/cpp files using make and runs the\
 executable file while calculating the elapsed time.
The default build has debugging configurations.\n
./compile_and_run.sh [options] [-a [arg1 arg2 ...]]\n
Options:
\t-c\tcleans (deletes) already built files that have the configurations given (debug or release)
\t-r\tbuilds using release configurations
\t-d\tbuilds using debug configurations, and runs the debugger
\t-h\tshows this help message\n
Arguments:
\t-a\tsends the following parameters as arguments to main function (this should be the last argument)\n"
			exit 0
		fi
		i=$(($i + 1))
	done
	shift
done
if [ $C -eq 1 ]; then
	make RELEASE=$R clean
fi
if [ $D -eq 1 ]; then
	make debug
	exit 0
fi
if [ $A -eq 0 ]; then
	shift $#
fi
make RELEASE=$R all
FILENAME=$(make RELEASE=$R getTarget)
echo --------------------------------------------------
\time -f "\n--------------------------------------------------\n\
Elapsed Time: %e sec\nCPU Percentage: %P" $FILENAME "$@"
exit $?
