#!/bin/bash

#DO NOT REMOVE THE FOLLOWING TWO LINES
git add $0 >> .local.git.out
git commit -a -m "Lab 2 commit" >> .local.git.out
git push >> .local.git.out || echo


#Your code here

SCORE=0

COUNT=0
ITER=1
while egrep -q ".{$ITER,}" $1; do
	let COUNT=COUNT+1
	let ITER=ITER+1
done
let SCORE=SCORE+COUNT

if [ $COUNT -gt 32 ] || [ $COUNT -lt 6 ]; then
	echo "Error: Password length invalid."
else

	if egrep -q '[#@$%+]' $1; then
		let SCORE=SCORE+5
	fi
	if egrep -q '[0-9]' $1; then
		let SCORE=SCORE+5
	fi
	if egrep -q '[A-Za-z]' $1; then
		let SCORE=SCORE+5
	fi
	if egrep -q '(\w)(\1{1,})' $1; then
		let SCORE=SCORE-10
	fi
	if egrep -q '[a-z]{3,}' $1; then
		let SCORE=SCORE-3
	fi
	if egrep -q '[A-Z]{3,}' $1; then
		let SCORE=SCORE-3
	fi
	if egrep -q '[0-9]{3,}' $1; then
		let SCORE=SCORE-3
	fi

	echo "Password Score: $SCORE"
fi
