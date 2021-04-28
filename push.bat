@echo off
rem     Copyright (c) 2021 on-keyday
rem     Released under the MIT license
rem     https://opensource.org/licenses/mit-license.php
setlocal
call setting\common_setting.bat
if not "%PUBLISH_DIR%"=="" (
    python common/license/add_to_script.py %PUBLISH_DIR% %LICENSE_FILE%
    cd %PUBLISH_DIR%
    cd
)
set COMMIT_NAME=
echo input commit name
set /P COMMIT_NAME=
set TODAY=%date:~0,4%%date:~5,2%%date:~8,2%
set time2=%time: =0%
set NOWTIME=%time2:~0,2%%time2:~3,2%%time2:~6,2%%
set FINAL="%TODAY%%NOWTIME%-%COMMIT_NAME%"
echo %FINAL%
git add %PUSH_ADD_OPT%
git commit -m %FINAL%
git push origin main
