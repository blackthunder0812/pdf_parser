cmake_minimum_required(VERSION 2.8)

project(pdf_parser_service)

set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

#boost
set(Boost_USE_MULTITHREADED ON)
find_package(Boost COMPONENTS log system filesystem thread REQUIRED)

if(Boost_FOUND)
    add_definitions(-DBOOST_LOG_DYN_LINK
        -DBOOST_LOG_ENABLE_CONSOLE_SINK
#        -DBOOST_LOG_ENABLE_FILE_SINK
#        -DBOOST_LOG_ENABLE_FILE_LINE_FUNCTION
        -DBOOST_LOG_SEVERITY_THRESHOLD=info
#        -DBOOST_LOG_FILENAME_PATTERN="log_%Y%m%d_%H:%M:%S.%06N.log"
#        -DBOOST_LOG_ROTATION_SIZE=10*1024*1024
#        -DBOOST_LOG_ROTATION_TIME_POINT=0,0,0
        )
endif()

include_directories(${Boost_INCLUDE_DIRS} "inc")
file(GLOB SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    mupdf mupdf-third
    ${Boost_LIBRARIES}
    Threads::Threads)
