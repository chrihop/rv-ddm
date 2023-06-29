cmake_minimum_required(VERSION 3.13)

project(rv-ddm)

set (CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
    boost_sml
    URL "https://github.com/boost-ext/sml/archive/refs/tags/v1.1.8.tar.gz"
    URL_HASH SHA256=d2626b2fd249fa0788ca364a2f8dcb4762dd72195f86d43370e4cad4c02262c5
    DOWNLOAD_EXTRACT_TIMESTAMP true
)

message(STATUS "Fetching Boost.SML ...")
FetchContent_MakeAvailable(boost_sml)

set (RVDDM_TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (RVDDM_OBJ_DIR ${CMAKE_CURRENT_BINARY_DIR})

set (RVDDM_INC_DIRS
    ${RVDDM_TOP_DIR}/inc
    ${boost_sml_SOURCE_DIR}/include
    )

# -- src --
add_subdirectory(src)
# --

# -- tests --
add_subdirectory(tests)
# --
