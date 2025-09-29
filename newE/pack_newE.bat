@echo off

del /s *.pdb *.lib *.exp *.obj *.ilk *.log *.tlog *.idb *.pch *.sdf *.ipdb *.iobj *.vcxproj* *.makefile *.ninja *.tmp *.cache *.rsp *.bat *.sh
del /s /q *.obj *.pch *.ilk *.log *.tmp *.ninja_* *.stamp *.tlog *.lastbuildstate *.pdb
rd /s /q obj
rd /s /q gen
del /q .ninja_deps .ninja_log build.ninja
for /f "delims=" %d in ('dir /ad/b/s ^| sort /R') do rd "%d"
del /s /q *test*
rd /s /q test_fonts
rd /s /q win_clang_x64_for_rust_host_build_tools

pause

















