# === Options
option(ENABLE_PROFILING "Link with gperftools profiler" OFF)
set(SANITIZERS "" CACHE STRING "Comma-separated list: address,undefined,leak,thread")

# This makes release builds debuggable (but still optimized)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")

# Set flags for Debug (low optimization, high debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wpedantic")

# Set flags for Release (high optimization)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall -Wpedantic")

# === Helper: parse SANITIZERS into flags
function(apply_sanitizers tgt)
  if(SANITIZERS)
    string(REPLACE "," ";" _list "${SANITIZERS}")
    set(_flags "")
    foreach(s IN LISTS _list)
      if(s STREQUAL "address")
        list(APPEND _flags address)
      elseif(s STREQUAL "undefined")
        list(APPEND _flags undefined)
      elseif(s STREQUAL "leak")
        list(APPEND _flags leak)
      elseif(s STREQUAL "thread")
        list(APPEND _flags thread)
      else()
        message(FATAL_ERROR "Unknown santizer: ${s}")
      endif()
    endforeach()
    if(_flags)
      # Join list back to comma-separated string for the compiler flag
      string(JOIN "," _flags_str ${_flags})
      # add compile+link sanitizer flags
      target_compile_options(${tgt} PRIVATE -fsanitize=${_flags_str})
      target_link_options(${tgt} PRIVATE -fsanitize=${_flags_str})
      # helpful for nicer stacks
      target_compile_options(${tgt} PRIVATE -fno-omit-frame-pointer)
    endif()

  endif()
endfunction()

# === Helper: profiler link
function(apply_profiler tgt)
  if(ENABLE_PROFILING)
    # Prefer find_library; fall back to raw flag if not found
    find_library(PROFILER_LIB profiler)
    if(PROFILER_LIB)
      target_link_libraries(${tgt} PRIVATE ${PROFILER_LIB})
    else()
      target_link_options(${tgt} PRIVATE -lprofiler)
    endif()
  endif()
endfunction()

# Create a "warnings" target that doesn't build code itself,
# but holds all our strict settings.
add_library(project_strict_warnings INTERFACE)

target_compile_options(project_strict_warnings INTERFACE
    # Settings for GCC and Clang
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall                   # Basic common warnings
        -Wextra                  # Additional common warnings
        -Wshadow                # Warn when a variable "shadows" another (see below)
        -Wuninitialized         # Warn about using variables before they are set
        -Wpedantic              # Ensure you aren't using non-standard compiler "extensions"
        -Wconversion            # Warn if data is lost during type conversion (e.g., double to int)
        -Wreturn-type           # The one that caught your previous error!
        -Werror                 # Turn all the above into errors
    >
    # Settings for Visual Studio (MSVC)
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4                     # Set warning level 4 (highest standard level)
        /WX                     # Warnings as errors
        /permissive-            # Enforce standard C++ behavior (turns off MSVC "cheats")
    >
)

# === Main App ===
add_executable(example.tsk
example.m.cpp
)
target_link_libraries(example.tsk PRIVATE exlib project_strict_warnings)

# ===  Apply knobs in one place
apply_sanitizers(exlib)
apply_sanitizers(example.tsk)
apply_profiler(example.tsk)

