#!/bin/bash
if $1=="-FIXED_CTRL_C"
then
	shift
else 
	$0 -FIXED_CTRL_C ${@:1}
	exit 0 
fi

cd build64
./FractalClient ${@:1}
cd ..
