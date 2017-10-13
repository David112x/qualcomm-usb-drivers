start /D%1 /MAX /WAIT CMD.EXE /C msbuild.exe %2 /t:Rebuild /p:Configuration=%3 /p:Platform=%4 /l:FileLogger,Microsoft.Build.Engine;logfile=%5
