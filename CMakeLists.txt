cmake_minimum_required(VERSION 3.28)
project(viper LANGUAGES C CXX)

find_package(PkgConfig REQUIRED)
pkg_search_module(LIBUSB REQUIRED libusb-1.0)

pkg_search_module(RAYLIB REQUIRED libraylib)

pkg_search_module(ZEROMQ REQUIRED zeromq)


# Add the Homebrew library path if not found automatically
if (LIBUSB_FOUND)
    message(STATUS "libusb-1.0 found: ${LIBUSB_LIBRARIES}")
else()
    message(WARNING "libusb-1.0 not found, adding fallback paths")
    set(LIBUSB_INCLUDE_DIRS "/opt/homebrew/Cellar/libusb/1.0.27/include/libusb-1.0")
    set(LIBUSB_LIBRARIES "/opt/homebrew/Cellar/libusb/1.0.27/lib/libusb-1.0.dylib")
endif()

# Add the Homebrew library path if not found automatically
if (RAYLIB_FOUND)
    message(STATUS "raylib found: ${RAYLIB_LIBRARIES}")
else()
    message(WARNING "raylib not found, adding fallback paths")
    set(RAYLIB_INCLUDE_DIRS "/opt/homebrew/opt/raylib/include")
    set(RAYLIB_LIBRARIES "/opt/homebrew/opt/raylib/lib/libraylib.5.5.0.dylib")
endif()

# Add the Homebrew library path if not found automatically
if (ZEROMQ_FOUND)
    message(STATUS "zeromq found: ${ZEROMQ_LIBRARIES}")
else()
    message(WARNING "zeromq not found, adding fallback paths")
    set(ZEROMQ_INCLUDE_DIRS "/opt/homebrew/opt/zeromq/include")
    set(ZEROMQ_LIBRARIES "/opt/homebrew/opt/zeromq/lib/libzmq.5.dylib")
endif()

set(CMAKE_CXX_STANDARD 17)
#set(LIBUSB_INCLUDE_DIR /opt/homebrew/Cellar/libusb/1.0.27/include)
#set(LIBUSB_LIB_DIR /opt/homebrew/Cellar/libusb/1.0.27/lib)
#
link_directories(/opt/homebrew/Cellar/libusb/1.0.27/lib)
include_directories(/opt/homebrew/Cellar/libusb/1.0.27/include/libusb-1.0)


link_directories("/opt/homebrew/opt/zeromq/lib")
include_directories("/opt/homebrew/opt/zeromq/include")

link_directories("/opt/homebrew/opt/raylib/lib")
include_directories("/opt/homebrew/opt/raylib/include")


add_executable(viper main.cpp viper_ui.h viper_ui.cpp viper_usb.h viper_usb.cpp ViperInterface.h viper_queue.h viper_queue.cpp
        ZMQServer.cpp
        ZMQServer.hpp
        SensorData.cpp
        SensorData.hpp
        PointCloudGUI.cpp
        PointCloudGUI.hpp)
# VPif.h VPif.cpp VPcmdDefs.h

# Include libusb headers
target_include_directories(viper PRIVATE ${LIBUSB_INCLUDE_DIRS} ${ZEROMQ_INCLUDE_DIRS} ${RAYLIB_INCLUDE_DIRS})

# Link libusb libraries
target_link_libraries(viper PRIVATE ${LIBUSB_LIBRARIES} pthread ${ZEROMQ_LIBRARIES} ${RAYLIB_LIBRARIES})

# Pass additional compiler options if necessary
target_compile_options(viper PRIVATE ${LIBUSB_CFLAGS_OTHER} ${RAYLIB_CFLAGS_OTHER})