REM del /s html
REM mkdir html
REM wiki.py
copy source\*.* html\
"%ProgramFiles%\7-zip\7z" x html.tgz
"%ProgramFiles%\7-zip\7z" x -y html.tar
del html.tar
htmlhelp\hhc.exe "html\OpenMPT Manual.hhp"
copy "html\OpenMPT Manual.chm" "..\..\packageTemplate\"
@pause