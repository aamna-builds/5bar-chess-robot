cmake_minimum_required(VERSION 3.15)
project(5bar CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RAYLIB_PATH "${CMAKE_SOURCE_DIR}/lib/raylib")
set(SERIAL_PATH "${CMAKE_SOURCE_DIR}/lib/serial")

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/bar.cpp
    src/chess.cpp 
    src/stockfish.cpp
    src/ui.cpp
    lib/serial/serialib.cpp
)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${RAYLIB_PATH}/include
)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SERIAL_PATH}
)

if (APPLE)
    target_link_libraries(${PROJECT_NAME}
        ${RAYLIB_PATH}/lib/libraylib.a
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
