#!/bin/sh
# Useage make_test.sh $module(eg:sever)

if [ $# -lt 1 ]; then
    echo "Useage make_test.sh $module(eg:sever...) [clean]"
    exit
fi

module=$1
cat ./make_template | sed "s/{module}/$module/g" > ./make_${module}

if [ $# -eq 2 ]; then
    make -f make_${module} clean
else
    make -f make_${module}
fi
rm -f ./make_${module}

