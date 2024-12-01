cmake_minimum_required(VERSION 3.16.3)

# set project name
project(smtp_client)

find_package(OpenSSL REQUIRED)

# sub directories
add_subdirectory(lib)

# add executables
add_executable(smtp_client
    src/main.c
)

# link libaraies
target_link_libraries(smtp_client
    OpenSSL::SSL
    base64
)

# add includes
target_include_directories(smtp_client
    PUBLIC
        inc
)