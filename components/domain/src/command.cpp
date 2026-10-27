#include "domain/command.hpp"

namespace domain {

namespace {

bool supported_type(CommandType type) noexcept {
    switch (type) {
    case CommandType::set_output:
    case CommandType::stop:
        return true;
    }
    return false;
}

bool supported_source(CommandSource source) noexcept {
    switch (source) {
    case CommandSource::local:
    case CommandSource::remote:
        return true;
    }
    return false;
}

} // namespace

CommandValidation validate_command(const Command& command, Milliseconds now, CommandLimits limits,
                                   bool remote_commands_enabled) noexcept {
    if (limits.max_run_ms <= 0 || limits.max_ttl_ms <= 0 || now < 0 || command.expires_at < 0 ||
        command.max_duration_ms < 0 || command.id == 0 || command.sequence == 0 ||
        command.identity == 0 || !supported_type(command.type) ||
        !supported_source(command.source)) {
        return CommandValidation::invalid;
    }
    if (command.type == CommandType::set_output && command.enabled &&
        (limits.max_run_ms <= 1 || command.max_duration_ms <= 0 ||
         command.max_duration_ms >= limits.max_run_ms)) {
        // Duration must end before TimedOutput's runtime fault line.
        return CommandValidation::invalid;
    }
    if (command.source == CommandSource::remote && !remote_commands_enabled) {
        return CommandValidation::unauthorized;
    }
    if (command.expires_at <= now) {
        return CommandValidation::expired;
    }
    if (command.expires_at - now > limits.max_ttl_ms) {
        return CommandValidation::invalid;
    }
    return CommandValidation::ok;
}

} // namespace domain
