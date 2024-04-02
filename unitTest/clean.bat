@echo off
cls

set folder=.\build\CMakeFiles
set html=.\build\*.html
set css=.\build\*.css

if exist %folder% (
    rmdir /S /Q %folder%
    echo cleaned %folder%
) else (
    echo nothing to clean
)

del /F /Q %html%
del /F /Q %css%

@echo on
