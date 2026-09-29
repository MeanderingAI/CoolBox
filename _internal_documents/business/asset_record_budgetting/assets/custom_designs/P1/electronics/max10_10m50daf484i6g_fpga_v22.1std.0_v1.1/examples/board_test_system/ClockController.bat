@echo off

echo ****************************************
echo *           CLOCK CONTROLLER           *
echo ****************************************
echo=

if not defined QUARTUS_ROOTDIR goto error_env
if not exist %QUARTUS_ROOTDIR%/bin64/jre64/bin/ goto error_dir

if exist "bts_log.txt" ATTRIB -R bts_log.txt

goto start

:error_env
start cmd /c "echo ------------------------------- && echo QUARTUS_ROOTDIR does not exist!&& echo ------------------------------- && echo The environment variable QUARTUS_ROOTDIR cannot be found. && echo Please add QUARTUS_ROOTDIR in System Properties. && echo QUARTUS_ROOTDIR should be ^<Quartus Install Path^>^\quartus. && pause"
exit

:error_dir
start cmd /c "echo --------------------------- && echo JRE library does not exist!&& echo --------------------------- && echo %QUARTUS_ROOTDIR%\bin64\jre64\bin cannot be found. && echo Please check the directory. && pause"
exit

:start
echo Board Test System starting...
@echo on
%QUARTUS_ROOTDIR%/bin64/jre64/bin/java.exe -jar bts.jar -mode clk
exit