#include "../interfaces/hal.h"
extern "C" HAL* get_hal();
extern "C" void kernel_main() {
    HAL* hal = get_hal();
    hal->print_text("Booting ZeroRing WASM OS v0.1...");
    hal->network_request("{\"action\":\"read_file\",\"file\":\"config.sys\"}");
}