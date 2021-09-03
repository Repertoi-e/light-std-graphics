@echo off
call .\vendor\Windows\cloc\cloc-1.64.exe lstd test_suite -force-lang="C++",cppm %*

