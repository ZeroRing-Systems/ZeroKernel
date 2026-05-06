# One Time setup
sudo apt update
sudo apt install -y emscripten

source /usr/share/emscripten/emsdk_env.sh

# Verify setup
emcc --version
