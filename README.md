## Command Bar

A thing that appears on your screen.

![cmdbar main window](https://i.imgur.com/mJOI1Wx.jpg)

### Requirements
* Windows Vista/7/8/10. XP and older are not supported (because we are using DirectWrite and Direct2D).

### cmds.ini
* Commands are loaded from cmds.ini, currently hardcoded to `D:/Vlad/cb/cmds.ini`.
```ini
[open_dir]
name=dls
path=D:\Downloads
[run_app]
name=subl
path=D:\Soft\Sublime Text 3\sublime_text.exe
```
#### Common params
* name - string - command name.
#### open_dir params
Opens directory in Windows Explorer
* path - string - path to directory to open.
#### run_app params
Launches application. All arguments from Command Bar text field will be passed to the application (`notepad D:\yay.txt` will work as expected).
* path - string - path to the application to launch.
* shell_exec - bool - use ShellExecute instead of CreateProcess to launch application.
* run_as_admin - bool - run app as administrator (when shell_exec is true).
* args - string - args that will be passed to the application (when shell_exec is true) alongside with Command Bar text field arguments (`args` come first).
* work_dir - string - working directory for app (when shell_exec is true).
#### Param data types
* string - string, parsed as is without expanding environment vars or escaping characters (but string is trimmed from tabs/spaces).
* bool - boolean, true values: 1, true, yes, y; false values: 0, false, no, n
