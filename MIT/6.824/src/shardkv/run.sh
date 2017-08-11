rm *.out

export GOPATH=$HOME/Gitrepo/6.824
for ((i=0; i < 60; i++))
	do
		echo "Testing in $i iteration"
		# go test -run "XXX"
		go test > $i.out | grep -E '^(--- )?FAIL|Passed|^Test:|^ok'
		if (grep -q 'FAIL' $i.out)
		then
			echo "$i failed"
			mv $i.out $i.fail
			break
		else
			echo "$i succeded"
			rm $i.out
		fi
		echo " Done $i"
	done