call clean_all.bat
cd compiled\settings
call get_customlocations.bat
cd ..
cd ..
call build_strongdc_sqlite.bat
call build_strongdc_sqlite_64.bat
copy *.7z "d:\ppa-doc\Dropbox\FlylinkDC++\beta-strongdc-sqlite"
copy *.7z "f:\ftp\DC++\strongdc-build\bin-src"
copy *.7z "C:\Users\ppa\Ubuntu One\build\bin-src\strongdc-sqlite"
copy changelog-sqlite-svn.txt "C:\Users\ppa\Ubuntu One\build\bin-src\strongdc-sqlite"
copy changelog-sqlite-svn.txt "f:\ftp\DC++\strongdc-build"