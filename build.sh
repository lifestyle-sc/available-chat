#!/bin/bash
set -e

# === Default settings ===
BUILD_TYPE="Debug"
ACTIONS=("build")
CMAKE_ARGS=()

# === Parse args ====
while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --action=*)
            IFS=',' read -r -a ACTIONS <<< "${1#--action=}"
            shift
            ;;
        -D*)
            # Validate that -D argument has proper format: -DVAR=value or -DVAR:type=value
            if [[ ! "$1" =~ ^-D[A-Za-z_][A-Za-z0-9_]*([=:]|$) ]]; then
                echo "❌ Invalid CMake variable format: $1"
                echo "   Expected: -DVAR=value or -DVAR:type=value"
                echo "   Example: -DSANITIZERS=address,undefined,leak"
                exit 1
            fi
            CMAKE_ARGS+=("$1")
            shift
            ;;
        *)
            echo "❌ Unknown option: $1"
            echo "Usage: $0 [--debug|--release] [--action=clean,build,ctest] [-DVAR=value ...]"
            echo ""
            echo "Examples:"
            echo "  $0 --debug --action=build,ctest"
            echo "  $0 --release --action=build -DSANITIZERS=address,undefined,leak"
            echo "  $0 --debug --action=build -DENABLE_PROFILING=ON"
            exit 1
            ;;
    esac
done

# === Derived paths ===
BUILD_DIR="build/$(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')"

# === Handle actions ===
for ACTION in "${ACTIONS[@]}"; do
    case "$ACTION" in
        build)
            echo "🛠️ Building in $BUILD_TYPE mode..."
            mkdir -p "$BUILD_DIR"
            cd "$BUILD_DIR"
            cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" "${CMAKE_ARGS[@]}" -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE ../..
            cmake --build .
            cd - > /dev/null
            echo "✅ Build completed"
            ;;
        clean)
            echo "🧹 Cleaning build directory: $BUILD_DIR"
            rm -rf "$BUILD_DIR"
            echo "✅ Cleaned"
            ;;
        ctest)
            if [[ ! -d "$BUILD_DIR" ]]; then
                echo "⚠️ Build directory not found. Please build first"
                exit 1
            fi
            echo "🧪 Running tests with ctest in $BUILD_DIR..."
            cd "$BUILD_DIR"
            ctest --output-on-failure
            cd - > /dev/null
            echo "✅ Tests run completed"
            ;;
        *)
            echo "❌ Unknown action: $ACTION"
            echo "Allowed actions: build, clean, ctest"
            exit 1
            ;;
    esac
done