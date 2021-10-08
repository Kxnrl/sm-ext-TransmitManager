set EXT_DIR="%cd%"
set VCVARSALL="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"

if "%VSCMD_VER%"=="" (
	set MAKE=
	set CC=
	set CXX=
	call %VCVARSALL% x86
)

git clone https://github.com/alliedmodders/metamod-source --branch "%MMBRANCH%" --single-branch "%EXT_DIR%/mmsource-%MMBRANCH%"
git clone https://github.com/alliedmodders/hl2sdk --branch csgo --single-branch "%EXT_DIR%/hl2sdk-csgo"
git clone https://github.com/alliedmodders/sourcemod --recursive --branch "%SMBRANCH%" --single-branch "%EXT_DIR%/sourcemod-%SMBRANCH%"

mkdir "%EXT_DIR%/build"
pushd "%EXT_DIR%/build"
python "%EXT_DIR%/configure.py" --enable-optimize --mms-path "%EXT_DIR%/mmsource-%MMBRANCH%" --sm-path "%EXT_DIR%/sourcemod-%SMBRANCH%" --hl2sdk-root "%EXT_DIR%" -s csgo || goto error
ambuild || goto error
popd

:error
exit /b %errorlevel%