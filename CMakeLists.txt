cmake_minimum_required(VERSION 2.8)
project(ps4eye_test)

SET(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")
SET(CMAKE_SHARED_LINKER_FLAGS "-lc++abi")

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
add_library( libusb SHARED IMPORTED )
set_target_properties( libusb PROPERTIES IMPORTED_LOCATION "/usr/local/Cellar/libusb/1.0.19/lib/libusb-1.0.0.dylib")

include_directories(${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS} "/usr/local/Cellar/libusb/1.0.19/include")

set(SOURCE_EXE main.cpp)
set(SOURCE_LIB ps4eye.cpp)

add_library(ps4eye STATIC ${SOURCE_LIB})	# Создание статической библиотеки с именем foo
add_executable(main ${SOURCE_EXE})	# Создает исполняемый файл с именем main

target_link_libraries(main ps4eye libusb ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})

install(TARGETS main DESTINATION "../..")
