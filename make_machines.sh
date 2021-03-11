#!/bin/sh

parallelization="-j 12"
cd ..

if  [ ! -d "pugixml" ] && ! git clone https://github.com/zeux/pugixml.git ; then
        echo >/dev/stderr "Couldn't clone pugixml"
        exit 1
fi

if [ ! -d "ceps" ] && ! git clone https://github.com/cepsdev/ceps.git ; then
	echo >/dev/stderr "Couldn't clone ceps"
	exit 2
fi

cd ceps/core
mkdir bin 2>/dev/null

echo "\033[1;32mBuilding ceps/core\033[0m"
if ! make $parallelization ; then
	echo >/dev/stderr "Failed to make ceps"
	exit 3
fi

cd ../..

if [ ! -d "log4ceps" ] && ! git clone https://github.com/cepsdev/log4ceps.git ; then
	echo >/dev/stderr "Couldn't clone log4ceps"
	exit 4
fi

if [ ! -d "cryptopp" ] && ! git clone https://github.com/weidai11/cryptopp.git ; then
	echo >/dev/stderr "Couldn't clone cryptopp"
	exit 5
fi

cd cryptopp
if ! git checkout CRYPTOPP_5_6_5 ; then
	echo >/dev/stderr "Couldn't checkout tag CRYPTOPP_5_6_5"
	exit 6
fi
echo "\033[1;32mBuilding cryptopp\033[0m"
if ! make $parallelization ; then
	echo >/dev/stderr "Couldn't build cryptopp"
	exit 7
fi

cd ..


cd machines4ceps || exit 9 

mkdir bin 2>/dev/null
echo "\033[1;32mBuilding machines4ceps\033[0m"
if ! make $parallelization TARGET=bin ; then 
	echo >/dev/stderr "Couldn't build machines4ceps"
	exit 10
fi

cd ..
echo
echo "\033[0;32m==>Build Complete<==\033[0m"  
machines4ceps/bin/ceps --version
echo


