@ECHO OFF
REM Updates the version number in files and creates appropriate commit and tag in git.
REM This does not compile, run tests, etc. This kind of validation must still be done manually beforehand!

SET PLUGIN_ROOT=%~dp0../
SET "VERSION_FILE=%PLUGIN_ROOT%.version"

SET /P LAST_VERSION=<"%VERSION_FILE%"
ECHO LAST_VERSION %LAST_VERSION%

SET /P NEW_VERSION=Enter the new version number (without prefix, i.e. in the format 'major.minor.patch'): 
SET NEW_VERSION_V=v%NEW_VERSION%

CALL :REPLACE_FILE "%VERSION_FILE%"
CALL :REPLACE_FILE "%PLUGIN_ROOT%README.md"
CALL :REPLACE_FILE "%PLUGIN_ROOT%OpenUnrealUtilities.uplugin"

git commit -m "Increased version to %NEW_VERSION%"
git tag %NEW_VERSION_V% -m "%NEW_VERSION_V%"

EXIT /B %ERRORLEVEL%

:REPLACE_FILE
SET REPLACE_FILENAME=%1
powershell -Command "(gc %REPLACE_FILENAME%) -replace '%LAST_VERSION%', '%NEW_VERSION%' | Out-File -encoding utf8 %REPLACE_FILENAME%
git add %REPLACE_FILENAME%
EXIT /B %ERRORLEVEL%
