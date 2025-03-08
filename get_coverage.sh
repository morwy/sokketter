#!/bin/bash

current_dir=$(pwd)

for path in "$current_dir"/build/*/; do
    if [ -f "$current_dir/coverage.info" ]; then
        rm "$current_dir/coverage.info"
        echo "Deleted existing coverage.info file."
    fi

    if [ -f "$current_dir/coverage.filtered.info" ]; then
        rm "$current_dir/coverage.filtered.info"
        echo "Deleted existing coverage.filtered.info file."
    fi

    if [ -d "$current_dir/coverage_report" ]; then
        rm -rf "$current_dir/coverage_report"
        echo "Deleted existing coverage_report directory."
    fi

    ctest --output-on-failure --test-dir "$path"

    lcov --capture --directory "$path" --output-file "$current_dir/coverage.info" --keep-going

    lcov --remove coverage.info "/Applications/*" "v1/*" "*gtest*" "googlemock/*" "$current_dir/third-party/*" "$path/_deps/*" "$current_dir/sokketter-cli/tests/*" --output-file "$current_dir/coverage.filtered.info" --keep-going

    genhtml "$current_dir/coverage.filtered.info" --output-directory "$current_dir/coverage_report" --keep-going

    break
done
