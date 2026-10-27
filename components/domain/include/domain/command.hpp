#pragma once

#include "domain/types.hpp"

#include <cstdint>

namespace domain {

enum class CommandType {
    set_output,
    stop
};

enum class CommandSource {
    local,
    remote
};

// Inbound actuator request. Deadlines use the device monotonic clock.
// Transport adapters assign that clock; this type does not carry wall time.
struct Command {
    std::uint64_t id{0};
    std::uint32_t sequence{0};
    std::uint32_t identity{0};
    CommandType type{CommandType::set_output};
    CommandSource source{CommandSource::local};
    bool enabled{false};
    Milliseconds expires_at{0};
    Milliseconds max_duration_ms{0};
};

enum class CommandStatus {
    accepted,
    rejected_invalid,
    rejected_expired,
    rejected_duplicate,
    rejected_stale_sequence,
    rejected_unauthorized,
    rejected_full,
    applied,
    not_applied,
    dropped_by_stop,
    expired_before_apply,
    superseded
};

// Synchronous admission and later control acknowledgement share this record.
struct CommandResult {
    std::uint64_t id{0};
    std::uint32_t sequence{0};
    std::uint32_t identity{0};
    CommandType type{CommandType::set_output};
    CommandSource source{CommandSource::local};
    CommandStatus status{CommandStatus::rejected_invalid};
    Milliseconds at{0};
};

struct CommandLimits {
    Milliseconds max_run_ms{0};
    Milliseconds max_ttl_ms{0};
};

enum class CommandValidation {
    ok,
    invalid,
    expired,
    unauthorized
};

namespace command_defaults {

inline constexpr Milliseconds max_ttl_ms = 15000;

} // namespace command_defaults

// Pure local checks: shape, range, deadline, and whether a remote source is allowed.
// Duplicate and queue-capacity checks need admission state and stay in application code.
CommandValidation validate_command(const Command& command, Milliseconds now, CommandLimits limits,
                                   bool remote_commands_enabled) noexcept;

} // namespace domain
