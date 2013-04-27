@echo off

for /f %%i in ('bin\win-bash\uname') do set OS=%%i
bin\Win-Bash\bash.exe build.sh -o %OS% %*