#!/bin/bash
test_case=$1
golden_file=${test_case}/main.expected.rs
out_dir=/tmp/hayroll-output/${test_case}

set -x
../build/hayroll -vv transpile ${test_case}/compile_commands.json --output-dir ${out_dir} || exit 1

[ ! -e ${golden_file} ] && cp ${out_dir}/src/main.rs ${golden_file}
exec diff ${out_dir}/src/main.rs ${golden_file}
