
for %%w in (strongdc-Install-*.iss) do (
"C:\Program Files (x86)\Inno Setup 5\ISCC.exe" /Q /O"output" /F"%%~nw" "%%w"
)

call clear_installers.bat
cd Output
call install-copy-www.bat
