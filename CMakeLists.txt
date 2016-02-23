cmake_minimum_required(VERSION 2.8.4)
project(Myfind)

set(SOURCE_FILES src/main.c)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ${DOXYGEN_FOUND})

if(BUILD_DOCUMENTATION)
    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif()

    set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/doxygen.dcf)
    set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${doxyfile_in} ${doxyfile} @ONLY)

    add_custom_target(doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION share/doc)
endif()

set(TEST_FIND_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)

add_custom_target(find_with_test
        COMMAND /bin/bash ${TEST_FIND_DIR}/test-find.sh -t ${CMAKE_CURRENT_SOURCE_DIR}/output/Debug/myfind -r ${TEST_FIND_DIR}/bic-myfind
        DEPENDS myfind)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra -Wstrict-prototypes -pedantic -fno-common -O3 -g -std=gnu11")

add_executable (myfind ${SOURCE_FILES})