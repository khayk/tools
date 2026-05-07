build_type="Release"
build_dir="builds/$build_type"

mkdir -p "$build_dir"

cmake -B "$build_dir" -S . -DCMAKE_BUILD_TYPE=$build_type
if [ $? -ne 0 ]; then
    echo "CMake configuration failed."
    exit 1
fi

cmake --build "$build_dir" --parallel 12 --config $build_type
if [ $? -ne 0 ]; then
    echo "Build failed."
    exit 1
fi

echo "Build completed successfully. Output is in the '$build_dir' directory."

ctest --test-dir "$build_dir" --build-config $build_type --output-on-failure
if [ $? -ne 0 ]; then
    echo "Tests failed."
    exit 1
fi
echo "All tests passed successfully."
