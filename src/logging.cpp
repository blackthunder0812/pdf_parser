#include "logging.hpp"

#include <boost/log/core/core.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <fstream>
#include <ostream>

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", boost::log::trivial::severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)

BOOST_LOG_GLOBAL_LOGGER_INIT(logger, boost::log::sources::severity_channel_logger_mt) {
    boost::log::sources::severity_channel_logger_mt<boost::log::trivial::severity_level> logger;

    // add attributes
    logger.add_attribute("LineID", boost::log::attributes::counter<unsigned int>(1));     // lines are sequentially numbered
    logger.add_attribute("TimeStamp", boost::log::attributes::local_clock());             // each log line gets a timestamp

    // specify the format of the log message
    boost::log::formatter formatter = boost::log::expressions::stream
                                      << std::setw(7) << std::right << std::setfill('0') << line_id << std::setfill(' ') << " | "
                                      << boost::log::expressions::format_date_time(timestamp, "%Y-%m-%d %H:%M:%S.%f") << " ["
                                      << std::setw(7) << std::left << boost::log::trivial::severity << "] "
                                      << boost::log::expressions::if_(channel != "") [
                                       boost::log::expressions::stream  << "[" << std::setw(10) << channel << "] "
                                   ]
                                      << "- " << boost::log::expressions::smessage;

#ifdef BOOST_LOG_ENABLE_FILE_SINK
    // file sink
    boost::shared_ptr<boost::log::sinks::text_file_backend> backend =
        boost::make_shared<boost::log::sinks::text_file_backend>(
            boost::log::keywords::file_name = BOOST_LOG_FILENAME_PATTERN,
            boost::log::keywords::rotation_size = BOOST_LOG_ROTATION_SIZE,
            boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(BOOST_LOG_ROTATION_TIME_POINT));
    boost::shared_ptr<boost::log::sinks::synchronous_sink<boost::log::sinks::text_file_backend>> sink = boost::make_shared<boost::log::sinks::synchronous_sink<boost::log::sinks::text_file_backend>>(backend);
    sink->set_formatter(formatter);
    sink->set_filter(severity >= boost::log::trivial::BOOST_LOG_SEVERITY_THRESHOLD);
    boost::log::core::get()->add_sink(sink);
#endif

#ifdef BOOST_LOG_ENABLE_CONSOLE_SINK
    // console sink
    boost::shared_ptr<boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend>> console_sink = boost::make_shared<boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend>>();
    console_sink->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));
    console_sink->set_formatter(formatter);
    console_sink->set_filter(severity >= boost::log::trivial::BOOST_LOG_SEVERITY_THRESHOLD);
    boost::log::core::get()->add_sink(console_sink);
#endif

    return logger;
}
