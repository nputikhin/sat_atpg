#include "log.h"

#include <iostream>

#if DEBUG
	Logger::LogLevel Logger::s_log_level = Logger::LogLevel::Debug;
#else
	Logger::LogLevel Logger::s_log_level = Logger::LogLevel::Info;
#endif

std::ostream Logger::s_log_stream(std::cout.rdbuf());
