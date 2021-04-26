@echo off
rem     Copyright (c) 2021 on-keyday
rem     Released under the MIT license
rem     https://opensource.org/licenses/mit-license.php
setlocal
call ..\setting\common_setting.bat
xcopy %PROJECT_BASE%  pre_src /Y /E /Q /I
rmdir /S /Q src
rename pre_src src
