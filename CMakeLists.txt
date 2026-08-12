cmake_minimum_required(VERSION 3.16)
project(BankManagementSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED mysqlclient)

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(bank_app ${SOURCES})

target_include_directories(bank_app PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${MYSQL_INCLUDE_DIRS}
)

target_link_libraries(bank_app PRIVATE ${MYSQL_LIBRARIES} pthread)
target_compile_options(bank_app PRIVATE ${MYSQL_CFLAGS_OTHER})
