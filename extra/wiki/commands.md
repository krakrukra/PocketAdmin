The command language used in payload files is based on existing [ducky script](https://github.com/hak5darren/USB-Rubber-Ducky/wiki/Duckyscript),  
originally made for the USB rubber ducky. This is to make the old rubber ducky  
payloads still work with PocketAdmin. There are, however, many differences and  
extentions, so it will not work the other way around.  
  
Here is a quick summary of the rules:  
  
1. Text encoding must be ASCII. Any symbol in a script that is  
not ASCII-printable causes the rest of the line to be ignored.  
Quotation marks used here are not parts of commands and  
are just used to indicate start and end of a command.  
  
2. All **special functionality** commands must be placed at the start of  
the line and terminated with a newline character. Other commands, such  
as **press key** or **mouse control** commands can be placed several times  
on the same line, with space characters separating them. In that case, all  
these key presses / mouse actions are considered to be applied simulteneously.  
  
3. Some commands take arguments, which follow the command keyword.  
These arguments will be denoted **n** for decimal numbers, **x** for hex  
numbers, **s** for strings or characters. The keyword and it's argument  
must be separated by exactly one space character.  
  
4. Hex arguments must have 0x prefix when used in scripts, but case of  
the letters doesn't matter (eg. 0x02ac, 0x02AC, 0x02aC are the same).  
Size of a decimal argument is allowed to be 1-6 symbols, of a hex  
argument 1-4 symbols, of a string argument 0-1000 symbols.  
  
5. a command keyword together with it's **numeric** argument are conidered  
as a single command (eg. "KEYCODE 0x04", "MOUSE_RIGHT 10"). You can have  
up to 10 different **press key** or **mouse control** commands on the same line.  
You can press up to 4 non-modifier keyboard keys simultaneously.  
  
6. There are no particular requirements for the order of commands on the  
same line (eg. "CTRL SHIFT a b" and "a SHIFT CTRL b" are the same). Also,  
you are not limited in which key combos are allowed and can use any  
arbitrary combination of **press key** and **mouse control** commands.  
  
  
### Below is a complete list of all payload commands:  
  
#### special functionality commands:  
* "REM **s**"   --- skips to the next line, used for comments  
* "DEFAULT_DELAY **n**"   --- sets how many milliseconds to wait after every command  
* "DEFAULTDELAY **n**"   --- same as "DEFAULT_DELAY **n**"  
* "ONACTION_DELAY **n**"   --- applies default delay, but only after some of commands  
* "DELAY **n**"   --- waits a specified number of milliseconds  
* "WAITFOR_INIT"   --- waits until host machine has finished device initialization  
* "WAITFOR_CAPSLOCK"   --- waits until user has manually toggled capslock 2 times  
* "WAITFOR_RESET"   --- waits until host machine resets the device (eg. on reboot)  
* "ALLOW_EXIT"   ---  gives user 1 second to toggle capslock, exits script if detected  
* "REPEAT_START"   --- marks the beginning of a repeat block  
* "REPEAT **n**"   --- repeats previous command block for a specified number of times  
* "STRING_DELAY **n**"   --- sets how long to keep a key pressed/released with "STRING **s**"  
* "STRING **s**"   --- types in an ASCII-printable character string  
* "HOLD **s**"   --- keeps specified keys / mouse clicks continuously pressed  
* "RELEASE"   --- releases all keys / mouse clicks held by "HOLD **s**" command  
* "SETNUM_ON"   --- toggle NUMLOCK until it's LED is turned ON  
* "SETNUM_OFF"   --- toggle NUMLOCK until it's LED is turned OFF  
* "SETCAPS_ON"   --- toggle CAPSLOCK until it's LED is turned ON  
* "SETCAPS_OFF"   --- toggle CAPSLOCK until it's LED is turned OFF  
* "SETSCROLL_ON"   --- toggle SCROLLLOCK until it's LED is turned ON  
* "SETSCROLL_OFF"   --- toggle SCROLLLOCK until it's LED is turned OFF  
  
#### press key commands:  
* "GUI"   --- applies left GUI modifier key  
* "WINDOWS"   --- same as "GUI"  
* "CTRL"   --- applies left CTRL modifier key  
* "CONTROL"   --- same as "CTRL"  
* "SHIFT"   --- applies left SHIFT modifier key  
* "ALT"   --- applies left ALT modifier key  
* "RGUI"   --- applies right GUI modifier key  
* "RCTRL"   --- applies right CTRL modifier key  
* "RSHIFT"   --- applies right SHIFT modifier key  
* "RALT"   --- applies right ALT modifier key  
*   
* "KEYCODE **n**"   --- sends a key by HID keycode (decimal)  
* "KEYCODE **x**"   --- sends a key by HID keycode (hex)  
* "MENU"   --- key to open a menu (often the same as right click)  
* "APP"   --- same as "MENU"  
* "ENTER"  
* "RETURN"   --- same as "ENTER"  
* "DOWN"  
* "LEFT"  
* "RIGHT"  
* "UP"  
* "DOWNARROW"   --- same as "DOWN" 
* "LEFTARROW"   --- same as "LEFT"  
* "RIGHTARROW"   --- same as "RIGHT"  
* "UPARROW"   --- same as "UP"  
* "PAUSE"  
* "BREAK"   --- same as "PAUSE"  
* "CAPSLOCK"  
* "DELETE"  
* "END"  
* "ESC"  
* "ESCAPE"   --- same as "ESC"  
* "HOME"  
* "INSERT"  
* "NUMLOCK"  
* "PAGEUP"  
* "PAGEDOWN"  
* "PRINTSCREEN"  
* "SCROLLLOCK"  
* "SPACE"  
* "SPACEBAR" --- same as "SPACE"  
* "TAB"  
* "F1"  
* "F2"  
* "F3"  
* "F4"  
* "F5"  
* "F6"  
* "F7"  
* "F8"  
* "F9"  
* "F10"  
* "F11"  
* "F12"  
* ASCII-printable characters, except space ("a", "b", "G", "/", etc)  
*   
* "KP_SLASH"  --- keypad "/"  
* "KP_ASTERISK"  --- keypad "*"  
* "KP_MINUS"  --- keypad "-"  
* "KP_PLUS"  --- keypad "+"  
* "KP_ENTER"  --- keypad ENTER  
* "KP_1"  --- keypad "1" or END  
* "KP_2"  --- keypad "2" or DOWNARROW  
* "KP_3"  --- keypad "3" or PAGEDOWN  
* "KP_4"  --- keypad "4" or LEFTARROW  
* "KP_5"  --- keypad "5"  
* "KP_6"  --- keypad "6" or RIGHTARROW  
* "KP_7"  --- keypad "7" or HOME  
* "KP_8"  --- keypad "8" or UPARROW  
* "KP_9"  --- keypad "9" or PAGEUP  
* "KP_0"  --- keypad "0" or INSERT  
* "KP_PERIOD"  --- keypad "." or DELETE  
  
  
#### mouse control commands:  
* "MOUSE_LEFTCLICK"  
* "MOUSE_RIGHTCLICK"  
* "MOUSE_MIDCLICK"  
*   
* "MOUSE_RIGHT **n**" --- move mouse cursor right by **n** units  
* "MOUSE_LEFT **n**" --- move mouse cursor left by **n** units  
* "MOUSE_UP **n**" --- move mouse cursor up by **n** units  
* "MOUSE_DOWN **n**" --- move mouse cursor down by **n** units  
* "MOUSE_SCROLLDOWN **n**" --- scroll mouse wheel by **n** units towards the user  
* "MOUSE_SCROLLUP **n**" --- scroll mouse wheel by **n** units away from user  
  
---
  
Below is a more detailed explaination for some commands:  
  
1. "WAITFOR_INIT" command is intended to be used at startup, as a first ever  
command in a script. It actively taps on capslock key and waits for host response.  
Once the host responds with a request to light up the capslock LED, this command  
makes sure that the capslock is turned off. If default HID+MSD configuration is  
used, then "WAITFOR_INIT" will also wait until at least 1 read command was sent  
to the MSD interface, making sure that it is configured as well. This allows you  
to make sure that the host is ready to receive keystrokes, even if there was a  
rather long delay because of drivers being installed; it also makes sure that your  
payload will be executed the same way, no matter if capslock was turned on or off  
before device was inserted. Payloads which use this command can run successfully  
on the very first insertion, and at the same time you don't have to hardcode  
a huge delay in the script and waste more time than necessary.  
  
2. "WAITFOR_CAPSLOCK" command waits until user manually taps capslock 2 times  
(on a real keyboard), and only then continues with the payload execution. This is  
useful when you do not know exactly how long should the device wait at a certain  
point in the script. For example, you might be starting up some program, or waiting  
for UAC prompt to show up, etc. Depending on the situation it might take a variable  
amount of time before the next part of the script should be executed. Also, you can  
use this command whenever you need to mix badusb input and manual input. For  
example, badusb is logging into some online account, but you need to solve a  
CAPTCHA; so you pause the payload and solve the CAPTCHA manually, then badusb  
can continue with the script. Or maybe you are in a BIOS boot menu, but you  
are not exactly sure in what order the drives will be placed; so you select  
the drive manually and continue, etc.  
  
3. "WAITFOR_RESET" command waits until the host machine sends a USB reset signal,  
and is mostly intended to detect a host reboot. Depending on the situation, target  
PC might take a variable amount of time to reboot, but once it reboots there is a  
very limited amount of time to enter the BIOS / boot menu, etc; this makes normal  
delays unsuitable for payloads which involve rebooting into BIOS. Keep in mind  
though, that if the device will be actually powered off, then script execution  
cannot continue. In that case you should use the OS fingerprinter to select  
appropriate BIOS-specific payload once the power is turned on again.  
  
4. "ONACTION_DELAY **n**" command is similar to "DEFAULT_DELAY **n**", and is in  
fact it's alternative. That is, you cannot use them simultaneously and if one of  
these commands is encountered in the script, it overwrites any effect that the  
previous command had. Both commands are used to automatically insert a  
specified delay (in milliseconds) after subsequent commands. The difference  
is, "DEFAULT_DELAY **n**" inserts a delay after any script line whatsoever, including  
"DEFAULT_DELAY **n**" itself, "REM **s**", "DELAY **n**", empty lines, etc; while the  
"ONACTION_DELAY **n**" only adds delay after "STRING **s**", "HOLD **s**", "RELEASE",  
or any combination of **press key** and **mouse control** commands.  
"DEFAULT_DELAY **n**" command only exists for compatibility with ducky script  
and is not recommended for use in your own scripts.  
  
5. "ALLOW_EXIT" command provides a means to stop current payload execution. It  
waits for 1 second while watching for user-initiated capslock toggles. If capslock  
was pressed 2 or more times, then the payload script will be abandoned and the  
device will go back to idle state, waiting for user to select some on-demand  
payload. If capslock toggles were not detected during the delay, payload execution  
will continue. You should make sure that that the device was successfully  
initialized before this command is encountered. In fact, usually this command  
is most useful after some sort of dynamic delay, eg. "WAITFOR_INIT", "ALLOW_EXIT"  
sequence allows you to have a payload that can be blocked by continuosly tapping  
on capslock while inserting the device (so you don't have to take the device  
apart and use MSD-only button for this).  
  
6. "STRING_DELAY **n**" command provides you a capability to slow down the rate  
at which subsequent "STRING **s**" commands will be typing specified characters.  
Normally, the device will send one report with pressed keys, and in the next frame,  
that is, after 1ms it will send a report with keys being released. If for some  
reason the host machine cannot keep up with such typing rate, specifying a  
"STRING_DELAY **n**" will make the device press a key and keep it pressed for **n**  
extra milliseconds. After this, the key will be released and again the extra **n** ms  
delay will be inseted before the next character will be typed in.  
  
7. "STRING **s**" command only accepts ASCII-printable characters, max length of the  
string is 1000 characters. By default a US-compatible keyboard layout is expected.  
If you have a different layout you need to change this by "USE_LAYOUT **s**"  
pre-configuration command in config.txt; otherwise symbols from script will not be  
typed in correctly. If you do have a US-compatible layout but you want to type a  
string in a different language, you should set the GUI to use your desired language  
and then use "STRING **s**" command with ASCII symbols that are bound to the same  
physical keys as the symbols that you are trying to print. For example, if GUI is  
configured to have RU input, "STRING Dtkjcbgtl CNTKC" command will result in  
"Велосипед СТЕЛС" string typed. (because you have to press the same key to  
type 'l' or 'д', same key for 'k' or 'л', etc)  
  
8. Mouse control commands which have to do with moving the pointer or the wheel  
take decimal arguments, ranging from 0 to 127. These arguments are relative  
values which represent displacement from the previous location and are expressed  
in logical units. It is completely up to the host machine to decide what these  
values actually mean and what postprocessing should be done on them. For example,  
dispacement by 10 units might mean 10 or 20 pixels, depending on the sensitivity  
setting. Also, host postprocessing might result in eg. "MOUSE_RIGHT 100" not  
being equivalent to "MOUSE_RIGHT 10" executed 10 times in a row. That means you  
should be extra careful and make correct assumptions about target machine to use  
mouse control successfully. Commands which have to do with moving mouse pointer  
on different axies (or add scroll) can be combined on a single line, for example,  
"MOUSE_RIGHT 120 MOUSE_DOWN 35 MOUSE_SCROLLUP 47" is a valid command.  
  
9. Single ASCII-printable character commands are available, which will type  
out that character. The appropriate modifiers (left SHIFT and right ALT keys)  
are applied automatically. That is, both "m" and "M" commands send the same  
keycode, but for "M" SHIFT modifier will be used. This also means you must use  
lowercase letters for key combos such as "GUI r" ("GUI R" means "GUI SHIFT r").  
You can, however, add more modifiers, like this: "CTRL SHIFT t" or "GUI CTRL M"  
  
10. "KEYCODE **n**", "KEYCODE **x**" commands are available, in case you want to  
send a keystroke by HID keycode (eg. you might want to type a ASCII non-printable  
character). The acceped keycode values are from 0 to 221 (decimal).  
Some information about keycodes can be found in [this document](https://usb.org/document-library/hid-usage-tables-112), pages 52-59.  
It is possible to use modifier keys with it, for example: "CTRL SHIFT KEYCODE 23"  
  
11. "HOLD **s**" command applies to keyboard keys (both modifiers or not) and also  
to mouse clicks. It is used whenever you need some keys continuously pressed, eg.  
while in windows 10 language selection menu, opened with GUI + SPACE keys. This  
command also affects output of "STRING **s**" and also of any **press key** or  
**mouse control** commands. To release all the keys/clicks currently held down  
you should use the "RELEASE" command. If you want to release only some  
particular keys, you can use several "HOLD **s**" commands in a row, since they  
discard any previously held keys. For example, the following command sequence  
"HOLD f g" "DELAY 500" "HOLD f" "DELAY 500" "RELEASE" will keep both "f" and "g"  
keys pressed for 500ms, then release only the "g" key, and only 500ms later  
release "f" as well.  
  
12. "REPEAT **n**" command can repeat one or more previous commands for a specified  
number of times. If a block of commands is to be repeated, the beginning of this  
block should be marked by a "REPEAT_START" command. The size of a repeat block  
is unlimited, so it can span for the entire length of the payload script. Once the  
"REPEAT **n**" command is completed, the previous repeat block is over and a new  
"REPEAT_START" command is expected. In case a "REPEAT **n**" command is used  
without a script block beginning explicitly specified, only the command  
immediately preceding the "REPAT **n**" will be repeated. Some examples:  
\
The following script will run 3 commands right before REPEAT for  
11 times (once normally + repeated 10 times):  
"REPEAT\_START"  
"STRING this command block of 3 commands is repeated"  
"ENTER"  
"DELAY 300"  
"REPEAT 10"  
\
The following script will run 1 command right before REPEAT for  
5 times (once normally + repeated 4 times):  
"CTRL ALT DELETE"  
"REPEAT 4"  
  