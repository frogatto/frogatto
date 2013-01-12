cmake_minimum_required(VERSION 2.8)
project(Frogatto)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SDL REQUIRED)
find_package(SDL_mixer REQUIRED)
find_package(SDL_image REQUIRED)
find_package(SDL_ttf REQUIRED)
find_package(ZLIB REQUIRED)

set(CMAKE_THREAD_PREFER_PTHREAD 1)
find_package(Threads REQUIRED)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost COMPONENTS iostreams regex thread system REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${GLEW_INCLUDE_DIR})
include_directories(${SDL_INCLUDE_DIR})
include_directories(${SDL_MIXER_INCLUDE_DIRS})
include_directories(${SDL_MIXER_INCLUDE_DIR})
include_directories(${SDL_IMAGE_INCLUDE_DIR})
include_directories(${SDL_TTF_INCLUDE_DIR})
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(src src/server)

file(GLOB game_src src/*.cpp)
file(GLOB server_src src/server/*.cpp)

add_executable(frogatto ${game_src})
add_executable(frogatto-server ${server_src})

target_link_libraries(frogatto ${Boost_LIBRARIES} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${SDL_LIBRARY} ${SDLIMAGE_LIBRARY} ${SDLTTF_LIBRARY} ${SDLMIXER_LIBRARY} ${ZLIB_LIBRARIES})
target_link_libraries(frogatto-server ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS frogatto frogatto-server DESTINATION bin/)
install(DIRECTORY data images music modules DESTINATION share/frogatto)
