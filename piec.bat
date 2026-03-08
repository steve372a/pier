::[Bat To Exe Converter]
::
::YAwzoRdxOk+EWAjk
::fBw5plQjdCqDJH2B50kkJwtoXB2KOXO1OrkT7+epoeOErS0=
::YAwzuBVtJxjWCl3EqQJgSA==
::ZR4luwNxJguZRRnk
::Yhs/ulQjdF+5
::cxAkpRVqdFKZSDk=
::cBs/ulQjdF+5
::ZR41oxFsdFKZSDk=
::eBoioBt6dFKZSDk=
::cRo6pxp7LAbNWATEpSI=
::egkzugNsPRvcWATEpSI=
::dAsiuh18IRvcCxnZtBJQ
::cRYluBh/LU+EWAnk
::YxY4rhs+aU+JeA==
::cxY6rQJ7JhzQF1fEqQJQ
::ZQ05rAF9IBncCkqN+0xwdVs0
::ZQ05rAF9IAHYFVzEqQJQ
::eg0/rx1wNQPfEVWB+kM9LVsJDGQ=
::fBEirQZwNQPfEVWB+kM9LVsJDGQ=
::cRolqwZ3JBvQF1fEqQJQ
::dhA7uBVwLU+EWDk=
::YQ03rBFzNR3SWATElA==
::dhAmsQZ3MwfNWATElA==
::ZQ0/vhVqMQ3MEVWAtB9wSA==
::Zg8zqx1/OA3MEVWAtB9wSA==
::dhA7pRFwIByZRRnk
::Zh4grVQjdCqDJFuF90klOCdERQGQcTn0VvtMper+++vWnl0UUfBxfZfeug==
::YB416Ek+ZG8=
::
::
::978f952a14a936cc963da21a135fa983
@echo off
@rem =======================================================
@rem  Project: Pier (Windows Package Installer)
@rem  Author: Sanakaprix <steve372@foxmail.com>
@rem  TikTok(China): Sanakaprix
@rem  Bilibili: https://space.bilibili.com/430970352
@rem =======================================================
title Package Installer by Sanakaprix
:: ???
setlocal enabledelayedexpansion

:: =======================================================
if "%PIER_ROOT%"=="" SET "PIER_ROOT=%~dp0"
IF "%PIER_ROOT:~-1%"=="\" SET "PIER_ROOT=%PIER_ROOT:~0,-1%"

:: Get short path name (8.3 format) for XP compatibility with spaces in path
for %%I in ("%PIER_ROOT%") do set "PIER_ROOT_SHORT=%%~sI"

SET "PATH=%PIER_ROOT%\bin;%PATH%"

:: ????????????????
for /f "delims=" %%a in ('%PIER_ROOT%\bin\pier-arch.exe sysarch') do set "SYS_ARCH=%%a"

:: =======================================================

SET /P LANGUAGE_DIR=< %PIER_ROOT%\etc\language.ini
:: Replace %PIER_ROOT% variable in LANGUAGE_DIR
set "LANGUAGE_DIR=!LANGUAGE_DIR:%%PIER_ROOT%%=%PIER_ROOT%!"
:: ?????????????????????????????????????????
if not exist "%LANGUAGE_DIR%\\lang.ini" set "LANGUAGE_DIR=%PIER_ROOT%\share\language\zh-CN"
SET version=2.3.0

:: ??????????????????????????????????????
if exist "%Temp%\pier_choice.tmp" (
    echo wscript.sleep 200 >%Temp%\Wait.vbs
    start /wait %Temp%\Wait.vbs
    del /f /q "%Temp%\pier_choice.tmp" 2>nul
)
if exist "%Temp%\pier_env.tmp" (
    echo wscript.sleep 200 >%Temp%\Wait.vbs
    start /wait %Temp%\Wait.vbs
    del /f /q "%Temp%\pier_env.tmp" 2>nul
)

:: ??????????????????????????????
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[welcome\]/{n;p}" "%LANGUAGE_DIR%\lang.ini"') do @set "welcome=%%a"
.\bin\vecho.exe %welcome% $brightcyan$%version%

:: Version check
set "PIER_LANG_DIR=%LANGUAGE_DIR%"
for /f "tokens=1" %%v in ("%version%") do %PIER_ROOT%\bin\pier-ver.exe %%v

:: 设置源路�?
SET sourceimage=/sources
SET onlinelist=/list/listonline.zip

:: ?????????????????
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[error_invalid_cmd\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "error_invalid_cmd=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[error_no_param\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "error_no_param=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[error_no_package\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "error_no_package=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[error_no_package_remove\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "error_no_package_remove=%%a"

:: ?????????????????
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[error_protected_lang\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "error_protected_lang=%%a"

:: ????????????????
:: ?? sourceimage.ini ?? [package_source] ? [alias_source]
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[package_source\]/{n;p}" %PIER_ROOT%\etc\sourceimage.ini') do @set "package_source=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[alias_source\]/{n;p}" %PIER_ROOT%\etc\sourceimage.ini') do @set "alias_source=%%a"
:: ??????? source ? package_source ???
SET source=%package_source%
:: ???
SET full_list_url=%source%%onlinelist%
SET pies=/pies
SET full_source_url=%source%%sourceimage%
SET full_pies_url=%source%%pies%
:: alias_source ??? pier-op ??
SET full_alias_source=%alias_source%

:: ??????????????
SET parameters=%1
SET package=%2
SET custom=%3
SET autoyes=%4
:: autoyes=?????????
:: ?????? -y ????????
if /I "%2%"=="-y" (
    set "autoyes=-y"
    set "package=%3"
    set "custom=%4"
)
if /I "%3%"=="-y" (
    set "autoyes=-y"
    set "custom=%4"
)
If "%parameters%"=="" goto error1
If "%parameters%"=="install" goto installpackages
If "%parameters%"=="remove" goto removepackages
If "%parameters%"=="--help" goto help
If "%parameters%"=="-help" goto help
If "%parameters%"=="help" goto help
If "%parameters%"=="?" goto help
If "%parameters%"=="-?" goto help
If "%parameters%"=="--?" goto help
If "%parameters%"=="license" start notepad %PIER_ROOT%\license && goto quit
If "%parameters%"=="list" goto listpackage
If "%parameters%"=="--setlang" goto language
If "%parameters%"=="-setlang" goto language
If "%parameters%"=="setlang" goto language
If "%parameters%"=="sl" goto language
If "%parameters%"=="-h" goto help
If "%parameters%"=="sources" goto repo
If "%parameters%"=="search" goto search
If "%parameters%"=="o" goto openpackage
echo %error_invalid_cmd%
goto quit

:listpackage
if "%package%"=="" goto onlinepulllist
echo %error_invalid_cmd%
goto quit

:onlinepulllist
:: ???? list ??????????
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[db_download_msg\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "db_download_msg=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[db_download_tip\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "db_download_tip=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[db_download_error\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "db_download_error=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[db_list_filename\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "db_list_filename=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[db_list_intro\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "db_list_intro=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[db_list_count_msg\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "db_list_count_msg=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[db_list_suffix\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "db_list_suffix=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[list_offline_disabled\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "list_offline_disabled=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[pull_list_failed\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "pull_list_failed=%%a"
:: ??? 280ms
if exist %Temp%\db.sque Del /f /s /q %Temp%\db.sque > nul
if exist %Temp%\dbm.sque Del /f /s /q %Temp%\dbm.sque > nul
echo wscript.sleep 320 >%Temp%\Wait.vbs
start /wait %Temp%\Wait.vbs  
echo %db_download_msg%%db_list_filename%
:: ???? db.sque??????????
%PIER_ROOT%\bin\uma-get.exe -q -P "%Temp%" --no-check-certificate "%source%/db.sque"
If not exist %Temp%\db.sque goto error_dbsque
:: ???? dbm.sque??????????????
%PIER_ROOT%\bin\uma-get.exe -q -P "%Temp%" --no-check-certificate "%source%/dbm.sque"
If not exist %Temp%\db.sque goto error_dbsque
:: ??? db.sque ?? dbm.sque??db.sque ????????????dbm.sque ??????????????
SET /P dbmsq=< %Temp%\dbm.sque
if exist %Temp%\dbm.sque Del /f /s /q %Temp%\dbm.sque > nul
:: ????????????????????????????? X ????????dbmsq ?????????
echo %db_list_intro%%db_list_count_msg% %dbmsq% %db_list_suffix%
echo.
type %Temp%\db.sque
if exist %Temp%\db.sque Del /f /s /q %Temp%\db.sque > nul
goto quit

:offlinepulllist
echo %list_offline_disabled%
goto quit

:language
:: ???? language ??????????
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[language_installed\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "language_installed=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[language_installed_success\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "language_installed_success=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[language_set_success\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "language_set_success=%%a"

if "%package%"=="" goto error2
if "%package%"=="set" goto langset
if "%package%"=="install" goto langinstall
if "%package%"=="reins" goto langdelete
goto error2

:langset
echo %%PIER_ROOT%%\share\language\%custom%> %PIER_ROOT%\etc\language.ini
SET /P LANGUAGE_DIR=< %PIER_ROOT%\etc\language.ini
:: Replace %PIER_ROOT% variable in LANGUAGE_DIR
set "LANGUAGE_DIR=!LANGUAGE_DIR:%%PIER_ROOT%%=%PIER_ROOT%!"
echo %language_set_success%
goto quit

:langdelete
if /I "%custom%"=="zh-CN" goto error5
if /I "%custom%"=="en-US" goto error5
echo %language_installed%
rd /s /q %PIER_ROOT%\share\language\%custom% > nul
:: ??????????????
goto removepackages

:: ???????? - ?????????????
:langinstall
:: ???????????????????????????????????????????
goto installpackages

:search
:: ???? search ??????????
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[db_download_tip\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "db_download_tip=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[db_download_error\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "db_download_error=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[search_results_title\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "search_results_title=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[search_not_found\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "search_not_found=%%a"

:: ?????????????
:: ??? 280ms
if exist %Temp%\db.sque Del /f /s /q %Temp%\db.sque > nul
echo wscript.sleep 320 >%Temp%\Wait.vbs
start /wait %Temp%\Wait.vbs  
echo %db_download_tip%
:: ???? db.sque
%PIER_ROOT%\bin\uma-get.exe -q -P "%Temp%" --no-check-certificate "%source%/db.sque"
If not exist %Temp%\db.sque goto error_dbsque
:: ??? db.sque ??????????????
set counter='0'
set "dbfile=%TEMP%\db.sque"
echo ???????? "%package%"...
findstr /i /r /c:"^.*%package%.*$" "%dbfile%" > results.tmp
:: ??????????
if %errorlevel% equ 0 (
    echo %search_results_title%
    type results.tmp
    del results.tmp
) else (
    echo %search_not_found%
    if exist results.tmp del results.tmp
)
goto quit


:help
:: ??????????????????????
type %LANGUAGE_DIR%\help.lang
echo.
echo - GNU sed (GPLv3): https://www.gnu.org/software/sed/
echo - GNU wget (GPLv3): https://www.gnu.org/software/wget/
echo - Info-ZIP (BSD-style): http://www.info-zip.org/
goto quit

:: ????????????%%???????????????????pier

:error1
:: ????????
echo %error_no_param%
goto quit

:: ??????
:error_inprepo
echo %error_invalid_cmd%
goto quit

:error2
echo %error_no_package%
goto quit

:error2_remove
echo %error_no_package_remove%
goto quit

:error3
echo %error_package_not_exist%
goto quit

:error_dbsque
echo %db_download_error%
goto quit

:pullfailed
:: ??????
echo %pull_list_failed%
goto quit

:error4
:: ?????????
echo %error_install_failed%
goto quit

:error5
:: ???????????????
echo %error_protected_lang%
goto quit

:ok1
:: "????????????????"
echo %package_not_installed%
goto quit

:repo
:: ???? sources ??????????
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[checking_source\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "checking_source=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[source_name_label\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "source_name_label=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[source_owner_label\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "source_owner_label=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[source_admin_label\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "source_admin_label=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[repo_change_confirm\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "repo_change_confirm=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[repo_change_confirm_2\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "repo_change_confirm_2=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[repo_change_confirm_3\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "repo_change_confirm_3=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[repo_changed\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "repo_changed=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[source_invalid\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "source_invalid=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[source_select_prompt\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "source_select_prompt=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[source_select_options\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "source_select_options=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[source_url_label\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "source_url_label=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[lang_onlinelist\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "lang_onlinelist=%%a"

::???
if "%package%"=="" goto repo_chg
if "%package%"=="list" goto repo_check
if "%package%"=="change" goto repo_chg
if "%package%"=="chg" goto repo_chg
echo %error_invalid_cmd%
goto quit

:repo_chg
if /I "%custom%"=="" goto repo_chg2
:: ???
echo %checking_source%
if exist %Temp%\db.sque Del /f /s /q %Temp%\db.sque > nul
:: ???%custom%??????????/???????
if "!custom:~-1!" == "/" ( 
    set "custom=!custom:~0,-1!"
)
:: ???%custom%????????????????
if "!custom:~-1!" == " " (
    set "custom=!custom:~0,-1!"
)
:: ?????????????????????
%PIER_ROOT%\bin\uma-get.exe -q -P "%Temp%" --no-check-certificate "%custom%/info.sque"
If not exist %Temp%\info.sque goto error_repo
:: ???? info ????????????????????????[???]\n?\n??????
for /f "delims=" %%a in ('type "%Temp%\info.sque" ^| %PIER_ROOT%\bin\sed.exe -n "/\[sourcename_cn\]/{n;p}"') do @set "info_namecn=%%a"
for /f "delims=" %%a in ('type "%Temp%\info.sque" ^| %PIER_ROOT%\bin\sed.exe -n "/\[sourcename_en\]/{n;p}"') do @set "info_nameen=%%a"
for /f "delims=" %%a in ('type "%Temp%\info.sque" ^| %PIER_ROOT%\bin\sed.exe -n "/\[Category\]/{n;p}"') do @set "info_category=%%a"
for /f "delims=" %%a in ('type "%Temp%\info.sque" ^| %PIER_ROOT%\bin\sed.exe -n "/\[Owner\]/{n;p}"') do @set "info_owner=%%a"
for /f "delims=" %%a in ('type "%Temp%\info.sque" ^| %PIER_ROOT%\bin\sed.exe -n "/\[Admin\]/{n;p}"') do @set "info_admin=%%a"
if exist "%Temp%\info.sque" del /f /q "%Temp%\info.sque" > nul 2>&1
:: ?????????????
if /I "%custom%"=="https://steve372a.github.io/pier-repo" (
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "82p" %LANGUAGE_DIR%\lang.ini') do @set "official=%%a"
)

:: ???????????????????????????????????
if /I "%LANGUAGE_DIR%"=="%PIER_ROOT%\share\language\zh-CN" (
echo %source_name_label% %info_namecn% %official%
) else (
echo %source_name_label% %info_nameen% %official%
)
echo %source_owner_label% %info_owner%
echo %source_admin_label% %info_admin%
:: ?????????
if /I "%custom%"=="https://steve372a.github.io/pier-repo" goto nextchangemirror
:: ???????????????????
echo.
echo %repo_change_confirm%
echo %repo_change_confirm_2%
echo %repo_change_confirm_3%
:: Y??????N?????????????
echo|set /p "= (Y/N): "
SET /P INS=
If /I "%INS%"=="Y" echo. && goto nextchangemirror
goto quit
:nextchangemirror
:: ??????
if exist %PIER_ROOT%\etc\sourceimage.ini Del /f /s /q %PIER_ROOT%\etc\sourceimage.ini > nul
echo %custom%> %PIER_ROOT%\etc\sourceimage.ini
echo.
echo %repo_changed%
echo %custom%
goto quit

:: ??????
:repo_chg2
@echo %source_select_prompt%
@echo %source_select_options%
<nul set /p "=?????: "
SET /P INS=
:: ??????????
If /I "%INS%"=="1" (
    set "custom=https://steve372a.github.io/pier-repo"
    goto changesource
)
goto quit

:: =========================================================== ?????? ===========================================================
:changesource
:: ???
echo %checking_source%
if exist %Temp%\db.sque Del /f /s /q %Temp%\db.sque > nul
if /I "%custom%"=="" goto repo_chg2
:: ???%custom%??????????/???????
if "!custom:~-1!" == "/" ( 
    set "custom=!custom:~0,-1!"
)
:: ???%custom%????????????????
if "!custom:~-1!" == " " (
    set "custom=!custom:~0,-1!"
)
:: ?????????????????????
%PIER_ROOT%\bin\uma-get.exe -q -P "%Temp%" --no-check-certificate "%custom%/info.sque"
If not exist %Temp%\info.sque goto error_repo
:: ???? info ????????????????????????[???]\n?\n??????
for /f "delims=" %%a in ('type "%Temp%\info.sque" ^| %PIER_ROOT%\bin\sed.exe -n "/\[sourcename_cn\]/{n;p}"') do @set "info_namecn=%%a"
for /f "delims=" %%a in ('type "%Temp%\info.sque" ^| %PIER_ROOT%\bin\sed.exe -n "/\[sourcename_en\]/{n;p}"') do @set "info_nameen=%%a"
for /f "delims=" %%a in ('type "%Temp%\info.sque" ^| %PIER_ROOT%\bin\sed.exe -n "/\[Category\]/{n;p}"') do @set "info_category=%%a"
for /f "delims=" %%a in ('type "%Temp%\info.sque" ^| %PIER_ROOT%\bin\sed.exe -n "/\[Owner\]/{n;p}"') do @set "info_owner=%%a"
for /f "delims=" %%a in ('type "%Temp%\info.sque" ^| %PIER_ROOT%\bin\sed.exe -n "/\[Admin\]/{n;p}"') do @set "info_admin=%%a"
if exist "%Temp%\info.sque" del /f /q "%Temp%\info.sque" > nul 2>&1
:: ?????????????
if /I "%custom%"=="https://steve372a.github.io/pier-repo" (
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "82p" %LANGUAGE_DIR%\lang.ini') do @set "official=%%a"
)

:: ???????????????????????????????????
if /I "%LANGUAGE_DIR%"=="%PIER_ROOT%\share\language\zh-CN" (
echo %source_name_label% %info_namecn% %official%
) else (
echo %source_name_label% %info_nameen% %official%
)
echo %source_owner_label% %info_owner%
echo %source_admin_label% %info_admin%
:: ?????????
if /I "%custom%"=="https://steve372a.github.io/pier-repo" goto nextchangemirror
:: ???????????????????
echo.
echo %repo_change_confirm%
echo %repo_change_confirm_2%
echo %repo_change_confirm_3%
:: Y??????N?????????????
<nul set /p "= (Y/N): "
SET /P INS=
If /I "%INS%"=="Y" echo. && goto nextchangemirror
goto quit
:nextchangemirror
:: ??????
if exist %PIER_ROOT%\etc\sourceimage.ini Del /f /s /q %PIER_ROOT%\etc\sourceimage.ini > nul
echo %custom%> %PIER_ROOT%\etc\sourceimage.ini
echo.
echo %repo_changed%
echo %custom%
goto quit

:error_repo
:: ?????????????
echo %source_invalid%
goto quit

:: ??????
:repo_check
echo.
echo %lang_onlinelist%: 
echo %full_list_url%
echo %source_url_label%: 
echo %full_source_url%
goto quit

:: ?????
:installpackages
:: --- New: Use pier-pkg.exe for installation ---
if "%package%"=="" goto error2
"%PIER_ROOT%\bin\pier-pkg.exe" install "%PIER_ROOT%" "%LANGUAGE_DIR%" "%full_source_url%" "%full_pies_url%" "%2" "%SYS_ARCH%" %package%
if errorlevel 1 goto error_install
if errorlevel 2 goto quit
goto quit

:: --- Old installation code (commented out) ---
:: for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[loading_metadata\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "loading_metadata=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[choiceapp\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "choiceapp=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[autoyes\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "autoyes=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[install_progress\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "install_progress=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[download_progress\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "download_progress=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[error_package_not_exist\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "error_package_not_exist=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[error_install_failed\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "error_install_failed=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[language_installed_success\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "language_installed_success=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[list_package_name\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "list_package_name=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[list_version\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "list_version=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[list_os_requirement\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "list_os_requirement=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[list_description\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "list_description=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[install_space_usage\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "install_space_usage=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[list_author\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "list_author=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[list_distributor\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "list_distributor=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[install_space_usage_unit\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "install_space_usage_unit=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[alias_display\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "alias_display=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[prupdated\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "prupdated=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[package_installed\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "package_installed=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[list_architecture\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "list_architecture=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[arch_warning\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "arch_warning=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[arch_continue_prompt\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "arch_continue_prompt=%%a"

:: --- ???????????????? ---
if "%package%"=="" goto error2
echo %loading_metadata%
if exist "%PIER_ROOT%\share\cache\*.*" (
    del /f /q "%PIER_ROOT%\share\cache\*.*" > nul
)
if not exist "%PIER_ROOT%\share\cache" mkdir "%PIER_ROOT%\share\cache"
%PIER_ROOT%\bin\uma-get.exe -q -P "%Temp%" --no-check-certificate %full_source_url%/%package%.metadata
if not exist "%Temp%\%package%.metadata" goto error3
%PIER_ROOT%\bin\unzip.exe "%Temp%\%package%.metadata" -d "%PIER_ROOT%\share\cache" > nul
del /f /q "%Temp%\%package%.metadata" > nul

:: ??? sed ??????????
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[PackageName\]/{n;p}" %PIER_ROOT%\share\cache\metadata.sque') do @set "P_NAME=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[InstallerName\]/{n;p}" %PIER_ROOT%\share\cache\metadata.sque') do @set "P_INSTALLERNAME=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[Version\]/{n;p}" %PIER_ROOT%\share\cache\metadata.sque') do @set "P_VER=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[OS\]/{n;p}" %PIER_ROOT%\share\cache\metadata.sque') do @set "P_OS=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[InstallDir\]/{n;p}" %PIER_ROOT%\share\cache\metadata.sque') do @set "P_INSTALLDIR=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[ProFile\]/{n;p}" %PIER_ROOT%\share\cache\metadata.sque') do @set "P_DESC=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[PackageSize\]/{n;p}" %PIER_ROOT%\share\cache\metadata.sque') do @set "P_SIZE=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[Author\]/{n;p}" %PIER_ROOT%\share\cache\metadata.sque') do @set "P_AUTHOR=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[Distributor\]/{n;p}" %PIER_ROOT%\share\cache\metadata.sque') do @set "P_DISTRIBUTOR=%%a"

:: ??? pier-arch.exe ?????????????????
for /f "delims=" %%a in ('%PIER_ROOT%\bin\pier-arch.exe pkgarch %PIER_ROOT%\share\cache\metadata.sque') do set "P_ARCH=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\pier-arch.exe pkgfile %PIER_ROOT%\share\cache\metadata.sque') do set "PIE_FILE=%%a"

:: ?????????????????????XP ?????
set "test_path=%P_INSTALLDIR:~0,1%"
if "%test_path%"=="\" (
    set "P_INSTALLDIR=%P_INSTALLDIR:~1%"
)

:: --- ??????????????? ---
set "P_ACTION=install"
:: ??? vecho ??????????
%PIER_ROOT%\bin\vecho.exe $brightgreen$%list_package_name%: $brightwhite$%P_NAME%
%PIER_ROOT%\bin\vecho.exe $brightgreen$%list_version%: $brightwhite$%P_VER%
%PIER_ROOT%\bin\vecho.exe $brightgreen$%list_os_requirement%: $brightwhite$Windows %P_OS%
%PIER_ROOT%\bin\vecho.exe $brightyellow$%list_description%: $brightwhite$%P_DESC%
%PIER_ROOT%\bin\vecho.exe $brightyellow$%list_author%: $brightwhite$%P_AUTHOR%
%PIER_ROOT%\bin\vecho.exe $brightyellow$%list_distributor%: $brightwhite$%P_DISTRIBUTOR%
echo:
%PIER_ROOT%\bin\vecho.exe $brightgreen$%list_architecture%: $brightwhite$%P_ARCH%
%PIER_ROOT%\bin\vecho.exe $write$%install_space_usage%$brightcyan$ %P_SIZE% $write$%install_space_usage_unit%

:: ??????????
set "alias_list="
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[Alias\]/{:loop; n; /^::end/q; s/:.*//; p; b loop;}" %PIER_ROOT%\share\cache\metadata.sque') do (
    if defined alias_list (
        set "alias_list=!alias_list!, %%a"
    ) else (
        set "alias_list=%%a"
    )
)

if defined alias_list (
    %PIER_ROOT%\bin\vecho.exe $brightgreen$%alias_display% $brightyellow%%alias_list%
)

:: ???????????
for /f "delims=" %%a in ('%PIER_ROOT%\bin\pier-arch.exe check %PIER_ROOT%\share\cache\metadata.sque') do set "ARCH_CHECK=%%a"
echo %ARCH_CHECK% | findstr /b "warn:" >nul
if not errorlevel 1 goto show_arch_warning
goto skip_arch_warning
:show_arch_warning
echo.
echo %arch_warning% (%ARCH_CHECK:~5%) %arch_continue_prompt%
echo.
:skip_arch_warning

:: ????????????
if /I "%2%"=="-y" set "INS=Y"
if /I "%3%"=="-y" set "INS=Y"
if /I "%2%"=="y" set "INS=Y"
if /I "%3%"=="y" set "INS=Y"
if /I "%2%"=="yes" set "INS=Y"
if /I "%3%"=="yes" set "INS=Y"

if /I "%INS%"=="Y" goto download_package
if /I "%INS%"=="N" goto quit
if /I "%INS%"=="n" goto quit

:: ?????????
%PIER_ROOT%\bin\vecho.exe %choiceapp%
<nul set /p "= (Y/N): "
SET /P INS=

if /I "%INS%"=="N" goto quit
if /I "%INS%"=="n" goto quit

:download_package
echo %download_progress% %P_NAME%...

:: ??????? URL ??????
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[DesktopShortcut\]/{n;p}" %PIER_ROOT%\share\cache\metadata.sque') do @set "shortcut=%%a"
:: ????????
if exist "%Temp%\%PIE_FILE%" (
    del /f /q "%Temp%\%PIE_FILE%" > nul
)
%PIER_ROOT%\bin\uma-get.exe -P "%Temp%" --no-check-certificate %full_pies_url%/%PIE_FILE% -q --show-progress

echo wscript.sleep 200 >%Temp%\Wait.vbs
start /wait %Temp%\Wait.vbs

:: ????????????
if not exist "%Temp%\%PIE_FILE%" (
    echo %error_package_not_exist%
    goto quit
)

:: --- ???????????????????? ---
echo %install_progress% %P_NAME%...

:: ??Sanaka ??????????????????????????????????
if /I "%P_OS%"=="language" (
    :: ?????????????? zh-CN
    if /I "%package%"=="zh-CN" (echo %error_protected_lang% & goto quit)
    %PIER_ROOT%\bin\unzip.exe -o "%Temp%\%PIE_FILE%" -d "%PIER_ROOT%\share\language\" > nul
    echo %language_installed_success%
    goto quit
)

:: ================================================================
:: ??? App ??? (??????? else ???????????? sed ?????????????)
:: ================================================================

:: 1. ??????????
if not exist "%PIER_ROOT%\app\%P_INSTALLDIR%\" mkdir "%PIER_ROOT%\app\%P_INSTALLDIR%\"
%PIER_ROOT%\bin\unzip.exe -o "%Temp%\%PIE_FILE%" -d "%PIER_ROOT%\app\%P_INSTALLDIR%" > nul

%PIER_ROOT%\bin\vecho.exe $brightyellow$%package_installed% $brightcyan$%PIER_ROOT%\app\%P_INSTALLDIR%\

:: 2. ????????? (Metadata)
if not exist "%PIER_ROOT%\metadata\" mkdir "%PIER_ROOT%\metadata\"
if exist "%PIER_ROOT%\share\cache\metadata.sque" (
    copy /y "%PIER_ROOT%\share\cache\metadata.sque" "%PIER_ROOT%\metadata\%P_INSTALLERNAME%.sque" > nul
)

:: 5. ??????????? (Sanaka Registry) - ?????????????sed ???????
if not exist "%PIER_ROOT%\etc\" mkdir "%PIER_ROOT%\etc\"
:: ??? type nul ????????????? echo ????
if not exist "%PIER_ROOT%\etc\pierlist.sque" type nul > "%PIER_ROOT%\etc\pierlist.sque"

:: ?????????????
"%PIER_ROOT%\bin\sed.exe" -i "/^%P_INSTALLERNAME% | /d" "%PIER_ROOT%\etc\pierlist.sque" 2>nul

:: ???????? (?????????? echo ???????????)
set "reg_line=%P_INSTALLERNAME% | %P_VER% | %DATE% | %P_INSTALLDIR% | %source%"
(echo !reg_line!)>>"%PIER_ROOT%\etc\pierlist.sque"

:: ????????? ^ ??????????????????? Pier ?????????
%PIER_ROOT%\bin\vecho.exe $write$%prupdated%: $brightgreen$%P_INSTALLERNAME%

goto quit

:removepackages
:: --- New: Use pier-pkg.exe for removal ---
if "%package%"=="" goto error2_remove
"%PIER_ROOT%\bin\pier-pkg.exe" remove "%PIER_ROOT%" "%LANGUAGE_DIR%" "%full_source_url%" "%2" %package%
if errorlevel 1 goto error_remove
if errorlevel 2 goto quit
goto quit

:: --- Old removal code (commented out) ---
:: for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[loading_metadata\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "loading_metadata=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[choiceremove\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "choiceremove=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[uninstall_progress\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "uninstall_progress=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[uninstall_success\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "uninstall_success=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[package_installed\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "package_installed=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[remove_warning_text\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "remove_warning_text=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[remove_space_usage\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "remove_space_usage=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[remove_space_usage_unit\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "remove_space_usage_unit=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[list_package_name\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "list_package_name=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[list_version\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "list_version=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[package_not_installed\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "package_not_installed=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[error_package_not_exist\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "error_package_not_exist=%%a"

:: --- ?????????????????????? ---
if "%package%"=="" goto error2
echo %loading_metadata%
if exist "%PIER_ROOT%\share\cache\*.*" (
    del /f /q "%PIER_ROOT%\share\cache\*.*" > nul
)
if not exist "%PIER_ROOT%\share\cache" mkdir "%PIER_ROOT%\share\cache"
%PIER_ROOT%\bin\uma-get.exe -q -P "%Temp%" --no-check-certificate %full_source_url%/%package%.metadata
if not exist "%Temp%\%package%.metadata" goto error3
%PIER_ROOT%\bin\unzip.exe "%Temp%\%package%.metadata" -d "%PIER_ROOT%\share\cache" > nul
del /f /q "%Temp%\%package%.metadata" > nul

:: ??? sed ???????????????????????????????
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[Version\]/{n;p}" %PIER_ROOT%\share\cache\metadata.sque') do @set "packageversion=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[PackageName\]/{n;p}" %PIER_ROOT%\share\cache\metadata.sque') do @set "packagename=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[InstallerName\]/{n;p}" %PIER_ROOT%\share\cache\metadata.sque') do @set "installername=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[OS\]/{n;p}" %PIER_ROOT%\share\cache\metadata.sque') do @set "ossystem=%%a"
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[InstallDir\]/{n;p}" %PIER_ROOT%\share\cache\metadata.sque') do @set "P_INSTALLDIR=%%a"

:: ?????????????????????XP ?????
setlocal
set "test_path=%P_INSTALLDIR:~0,1%"
if "%test_path%"=="\" (
    endlocal
    set "P_INSTALLDIR=%P_INSTALLDIR:~1%"
) else (
    endlocal
)

:: --- ??????????? ---
set "is_installed=0"
if /I "%ossystem%"=="language" (
    if exist "%PIER_ROOT%\share\language\%package%\" (
        set "is_installed=1"
    )
) else (
    if exist "%PIER_ROOT%\app\%P_INSTALLDIR%\" (
        set "is_installed=1"
    )
)
if "!is_installed!"=="0" (
    echo %package_not_installed%
    goto quit
)

:: --- ????????CLI ?????? ---
set "P_ACTION=uninstall"
:: ????????
%PIER_ROOT%\bin\vecho.exe $brightgreen$%list_package_name%: $brightwhite$%packagename%
%PIER_ROOT%\bin\vecho.exe $brightgreen$%list_version%: $brightwhite$%packageversion%

:: ?????????????
set "folder_size_mb=0"
if /I "%ossystem%"=="language" (
    if exist "%PIER_ROOT%\share\language\%package%\" (
        set "total=0"
        for /f "delims=" %%f in ('dir /s /b "%PIER_ROOT%\share\language\%package%\" 2^>nul') do (
            set /a total+=%%~zf
        )
        set /a total_mb=total/1048576
        set /a remainder=total%%1048576
        set /a decimal=remainder*10/1048576
        set "folder_size_mb=!total_mb!.!decimal!"
    )
) else (
    if exist "%PIER_ROOT%\app\%P_INSTALLDIR%\" (
        set "total=0"
        for /f "delims=" %%f in ('dir /s /b "%PIER_ROOT%\app\%P_INSTALLDIR%\" 2^>nul') do (
            set /a total+=%%~zf
        )
        set /a total_mb=total/1048576
        set /a remainder=total%%1048576
        set /a decimal=remainder*10/1048576
        set "folder_size_mb=!total_mb!.!decimal!"
    )
)
%PIER_ROOT%\bin\vecho.exe $brightwhite$%remove_space_usage%: !folder_size_mb! %remove_space_usage_unit%

:: ??????????
%PIER_ROOT%\bin\vecho.exe %remove_warning_text%
echo.

if /I "!autoyes!"=="-y" (
    set "INS=Y"
) else (
    if /I "!autoyes!"=="y" (
        set "INS=Y"
    ) else (
        if /I "!autoyes!"=="yes" (
            set "INS=Y"
        ) else (
            :: ???
            %PIER_ROOT%\bin\vecho.exe %choiceremove% $brightred$%packagename%
            echo|set /p "= (Y/N): "
            SET /P INS=
        )
    )
)
if /I "%INS%"=="N" goto quit

:: --- ???????????????????? ---
echo %uninstall_progress% %packagename%...

:: ??? 1???????????????
if /I "%ossystem%"=="language" (
    :: ?????? zh-CN
    if /I "%package%"=="zh-CN" (
        for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[error_protected_lang\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "err_protect=%%a"
        echo !err_protect!
        goto quit
    )
    if exist "%PIER_ROOT%\share\language\%package%\" rd /s /q "%PIER_ROOT%\share\language\%package%\"
    goto :uninstall_end
)

:: ??? 2?????????? App ???
:: ????????????????????? InstallDir??
set "target_dir=%PIER_ROOT%\app\%P_INSTALLDIR%\"

:: ???????????????????????
if exist "%target_dir%uninstall.exe" (
    start /wait "" "%target_dir%uninstall.exe"
)

:: ???????????????????????????????
if exist "%target_dir%" (
    rd /s /q "%target_dir%"
)

:uninstall_end
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[uninstall_success\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "uninstall_ok=%%a"
echo !uninstall_ok!
:: ???????????????? InstallerName??
if exist "%PIER_ROOT%\metadata\%installername%.sque" (
    del /f /q "%PIER_ROOT%\metadata\%installername%.sque" > nul
)
:: ??pierlist.sque????????
"%PIER_ROOT%\bin\sed.exe" -i "/^%installername% | /d" "%PIER_ROOT%\etc\pierlist.sque" 2>nul
goto quit

:openpackage
:: pier o command - now handled by pier-op.exe for unlimited parameter support
:: Usage: pier o <package> [alias] [args...]
::        pier o <user/package> [alias] [args...] (third-party alias)

:: Check for help request
if /I "%package%"=="?" goto open_help
if /I "%package%"=="help" goto open_help
if /I "%package%"=="/" goto open_help
if /I "%package%"=="-h" goto open_help
if /I "%package%"=="--help" goto open_help

:: Check if package is specified
if "%package%"=="" (
    for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[error_no_package\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @echo %%a
    goto quit
)

:: Call pier-op.exe with all arguments
:: Arguments: PIER_ROOT LANGUAGE_DIR source full_source_url alias_source package [alias] [args...]
:: Note: We pass %2 onwards (skip %1 which is 'o')
set "_op_args=%2"
if not "%3"=="" set "_op_args=%_op_args% %3"
if not "%4"=="" set "_op_args=%_op_args% %4"
if not "%5"=="" set "_op_args=%_op_args% %5"
if not "%6"=="" set "_op_args=%_op_args% %6"
if not "%7"=="" set "_op_args=%_op_args% %7"
if not "%8"=="" set "_op_args=%_op_args% %8"
if not "%9"=="" set "_op_args=%_op_args% %9"
%PIER_ROOT%\bin\pier-op.exe "%PIER_ROOT%" "%LANGUAGE_DIR%" "%source%" "%full_source_url%" "%full_alias_source%" %_op_args%

:: pier-op.exe handles all output and execution
:: Return codes: 0=success, 1=error, 2=package not installed (third-party alias)
goto quit
:trim_whitespace
setlocal
set "str=!%~1!"
:trim_left
if "!str:~0,1!"==" " (
    set "str=%str:~1%"
    goto trim_left
)
:trim_right
if "%str:~-1%"==" " (
    set "str=%str:~0,-1%"
    goto trim_right
)
endlocal & set "%~1=%str%"
goto :eof

:open_help
:: Display help for pier o command
for /f "delims=" %%a in ('%PIER_ROOT%\bin\sed.exe -n "/\[open_alias_help\]/{n;p}" %LANGUAGE_DIR%\lang.ini') do @set "open_alias_help=%%a"
%PIER_ROOT%\bin\vecho.exe %%open_alias_help%%
goto quit

:error_install
goto quit

:error_remove
goto quit

:quit
if exist "%PIER_ROOT%\share\cache\metadata.sque" del /f /q "%PIER_ROOT%\share\cache\metadata.sque" > nul 2>&1
if exist "%PIER_ROOT%\sed*" del /f /q "%PIER_ROOT%\sed*" > nul 2>&1
.\bin\vecho.exe $cyan$Thanks for using Pier Package Installer by Sanakaprix.