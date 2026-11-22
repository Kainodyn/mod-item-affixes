@echo off
setlocal
rem mod-item-affixes client patcher. Close World of Warcraft first.
rem Run after the mod-classless-wildcard installer, and again after every run of it.
cd /d "%~dp0"
where py >nul 2>nul && (py -3 "%~dp0ItemAffixesPatch.py" %* & goto :done)
where python >nul 2>nul && (python "%~dp0ItemAffixesPatch.py" %* & goto :done)
echo Python 3 was not found. Install it from https://www.python.org/downloads/ and tick "Add python.exe to PATH".
:done
echo.
pause
