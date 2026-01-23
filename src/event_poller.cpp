// Define NOMINMAX macro to avoid conflict between Windows max macro and std::max
#define NOMINMAX

#include <tang/event_poller.h>

// Platform-specific implementations are in their respective files
// Windows: event_poller_win.cpp
// Linux: event_poller_linux.cpp
// This file only contains platform-independent code or common implementations