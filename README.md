# mackeys

## Why?!

As a heavy terminal user I've always been frustrated that the normal keystrokes
for copy and paste can't be used in the terminal. After spending some time using
macOS I have learned to really like being able to use command+c and command+v
to copy and paste everywhere including the terminal.

To make this work properly requires knowing if the currently focused window is a
terminal or not, and there is no easy way to know that.

Execution
```
mackeys - Use macOS style copy and paste shortcuts on Linux

usage mackeys [-h | [-t | -T | [-c cmd]] [-k keys]]

options:
	-h			Show this message and exit
	-t			Assume terminal mode, send ctrl+shift+<key>
	-T			Assume NOT terminal mode, send ctrl+<key>
	-c cmd		Command to run to determine mode. An exit code of 0 is used to
				indicate terminal mode.
	-k keys		Specify a list of keys that should be converted. Default: cv
```

## Dependencies

[Interception Tools](https://gitlab.com/interception/linux/tools))


