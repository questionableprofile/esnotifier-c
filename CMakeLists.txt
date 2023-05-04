cmake_minimum_required(VERSION 3.17)

project(esnotifier-native C)
add_compile_options(-Werror=vla -Wall)

add_executable(notifier
        src/main.c
        src/util.c
        src/server.c
        src/http.c
        src/telegram.c
        src/eso.c
        src/config.c
        src/file-saver.c
)
set_property(TARGET notifier PROPERTY C_STANDARD 11)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

include(FindPkgConfig)
pkg_check_modules(telebot REQUIRED IMPORTED_TARGET telebot)
include_directories(${telebot_INCLUDE_DIRS})
link_directories(${telebot_LIBRARY_DIRS})
find_package(json-c CONFIG)
target_link_libraries(notifier PRIVATE json-c::json-c PRIVATE telebot)