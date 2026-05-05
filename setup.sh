# One Time setup
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest

# Every time setup
source ./emsdk_env.sh

# Verify setup
emcc --version
