#include "networking/sntp_time.hpp"

#include "esp_sntp.h"
#include "sdkconfig.h"

#include <ctime>

namespace networking {

void SntpTime::begin() noexcept {
#if CONFIG_ESPKIT_NETWORK_ENABLED
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
#endif
}

bool SntpTime::ready() const noexcept {
    // Certificate checks use wall time; local safety uses the monotonic clock.
    constexpr std::time_t minimum_tls_time = 1704067200; // 2024-01-01 UTC
    return std::time(nullptr) >= minimum_tls_time;
}

} // namespace networking
