cmake_minimum_required(VERSION 3.15)

set (RVDDM_SRC
    events.cc
    )

include_directories(
    ${RVDDM_INC_DIRS}
)

add_library(rvddm
    STATIC
    ${RVDDM_SRC})
