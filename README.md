# machines4ceps
Engine for UML2ish state charts. Supports composite states, orthogonal regions, events, actions, init/exit etc. Small, fast, portable. Handles large state spaces (100k+). Monitoring, tracing, and a variety of communication protocols e.g. websockets, CAN field bus etc.

## Installation

### Prerequisites:
### Build Steps:
All repositories need to be in the same directory as machines4ceps.
* Clone pugixml:
  * git clone https://github.com/zeux/pugixml.git
* Clone and build ceps:
  * git clone https://github.com/cepsdev/ceps.git
  * cd ceps/core
  * make
  * cd ../..
* Clone log4ceps
  * git clone https://github.com/cepsdev/log4ceps.git
* Clone and build cryptopp:
  * git clone https://github.com/weidai11/cryptopp.git 
  * cd cryptopp
  * git checkout CRYPTOPP_5_6_5
  * make
  
