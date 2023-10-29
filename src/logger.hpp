// Logging helper utilities
// Date: 28-Oct-23
// Author: Daniel Walls
// RedID: 825776127

#ifndef ASSIG2_LOGGER_HPP
#define ASSIG2_LOGGER_HPP

// Helper macro for logging TO-DO messages.
#define TODO(message) Logger::log_warning("TODO - %s", message)

// Functions that provide a single-point of access for logging, used globally throughout the program.
namespace Logger
{
    extern bool enabled;

    void log_info(const char* message, ...);
    void log_warning(const char* message, ...);
    void log_error(const char* message, ...);
}

#endif // ASSIG2_LOGGER_HPP