cmake_minimum_required(VERSION 2.8.4)
project(Find)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra -Wstrict-prototypes -pedantic -fno-common -O3 -g -std=gnu11")

add_executable (find src/main.c src/helpers.c src/helpers.h)