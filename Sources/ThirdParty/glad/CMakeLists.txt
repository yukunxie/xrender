cmake_minimum_required(VERSION 3.15)

project ( glad )

set( STATIC_LIB_SOURCES
    src/glad.c
)

include_directories(${PROJECT_SOURCE_DIR}/include/)

set( STATIC_LIB_HEADERS
    include/glad/glad.h
    include/KHR/khrplatform.h
)

add_library( glad STATIC 
    ${STATIC_LIB_SOURCES} ${STATIC_LIB_HEADERS}
)
