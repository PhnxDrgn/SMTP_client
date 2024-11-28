# add executables
add_executable(smtp_client
    src/main.c
)

# add includes
target_include_directories(smtp_client
    PUBLIC
        inc
)