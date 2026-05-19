@echo off
cd /d %~dp0
SET /P LANGUAGE_DIR=< .\etc\language.ini
type %LANGUAGE_DIR%\help.lang
echo.
set PATH=%PATH%;%~dp0scuts
cmd