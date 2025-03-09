#!/bin/bash

current_directory=$(pwd)
build_directory="$current_directory/build-coverage"

if [ -d "$build_directory" ]; then
    rm -rf "$build_directory"
    echo "Deleted existing coverage directory."
fi

cmake -B "$build_directory" -DCMAKE_CXX_COMPILER=clang++ -DIS_COMPILING_STATIC=true -DIS_COMPILING_SHARED=false -DENABLE_TESTING=true -DENABLE_COVERAGE=true

cmake --build "$build_directory" --config Release

ctest --output-on-failure --test-dir "$build_directory"

lcov --capture --directory "$build_directory" --output-file "$build_directory/coverage.info" --keep-going

lcov --remove "$build_directory/coverage.info" "/Applications/*" "v1/*" "*gtest*" "googlemock/*" "$current_directory/third-party/*" "$build_directory/_deps/*" "$current_directory/sokketter-cli/tests/*" --output-file "$build_directory/coverage.filtered.info" --keep-going

genhtml "$build_directory/coverage.filtered.info" --output-directory "$build_directory/coverage_report" --keep-going

FILE="$build_directory/coverage_report/index.html"
[ -f "$FILE" ] && open "$FILE"
