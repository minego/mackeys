# mackeys

- Map super+x to shift+delete
- Map super+c to ctrl+insert
- Map super+v to shift+insert

## Why?!

When I press ctrl+c the outcome is not consistent. If I'm in a terminal then it
sends a break. If I'm not in a terminal then it does a copy on Linux, but not on
macOS.

I want a consistent set of key bindings for copy and paste that will behave the
same regardless of the application that I'm in and regardless of the OS that I
am on.

On macOS this is pretty easy. Using command+c and command+v behaves pretty much
everywhere, including in a terminal. Unfortunately those bindings don't do
anything on a Linux system. This is an attempt to fix that.

Mapping these bindings to ctrl+c and ctrl+v is sufficient for most applications
but does not work in a terminal but using ctrl+insert and shift+insert does work
both in terminals (at least the ones I've tried) and in most Linux gui
applications.


Execution
```
mackeys - Use macOS style copy and paste shortcuts on Linux

usage mackeys [-h | [-t | -T | [-c cmd]] [-k keys] [-s]]

options:
	-h			Show this message and exit
	-d          Delay used for key sequences (default: 20000 microseconds)\n"
	
```

## Dependencies

[Interception Tools](https://gitlab.com/interception/linux/tools)


