cd ..
cd ..

cmd /k build\auto\build_libopenmpt.cmd vs2010 Win32
cmd /k build\auto\build_libopenmpt.cmd vs2010 x64
cmd /k build\auto\package_libopenmpt_win.cmd

cmd /k call build\auto\build_libopenmpt.cmd vs2008 Win32
cmd /k call build\auto\package_libopenmpt_winold.cmd

pause
