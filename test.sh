#!/bin/sh

if [ -z $1 ]
then
	dir="as3"
else
	dir=$1
fi

testCases=`find $dir -name "*.abc" | sort`

total=0
failed=0

rm -f tests.failed

for i in $testCases
do
	./tightspark $i > /dev/null 2> /tmp/out.test
	$AVMSHELL $i > /tmp/out.ref
	total=$(($total+1))

	cat /tmp/out.test
	iserror=""
	grep "Error" /tmp/out.test
	if [ $? -eq 0 ]
	then
		iserror=" Error"
	fi
	grep "FAILED" /tmp/out.test
	if [ $? -eq 0 ]
	then
		grep $i tests.knowfailures
		if [ $? -eq 1 ]
		then
			echo "Test failed on $i"
			failed=$(($failed+1))
			echo $i $iserror >> tests.failed
			continue
		fi
	fi

	cmp /tmp/out.test /tmp/out.ref
	if [ $? -ne 0 ]
	then
		grep $i tests.knowfailures
		if [ $? -eq 1 ]
		then
			echo "Differences between reference and tightspark on " $i
			failed=$(($failed+1))
			echo $i $iserror >> tests.failed
		fi
	fi
	rm /tmp/out.test /tmp/out.ref
done

echo "Failed " $failed " on " $total

exit 0
