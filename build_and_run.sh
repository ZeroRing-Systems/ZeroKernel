#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
KERNEL_DIR="$ROOT_DIR/ZeroKernel"
CLOUD_DIR="$ROOT_DIR/ZeroRing-Cloud"
KERNEL_BUILD_DIR="$KERNEL_DIR/build"
CLOUD_BUILD_DIR="$CLOUD_DIR/build"
PUBLIC_WASM_DIR="$CLOUD_DIR/public/wasm"
KERNEL_WASM_FILE="$KERNEL_BUILD_DIR/kernel.wasm"
PUBLIC_WASM_FILE="$PUBLIC_WASM_DIR/kernel.wasm"
SERVER_BIN="$CLOUD_BUILD_DIR/server"
HTTP_PORT="8000"
WS_PORT="8080"

USE_PG="OFF"
for arg in "$@"; do
    case "$arg" in
        --postgres) USE_PG="ON" ;;
    esac
done

mkdir -p "$PUBLIC_WASM_DIR"
rm -rf "$KERNEL_BUILD_DIR" "$CLOUD_BUILD_DIR"
mkdir -p "$KERNEL_BUILD_DIR" "$CLOUD_BUILD_DIR"

echo "[1/4] Building ZeroKernel to WebAssembly..."
(
  cd "$KERNEL_BUILD_DIR"
  emcmake cmake "$KERNEL_DIR"
  cmake --build . --parallel "$(nproc)"
)

if [[ ! -f "$KERNEL_WASM_FILE" ]]; then
  echo "Error: expected WebAssembly output at $KERNEL_WASM_FILE" >&2
  exit 1
fi

cp "$KERNEL_WASM_FILE" "$PUBLIC_WASM_FILE"
echo "    -> Copied kernel.wasm to $PUBLIC_WASM_DIR"

echo "[2/4] Building ZeroRing-Cloud backend (USE_POSTGRES=$USE_PG)..."
(
  cd "$CLOUD_BUILD_DIR"
  cmake "$CLOUD_DIR" -DUSE_POSTGRES="$USE_PG"
  cmake --build . --parallel "$(nproc)"
)

if [[ ! -x "$SERVER_BIN" ]]; then
  echo "Error: expected server binary at $SERVER_BIN" >&2
  exit 1
fi

echo "[3/4] Starting web server and backend..."
trap 'kill "$SERVER_PID" "$HTTP_PID" 2>/dev/null || true' EXIT INT TERM

"$SERVER_BIN" 2>&1 | tee "$CLOUD_DIR/server.log" &
SERVER_PID=$!

python3 -m http.server "$HTTP_PORT" --directory "$CLOUD_DIR/public" >"$CLOUD_DIR/http.log" 2>&1 &
HTTP_PID=$!

sleep 2

echo "[4/4] ZeroRing is running."
echo ""
echo "  Frontend:  http://localhost:$HTTP_PORT"
echo "  Backend:   ws://localhost:$WS_PORT"
if [[ "$USE_PG" == "ON" ]]; then
  echo "  Database:  PostgreSQL (set ZERORING_DB for custom conninfo)"
else
  echo "  Database:  In-memory VFS (pass --postgres for PostgreSQL)"
fi
echo ""
echo "Press Ctrl+C to stop."
wait "$SERVER_PID" "$HTTP_PID"
