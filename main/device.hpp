#pragma once

namespace device {

// Builds the boot-lifetime runtime and starts its workers once.
// Network adapters can stop and start again; this function does not.
void start_device();

} // namespace device
