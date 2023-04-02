project(Runtime)

add_subdirectory(glm)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG(-O3 COMPILER_SUPPORTS_O3)
if(COMPILER_SUPPORTS_O3)
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

CHECK_CXX_COMPILER_FLAG(-g COMPILER_SUPPORTS_G)
if(COMPILER_SUPPORTS_G)
  set(CMAKE_CXX_FLAGS_DEBUG "-g")
endif()
include_directories(${PROJECT_SOURCE_DIR}/../ThirdParty/embree/include)
include_directories(${PROJECT_SOURCE_DIR}/../ThirdParty/embree/common/)
include_directories(${PROJECT_SOURCE_DIR}/../ThirdParty/embree/kernels/)
include_directories(${PROJECT_SOURCE_DIR}/../ThirdParty/stb/)
include_directories(${PROJECT_SOURCE_DIR}/../ThirdParty/glfw/)
include_directories(${PROJECT_SOURCE_DIR}/../ThirdParty/glfw/include/)
include_directories(${PROJECT_SOURCE_DIR}/../ThirdParty/glfw/deps/)
include_directories(${PROJECT_SOURCE_DIR}/../ThirdParty/glad/include/)
include_directories(${PROJECT_SOURCE_DIR}/../Runtime)
# include_directories(${PROJECT_SOURCE_DIR}/lib/nlohmann/)

# list(APPEND CMAKE_PREFIX_PATH "../ThirdParty/embree/include")


file(GLOB_RECURSE _source_list ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.c ${PROJECT_SOURCE_DIR}/*.cxx ${PROJECT_SOURCE_DIR}/*.hpp ${PROJECT_SOURCE_DIR}/*.h)

foreach(_source IN ITEMS ${_source_list})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "${CMAKE_SOURCE_DIR}" "" _group_path "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

add_executable(${PROJECT_NAME} ${_source_list})

ADD_DEFINITIONS(-DEMBREE_TARGET_AVX2=1)
ADD_DEFINITIONS(-D__AVX2__=1)
ADD_DEFINITIONS(-DTASKING_TBB=1)
#ADD_DEFINITIONS(-DSTB_IMAGE_WRITE_IMPLEMENTATION=1)
#ADD_DEFINITIONS(-DSTB_IMAGE_IMPLEMENTATION=1)

target_link_libraries(${PROJECT_NAME} embree math simd sys tasking glfw glad Threads::Threads)
