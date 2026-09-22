@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

set CC=cl
where cl >nul 2>nul || set CC=clang-cl
where %CC% >nul 2>nul || (echo need cl or clang-cl on PATH && exit /b 1)
where python >nul 2>nul || (echo need python on PATH && exit /b 1)

set CFLAGS=/nologo /O2 /MD /EHsc /W3 /DNDEBUG /Ithird_party\minhook\include
set LDFLAGS=/link /DLL user32.lib kernel32.lib
set SRC=src\dllmain.cpp src\scanner.cpp src\types.cpp third_party\minhook\src\buffer.c third_party\minhook\src\hook.c third_party\minhook\src\trampoline.c third_party\minhook\src\hde\hde64.c

rem build.bat [profile]  (default: every profiles\*.ini) -> dist\<profile>\ktfl.asi
set LIST=%~1
if "%LIST%"=="" for %%f in (profiles\*.ini) do set LIST=!LIST! %%~nf

for %%p in (%LIST%) do (
    if not exist obj\%%p mkdir obj\%%p
    if not exist dist\%%p mkdir dist\%%p
    python tools\gen_profile.py profiles\%%p.ini obj\%%p\profile_gen.h || exit /b 1
    %CC% %CFLAGS% /Iobj\%%p /Fo:obj\%%p\ %SRC% /Fe:dist\%%p\ktfl.asi %LDFLAGS% || exit /b 1
    echo built dist\%%p\ktfl.asi
)
