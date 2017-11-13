# Description
Set Process Environment Hack
This initial add is the POC where I've proved that it works.  It's compiled to work with VS 2016 x64 and runs successfully on Windows 10.
This hack does a very simple DLL injection technique via CreateRemoteThread that will modify the targeted processes environment with a new name/value pair as a first demo.

# Limitations
* The injected DLL must be in the current directory or in the path of the program that is to be injected follwing the rules for the LoadLibrary function.

# How to make it work
*  Currently only runs on notepad.  So from a command prompt:
*  Copy the SetProcEnv.exe and the IP0wnYou.dll into the current directory
*  Execute notepad.exe from the command line
*  Start a copy of process explorer, show the current dlls and the current envrionment in that notepad.exe 
*  Execute the SetEnvProc.exe.  The output is the lower 32 bites of the newly mapped DLL's address.
*  Note now that after a refresh the environment variables show a new name/value pair named 'YOURCOMPUTER' which is 
set to 'P0wned'
*  Verify that the IP0wnedYou.dll is now loaded as a dll from with the notepad.exe process itself.

# To Do
* Clean UP Code
* Make it nice and OO'ified
* Notify waiters I'm done.




