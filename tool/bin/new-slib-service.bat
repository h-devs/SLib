@echo off

set CURRENT_PATH=%cd%
if defined SLIB_PATH goto setup_path_end
set BAT_PATH=%~dp0
set BAT_PATH=%BAT_PATH:~0,-1%
call %BAT_PATH%/../../setup-path.bat quick
cd %CURRENT_PATH%

:setup_path_end
set APP_NAME=%1
if not "%APP_NAME%"=="" goto input_app_name_end
set /p APP_NAME=Please input the service name:
if "%APP_NAME%"=="" goto :eof

:input_app_name_end
set APP_NAME=%APP_NAME:-=%
if "%APP_NAME%"=="" goto :eof
set APP_NAME=%APP_NAME: =%
if "%APP_NAME%"=="" goto :eof

echo New SLib Service: %APP_NAME%

xcopy /h /e "%SLIB_PATH%\tool\template\service" "%CURRENT_PATH%\"
ReplaceTextInFile.exe "%CURRENT_PATH%\main.cpp" "%CURRENT_PATH%\main.cpp" SLIB_TEMPLATE_APP_NAME %APP_NAME%
ReplaceTextInFile.exe "%CURRENT_PATH%\app.h" "%CURRENT_PATH%\app.h" SLIB_TEMPLATE_APP_NAME %APP_NAME%
ReplaceTextInFile.exe "%CURRENT_PATH%\app.cpp" "%CURRENT_PATH%\app.cpp" SLIB_TEMPLATE_APP_NAME %APP_NAME%

ReplaceTextInFile.exe "%CURRENT_PATH%\Win32\SLIB_TEMPLATE_APP_NAME.sln" "%CURRENT_PATH%\Win32\SLIB_TEMPLATE_APP_NAME.sln" SLIB_TEMPLATE_APP_NAME %APP_NAME%
rename "%CURRENT_PATH%\Win32\SLIB_TEMPLATE_APP_NAME.sln" "%APP_NAME%.sln"
ReplaceTextInFile.exe "%CURRENT_PATH%\Win32\SLIB_TEMPLATE_APP_NAME.vcxproj" "%CURRENT_PATH%\Win32\SLIB_TEMPLATE_APP_NAME.vcxproj" SLIB_TEMPLATE_APP_NAME %APP_NAME%
rename "%CURRENT_PATH%\Win32\SLIB_TEMPLATE_APP_NAME.vcxproj" "%APP_NAME%.vcxproj"
rename "%CURRENT_PATH%\Win32\SLIB_TEMPLATE_APP_NAME.vcxproj.filters" "%APP_NAME%.vcxproj.filters"

ReplaceTextInFile.exe "%CURRENT_PATH%\Linux\CMakeLists.txt" "%CURRENT_PATH%\Linux\CMakeLists.txt" SLIB_TEMPLATE_APP_NAME %APP_NAME%
