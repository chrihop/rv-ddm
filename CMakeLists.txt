cmake_minimum_required(VERSION 3.13)

project(rv-ddm)

set (CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
    boost_sml
    URL "https://github.com/boost-ext/sml/archive/refs/tags/v1.1.8.tar.gz"
    URL_HASH SHA256=d2626b2fd249fa0788ca364a2f8dcb4762dd72195f86d43370e4cad4c02262c5
)

FetchContent_GetProperties(boost_sml)
if(NOT boost_sml_POPULATED)
    message(STATUS "Fetching Boost.SML ...")
    FetchContent_Populate(boost_sml)
    add_subdirectory(${boost_sml_SOURCE_DIR} ${boost_sml_BINARY_DIR})
endif()

set (RVDDM_TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (RVDDM_OBJ_DIR ${CMAKE_CURRENT_BINARY_DIR})

# -- src --

# --

# -- tests --
add_subdirectory(tests)
# --
