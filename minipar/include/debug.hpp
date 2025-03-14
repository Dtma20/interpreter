#ifndef DEBUG_HPP
#define DEBUG_HPP

#ifdef DEBUG_MODE
#include <iostream>
#define LOG_DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl
#else
#define LOG_DEBUG(msg)
#endif

#endif