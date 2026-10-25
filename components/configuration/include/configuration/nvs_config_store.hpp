#pragma once

#include "configuration/network_config.hpp"

namespace configuration {

class NvsConfigStore {
    public:
    // Preserves the existing NVS namespace and schema; never erases storage.
    bool load(NetworkConfig& config, bool allow_plaintext = false) const noexcept;
};

} // namespace configuration
