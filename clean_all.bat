del .\compiled\*.ilk
del .\compiled\StrongDC_debug.*
del /S /F *.orig
del /S /F *.obj
del /S /F *.pdb
del /S /F /Q vc10
del *.tmp
"%VS100COMNTOOLS%..\ide\devenv" StrongDC.sln /Clean "Release|Win32"
"%VS100COMNTOOLS%..\ide\devenv" StrongDC.sln /Clean "Release|x64"
"%VS100COMNTOOLS%..\ide\devenv" StrongDC.sln /Clean "Debug|Win32"
"%VS100COMNTOOLS%..\ide\devenv" StrongDC.sln /Clean "Debug|x64"

