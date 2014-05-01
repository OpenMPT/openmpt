REM del /s html
REM mkdir html
REM wiki.py
copy source\*.* html\
htmlhelp\hhc.exe "html\OpenMPT Manual.hhp"
copy "html\OpenMPT Manual.chm" "..\..\packageTemplate\"
@pause