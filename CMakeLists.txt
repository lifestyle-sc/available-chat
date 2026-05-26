cmake_minimum_required(VERSION 3.14)
project(example CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Disables compiler-specific "shortcuts"
enable_testing()

add_subdirectory(src)
add_subdirectory(groups/availablecommon)
add_subdirectory(groups/availableserver)
add_subdirectory(groups/availableclient)

add_custom_target(
    check-format ALL
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/format.sh --verify-only ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/groups
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Verifying source code formatting"
)
