rm *.out

for ((i=0; i < 200; i++))
	do
		go test 2>$i.out | grep -P '^(--- )?FAIL|Passed|^Test:|^ok'
		if (grep -q 'FAIL' $i.out)
		then
			echo "$i failed"
			mv $i.out $i.fail
			break
		else
			echo "$i succeded"
			rm $i.out
		fi
		echo -n " $i"
	done