del compiled\StrongDC64*.exe
call UpdateRevision.bat
if errorlevel 1 goto :error

rem Extract revision
for /f "tokens=2,3 delims= " %%a in ('type revision.h') do (
if %%a == REVISION set FLYLINK_REVISION=%%b
)

call "%VS100COMNTOOLS%\..\..\VC\bin\amd64\vcvars64.bat"
"%VS100COMNTOOLS%..\ide\devenv" StrongDC.sln /Rebuild "Release|x64"

call src_gen_filename.bat -x64
7z.exe a -r -t7z -m0=lzma -mx=9 -mfb=512 -md=1024m -ms=on -x@src_exclude_hard.txt -x!*.svn-base -i@dll_include.txt  %FILE_NAME%.7z compiled/Settings/GeoIPCountryWhois.csv compiled/Settings/Custom*.* changelog-sqlite-svn.txt cvs-changelog.txt compiled\StrongDC64.exe compiled\readme.txt compiled\Settings\IPTrust.ini compiled\crshhndl-x64.dll
call src_gen_filename.bat -debug-info
7z.exe a -r -t7z -m0=lzma -mx=9 -mfb=512 -md=1024m -ms=on %FILE_NAME%.7z *.pdb -x!vc100.pdb -x!vc10


:error