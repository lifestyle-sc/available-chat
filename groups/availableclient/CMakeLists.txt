# === Exclude main and test files using regex
file(GLOB SRC_FILES CONFIGURE_DEPENDS "*.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX ".*\\.m\\.cpp$")
list(FILTER SRC_FILES EXCLUDE REGEX ".*\\.t\\.cpp$")

# === Build Source files as libraries
add_library(availableclientlib STATIC ${SRC_FILES})
target_link_libraries(availableclientlib PRIVATE availablecommonlib)
target_include_directories(availableclientlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# === GoogleTest via FetchContent ===
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# === Auto-discover test files matching *.t.cpp in root ===
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.t.cpp")

include(GoogleTest)

foreach(TEST_SRC ${TEST_SOURCES})
    get_filename_component(FILE_NAME ${TEST_SRC} NAME)
    string(REGEX REPLACE "\\.cpp$" "" TEST_NAME ${FILE_NAME}) # e.g., math.t.cpp -> math.t
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} PRIVATE availableclientlib gtest_main)
    gtest_discover_tests(${TEST_NAME})
endforeach()