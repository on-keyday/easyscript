@echo off
rem     Copyright (c) 2021 on-keyday
rem     Released under the MIT license
rem     https://opensource.org/licenses/mit-license.php
cd clang_build
call reconfig.bat
cd ..\gcc_build
call reconfig.bat
cd ..\cl_build
call reconfig.bat
cd ..