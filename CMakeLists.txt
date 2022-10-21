cmake_minimum_required(VERSION 3.17)
project(solopong)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_executable(solopong main.c)
add_subdirectory(./QuickGame/ QuickGame)

target_link_libraries(solopong PUBLIC QuickGame)
target_include_directories(solopong PUBLIC QuickGame/include)

if (PLATFORM_PSP)
    create_pbp_file(
        TARGET solopong
        TITLE Pong Solo PSP
	ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets/sprites/ball.png"
        BUILD_PRX ON
    )
endif()

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})