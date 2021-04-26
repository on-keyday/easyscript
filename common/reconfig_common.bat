rem     Copyright (c) 2021 on-keyday
rem     Released under the MIT license
rem     https://opensource.org/licenses/mit-license.php
call ..\setting\common_setting.bat
call ..\common\clean_built.bat
if "%PROJECT_COPY%"=="true" call ..\common\copy_src.bat
