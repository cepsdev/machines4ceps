# machines4ceps
Engine for UML2ish state charts. Supports composite states, orthogonal regions, events, actions, init/exit etc. Small, fast, portable. Handles large state spaces (100k+). Monitoring, tracing, and a variety of communication protocols e.g. websockets, CAN field bus etc.

## Installation

### Prerequisites:
* ceps
* log4ceps
* cryptopp (5.x):
  * Clone https://github.com/weidai11/cryptopp.git (sm4ceps build script expects cryptopp to be in the same directory as the sm4ceps repo) 
  * Change working directory to cryptopp
  * Checkout tag CRYPTOPP_5_6_5 (git checkout CRYPTOPP_5_6_5)
  * run make
  
