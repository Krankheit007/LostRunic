@echo off
rem Test runner: unattended automation run for all LostRunic tests, exits when done.
"D:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "D:\25DGame\LostRunic\LostRunic.uproject" -unattended -nopause -ExecCmds="Automation RunTests LostRunic; Quit"
