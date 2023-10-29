#include <cstdio>
#include <cstdarg>
#include "logger.hpp"

static const int BUFFER_LENGTH = 1000;

bool Logger::enabled {true};

void Logger::log_info(const char* message, ...)
{
    if (!enabled)
        return;

    va_list list{};
    va_start(list, message);

    char buffer[BUFFER_LENGTH];
    snprintf(buffer, BUFFER_LENGTH, "INFO: %s\n", message);
    vfprintf(stdout, buffer, list);

    va_end(list);
}

void Logger::log_warning(const char* message, ...)
{
    if (!enabled)
        return;

    va_list list{};
    va_start(list, message);

    char buffer[BUFFER_LENGTH];
    snprintf(buffer, BUFFER_LENGTH, "WARNING: %s\n", message);
    vfprintf(stderr, buffer, list);

    va_end(list);
}

void Logger::log_error(const char* message, ...)
{
    if (!enabled)
        return;

    va_list list{};
    va_start(list, message);

    char buffer[BUFFER_LENGTH];
    snprintf(buffer, BUFFER_LENGTH, "ERROR: %s\n", message);
    vfprintf(stderr, buffer, list);

    va_end(list);
}
