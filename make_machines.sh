#!/bin/sh

echo parallelization="-j 12"
cd ..

if ! git clone https://github.com/zeux/pugixml.git ; then
        echo >/dev/stderr "Couldn't clone pugixml"
        exit 1
fi

if ! git clone https://github.com/cepsdev/ceps.git ; then
	echo >/dev/stderr "Couldn't clone ceps"
	exit 2
fi

cd ceps/core
mkdir bin

if ! make $parallelization ; then
	echo >/dev/stderr "Failed to make ceps"
	exit 3
fi

cd ../..

if ! git clone https://github.com/cepsdev/log4ceps.git ; then
	echo >/dev/stderr "Couldn't clone log4ceps"
	exit 4
fi

if ! git clone https://github.com/weidai11/cryptopp.git ; then
	echo >/dev/stderr "Couldn't clone cryptopp"
	exit 5
fi

cd cryptopp
if ! git checkout CRYPTOPP_5_6_5 ; then
	echo >/dev/stderr "Coudldn't checkout tag CRYPTOPP_5_6_5"
	exit 6
fi

if ! make $parallelization ; then
	echo >/dev/stderr "Couldn't build cryptopp"
	exit 7
fi

cd ..


cd machines4ceps
mkdir bin
if ! make $parallelization TARGET=bin ; then 
	echo >/dev/stderr "Couldn't buuild machines4ceps"
	exit 9
fi

cd ..

echo "################################################################"
machines4ceps/bin/ceps --version
echo "################################################################"



