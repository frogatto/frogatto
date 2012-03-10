cmake_minimum_required(VERSION 2.8)
project(Frogatto)

find_package(GLUT)
find_package(OpenGL)
find_package(SDL)
find_package(SDL_mixer)
find_package(SDL_image)
find_package(SDL_ttf)
SET( CMAKE_THREAD_PREFER_PTHREAD 1 )
find_package(Threads)

set( BOOST_COMPONENTS_NEEDED regex thread system )

# The following verifyies that BOOST_ROOT is set properly.
if(NOT BOOST_ROOT AND NOT $ENV{BOOST_ROOT} STREQUAL "")
    FILE( TO_CMAKE_PATH $ENV{BOOST_ROOT} BOOST_ROOT )
    if( NOT EXISTS ${BOOST_ROOT} ) 
        MESSAGE( STATUS  ${BOOST_ROOT} " does not exist. Checking if BOOST_ROOT was a quoted string.." )
        STRING( REPLACE "\"" "" BOOST_ROOT ${BOOST_ROOT} ) 
        if( EXISTS ${BOOST_ROOT} ) 
            MESSAGE( STATUS "After removing the quotes " ${BOOST_ROOT} " was now found by CMake" )
        endif( EXISTS ${BOOST_ROOT})
    endif( NOT EXISTS ${BOOST_ROOT} )

# Save the BOOST_ROOT in the cache
    if( NOT EXISTS ${BOOST_ROOT} ) 
        MESSAGE( WARNING ${BOOST_ROOT} " does not exist." )
    else(NOT EXISTS ${BOOST_ROOT})
        SET (BOOST_ROOT ${BOOST_ROOT} CACHE STRING "Set the value of BOOST_ROOT to point to the root folder of your boost install." FORCE)
        #SET (BOOST_INCLUDEDIR ${BOOST_ROOT})
        #SET (BOOST_LIBRARYDIR ${BOOST_ROOT}/lib)
    endif( NOT EXISTS ${BOOST_ROOT} )

endif(NOT BOOST_ROOT AND NOT $ENV{BOOST_ROOT} STREQUAL "")

if( WIN32 AND NOT BOOST_ROOT )
    MESSAGE( WARNING "Please set the BOOST_ROOT environment variable." )
endif( WIN32 AND NOT BOOST_ROOT )

set(Boost_ADDITIONAL_VERSIONS "1.47" "1.47.0")
set(Boost_DEBUG ON)
set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
FIND_PACKAGE(Boost 1.47.0 COMPONENTS ${BOOST_COMPONENTS_NEEDED})
if( Boost_FOUND )
    include_directories( ${Boost_INCLUDE_DIRS} )
    link_directories( ${Boost_LIBRARY_DIRS} )
endif( Boost_FOUND )

if( SDL_FOUND STREQUAL "YES" )
    MESSAGE( STATUS "SDL FOUND: " ${SDL_INCLUDE_DIR} " : " ${SDL_LIBRARY} )
    #include_directories( ${SDL_INCLUDE_DIR} )
    link_directories( ${SDL_LIBRARY} )
    #add_library( SDL_image.lib SHARED IMPORTED )
    #add_library( SDL_mixer.lib SHARED IMPORTED )
    #add_library( SDL_ttf.lib SHARED IMPORTED )
else( SDL_FOUND STREQUAL "NO" )
    MESSAGE( STATUS "SDL NOT FOUND: " ${SDL_INCLUDE_DIR} " : " ${SDL_LIBRARY} )
endif( SDL_FOUND STREQUAL "YES" )

if( OPENGL_FOUND )
    include_directories( ${OPENGL_INCLUDE_DIR} )
endif( OPENGL_FOUND )

add_subdirectory( src )

if( WIN32 )
    set_target_properties( frogatto PROPERTIES 
        COMPILE_DEFINITIONS "_WIN32_WINNT=0x0501;NOMINMAX;DISABLE_FORMULA_PROFILER"
        COMPILE_FLAGS "/EHsc /GR" 
        LINK_FLAGS "/SUBSYSTEM:WINDOWS" )
    set_target_properties( server PROPERTIES 
        COMPILE_DEFINITIONS "_WIN32_WINNT=0x0501" 
        COMPILE_FLAGS "/EHsc /GR" 
        LINK_FLAGS "/SUBSYSTEM:CONSOLE" ) 
endif( WIN32 )

