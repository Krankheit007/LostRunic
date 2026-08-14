@echo off
rem Build wrapper: avoids Git Bash / cmd quoting issues with paths containing spaces.
call "D:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" LostRunicEditor Win64 Development -Project="D:\25DGame\LostRunic\LostRunic.uproject" %*
