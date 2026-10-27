#pragma once

#include "configuration/network_config.hpp"

namespace configuration {

class NvsConfigStore {
    public:
    // Reads the espkit namespace. Never erases or rewrites NVS.
    // Applies migrate_settings in memory. A false return leaves `config` unchanged.
    bool load(NetworkConfig& config, bool allow_plaintext = false) const noexcept;
};

} // namespace configuration
