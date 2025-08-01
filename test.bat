start .\server.exe

for /l %%i in (1,1,9) do (
    start .\client.exe
)
pause
