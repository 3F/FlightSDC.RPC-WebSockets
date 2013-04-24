call update_version.bat %1 %2 %3 %4

:Loop
IF "%1"=="" GOTO Continue
if "%1"=="norev" goto :ValidParam
if "%1"=="nover" goto :ValidParam
if "%1"=="nobeta" goto :ValidParam
rem Found parameter that is no "norev" nor "nover" nor "nobeta". It must be filename mask
goto :Continue
:ValidParam
SHIFT
GOTO Loop
:Continue

if "%1"=="" goto :ProcessAllScripts
for %%w in (%1) do (
"C:\Program Files (x86)\Inno Setup 5\ISCC.exe" /Q "%%w"
ren "output\Setup%%~nw.exe" "Setup%%~nw-r%VERSION_NUM%%BETA_STATE%%REVISION_NUM%.exe"
)
goto :end

:ProcessAllScripts
for %%w in (*.iss) do (
"C:\Program Files (x86)\Inno Setup 5\ISCC.exe" /Q "%%w"
ren "output\Setup%%~nw.exe" "Setup%%~nw-r%VERSION_NUM%%BETA_STATE%%REVISION_NUM%.exe"
)

goto :end

:err
echo Error get version!!!
pause

:end