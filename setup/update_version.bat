call ..\tools\ExtractVersion.bat %1 %2 %3 %4
if errorlevel 1 goto :err

echo AppVerName=StrongDC++ sqlite r%VERSION_NUM%%BETA_STATE%%REVISION_NUM_UI% > version.txt

goto :end

:err
pause

:end