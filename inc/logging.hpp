#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/global_logger_storage.hpp>

//#define BOOST_LOG_ENABLE_FILE_SINK

//#define BOOST_LOG_ENABLE_CONSOLE_SINK

//#define BOOST_LOG_ENABLE_FILE_LINE_FUNCTION

#ifndef BOOST_LOG_FILENAME_PATTERN
#define BOOST_LOG_FILENAME_PATTERN "log_%Y%m%d_%H:%M:%S.%06N.log"
#endif

#ifndef BOOST_LOG_ROTATION_SIZE
#define BOOST_LOG_ROTATION_SIZE 10*1024*1024
#endif

#ifndef BOOST_LOG_ROTATION_TIME_POINT
#define BOOST_LOG_ROTATION_TIME_POINT 0,0,0
#endif

#ifndef BOOST_LOG_SEVERITY_THRESHOLD
#define BOOST_LOG_SEVERITY_THRESHOLD warning
#endif

BOOST_LOG_GLOBAL_LOGGER(logger, boost::log::sources::severity_channel_logger_mt<boost::log::trivial::severity_level>)

#ifdef BOOST_LOG_ENABLE_FILE_LINE_FUNCTION
#define LOG(logger, severity) BOOST_LOG_SEV(logger::get(), boost::log::trivial::severity) \
    << "(" << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << ") "
#define LOG_CHANNEL(logger, channel, severity) BOOST_LOG_CHANNEL_SEV(logger::get(), channel, boost::log::trivial::severity) \
    << "(" << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << ") "
#else
#define LOG(logger, severity) BOOST_LOG_SEV(logger::get(), boost::log::trivial::severity)
#define LOG_CHANNEL(logger, channel, severity) BOOST_LOG_CHANNEL_SEV(logger::get(), channel, boost::log::trivial::severity)
#endif

// ===== log macros =====
#define LOG_TRACE   LOG(logger, trace)
#define LOG_DEBUG   LOG(logger, debug)
#define LOG_INFO    LOG(logger, info)
#define LOG_WARNING LOG(logger, warning)
#define LOG_ERROR   LOG(logger, error)
#define LOG_FATAL   LOG(logger, fatal)

#define LOG_CHANNEL_TRACE(channel)   LOG_CHANNEL(logger, channel, trace)
#define LOG_CHANNEL_DEBUG(channel)   LOG_CHANNEL(logger, channel, debug)
#define LOG_CHANNEL_INFO(channel)    LOG_CHANNEL(logger, channel, info)
#define LOG_CHANNEL_WARNING(channel) LOG_CHANNEL(logger, channel, warning)
#define LOG_CHANNEL_ERROR(channel)   LOG_CHANNEL(logger, channel, error)
#define LOG_CHANNEL_FATAL(channel)   LOG_CHANNEL(logger, channel, fatal)
