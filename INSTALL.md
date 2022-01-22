# Installation

## How to build the thing

### Prerequisites:
* Linux (Kernel Version >= 2.6)
* g++ (Version >= 8)
* bison (Version >= __3.0__)
* make

### Using the Build Script
 * git clone https://github.com/cepsdev/machines4ceps.git
 * cd machines4ceps
 * ./make_machines

:warning: Build parallelization (make's -j option) is set to 12 in the build script. Set the variable __parallelization__ to a value that fits your needs better. If you intend to build on a machine where RAM is an issue you should set __parallelization__ to 1.  

:warning: __Important Remark:__ __machines4ceps__ is maintained in lockstep with __ceps__ which is __not__ referenced as a git submodule. 
If you pull changes into your local copy of machines4ceps make sure that you have the latest version of the __ceps__ repo too. 

### Build Manually:
All repositories need to be in the same directory as machines4ceps.
* Clone pugixml:
  * git clone https://github.com/zeux/pugixml.git
* Clone and build ceps:
  * git clone https://github.com/cepsdev/ceps.git
  * cd ceps/core
  * mkdir bin
  * make
  * cd ../..
* Clone log4ceps
  * git clone https://github.com/cepsdev/log4ceps.git
* Clone, checkout 5.x branch, and build cryptopp:
  * git clone https://github.com/weidai11/cryptopp.git 
  * cd cryptopp
  * git checkout CRYPTOPP_5_6_5
  * make
  * cd ..
* Clone and build  machines4ceps
  * git clone https://github.com/cepsdev/machines4ceps.git
  * cd machines4ceps
  * mkdir bin
  * make TARGET=bin

This should produce a binary called __ceps__ in the directory machines4ceps/bin.

### Tested Platforms

* Ubuntu 18.04 (bionic)
* Ubuntu 20.04 (focal)
* Raspberry Pi OS (Kernel >= 5.10)
