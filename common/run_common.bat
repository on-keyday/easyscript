@echo off
rem     Copyright (c) 2021 on-keyday
rem     Released under the MIT license
rem     https://opensource.org/licenses/mit-license.php
call ..\setting\common_setting.bat
if exist built\built\%RUNEXECUTABLE%.exe (
  cd built/built
  %RUNEXECUTABLE% %RUNCMDLINE%
  cd ../..
)
