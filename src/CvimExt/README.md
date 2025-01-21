# README

## README.md for the cvimext DLL.

Written by Anthony Daves.


### INSTALLATION

To install the `Edit with Vim` popup menu entry, it is recommended to use the
`install.exe` program.  It will ask you a few questions and install the needed
registry entries.

In special situations you might want to make changes by hand.  Check these
items:
- The cvimext.dll, gvim.exe and uninstall.exe either need to be in the search
  path, or you have to set the full path in the registry entries.  You could
  move the cvimext.dll to the `C:\Windows\System` or `C:\Windows\System32`
  directory, where the other DLL files are.
- You can find the names of the used registry entries in the file
  `CvimExt.reg`.  You can edit this file to add the paths.  To install the
  registry entries, right-click the gvimext.reg file and choose the `merge`
  menu option.
- The registry key [HKEY_LOCAL_MACHINE\Software\cVim\Cvim] is used by the
  `cvimext.dll`.  The value "path" specifies the location of `cvim.exe`.  If
  `cvim.exe` is in the search path, the path can be omitted.  The value "lang"
  can be used to set the language, for example "de" for German.  If "lang" is
  omitted, the language set for Windows will be used.

It is the preferred method to keep gvim.exe with the runtime files, so that
Vim will find them (also the translated menu items are there).


### UNINSTALLATION

To uninstall the `Edit with Vim` popup menu entry, it is recommended to use
the `uninstall.exe` program.

In special situations you might want to uninstall by hand:
- Open the registry by running `regedit.exe`.
- Delete all the keys listed in `CvimExt.reg`, except this one:
  [HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved]
  For this key, only delete one value:
  "{51EEE242-AD87-11d3-9C1E-0090278BBD99}"="cVim Shell Extension"
- Delete the cvimext.dll, if you want.  You might need to reboot the machine
  in order to remove this file.  A quick way is to log off and re-login.

Another method is by using the uninst.bat script:
    uninst cvimext.inf
This batch file will remove all the registry keys from the system.  Then you
can remove the cvimext.dll file.
Note: In order for this batch file to work, you must have two system files:
rundll32.exe and setupapi.dll.  I believe you will have rundll32.exe in your
system.  I know windows nt 4.0 with the service pack 4 has setupapi.dll.  My
windows 95 has setupapi.dll.  I find that the internet explorer 4.0 comes with
the setupapi.dll in file Ie4_5.cab.

If you do encounter problems running this script, then probably you need to
modify the uninst.bat to suit to your system.  Basically, you must find out
where are the locations for your rundll32.exe and setupapi.dll files.
You can run this script:
```bash
    rundll32.exe c:\windows\system\setupapi.dll,InstallHinfSection DefaultUninstall 128 %1
```
where %1 can be substitued by cvimext.inf


### THE SOURCE CODE

I have provided the source code here in hope that cVim users around world can
further enhance this little dll.  I believe the only thing you need to change
is cvimext.cpp file.  The important two functions you need to look at are
QueryContextMenu and InvokeCommand.  You can modify right-click menus in the
QueryContextMenu function and invoke gvim in the InvokeCommand function.  Note
the selected files can be accessed from the DragQueryFile function.  I am not
familiar with the invoking options for cVim.  I believe there are some
improvements that can be made on that side.

I use MS Visual C++ 6.0's nmake to make the cvimext.dll.  I don't have a
chance to try earlier versions of MSVC.  The files that are required for build
are:
	cvimext.cpp
	cvimext.h
	cvimext.def
	cvimext.rc
	resource.h
	Makefile

To compile the DLL from the command line:
```bash
   vcvars32
   nmake -f Makefile
```

If you did something interesting to this DLL, please let me know
@adaves1.

Happy cvimming!!!
