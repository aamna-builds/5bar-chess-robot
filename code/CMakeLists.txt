cmake_minimum_required(VERSION 3.15)
project(5bar CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RAYLIB_PATH "${CMAKE_SOURCE_DIR}/lib/raylib")

add_executable(${PROJECT_NAME} src/main.cpp src/bar.cpp src/chess.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${RAYLIB_PATH}/include
)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${RAYLIB_PATH}/lib
)

if (APPLE)
    target_link_libraries(${PROJECT_NAME}
        raylib
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework IOKit"
        "-framework CoreAudio"
    )
elseif (WIN32)
    target_link_libraries(${PROJECT_NAME}
        raylib
        winmm
        gdi32
        opengl32
    )
endif()
