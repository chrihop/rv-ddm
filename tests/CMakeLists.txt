cmake_minimum_required(VERSION 3.13)

include_directories(
    ${RVDDM_TOP_DIR}/inc
    ${boost_sml_SOURCE_DIR}/include
)

add_executable(rv-sm
    rv-sm.cc)

# -- unit test --
FetchContent_Declare(
    googletest
    URL "https://github.com/google/googletest/archive/refs/tags/v1.13.0.tar.gz"
    URL_HASH SHA256=ad7fdba11ea011c1d925b3289cf4af2c66a352e18d4c7264392fead75e919363
)

set(BUILD_GMOCK ON)
set(INSTALL_GTEST OFF)
message(STATUS "Fetching googletest ...")
FetchContent_MakeAvailable(googletest)

enable_testing()
