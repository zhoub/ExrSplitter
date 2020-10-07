cmake_minimum_required(VERSION 3.8)

project(ExrSplitter VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)

find_path(OPENEXR_INCLUDE_DIR REQUIRED)

find_path(OIIO_INCLUDE_DIR REQUIRED)
find_library(OIIO_LIBRARY REQUIRED)

add_executable(ExrSplitter ExrSplitter.cpp)
target_include_directories(ExrSplitter PRIVATE
	${OPENEXR_INCLUDE_DIR}
	${OIIO_INCLUDE_DIR})
target_link_libraries(ExrSplitter
	${OIIO_LIBRARY})

install(TARGETS ExrSplitter DESTINATION bin)
