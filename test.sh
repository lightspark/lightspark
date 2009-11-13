#!/bin/sh

if [ -z $1 ]
then
	dir="as3"
else
	dir=$1
fi

testCases=`find $dir -name "*.abc" | sort`

for i in $testCases
do
	./tightspark $i > /dev/null 2> /tmp/out.test
	$AVMSHELL $i > /tmp/out.ref

	cat /tmp/out.test
	grep "FAILED" /tmp/out.test
	if [ $? -eq 0 ]
	then
		grep $i tests.knowfailures
		if [ $? -eq 1 ]
		then
			echo "Test failed on $i"
			exit 1
		fi
	fi

	cmp /tmp/out.test /tmp/out.ref
	if [ $? -ne 0 ]
	then
		grep $i tests.knowfailures
		if [ $? -eq 1 ]
		then
			echo "Differences between reference and tightspark on " $i
			exit 2
		fi
	fi
	rm /tmp/out.test /tmp/out.ref
done

exit 0
