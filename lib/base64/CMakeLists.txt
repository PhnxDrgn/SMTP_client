project(base64)

add_library(base64
    src/base64.c
)

target_include_directories(base64
    PUBLIC
        inc
)