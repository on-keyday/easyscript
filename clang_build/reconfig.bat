@echo off
rem     Copyright (c) 2021 on-keyday
rem     Released under the MIT license
rem     https://opensource.org/licenses/mit-license.php
setlocal
call ..\common\reconfig_common
cmake -G %CMAKE_GENERATOR% -D CMAKE_BUILD_TYPE=%BUILD_MODE% -D CMAKE_C_COMPILER=clang -D CMAKE_CXX_COMPILER=clang++ -B ./built .