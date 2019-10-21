The command language used in payload files is heavily based on existing [ducky script](https://github.com/hak5darren/USB-Rubber-Ducky/wiki/Duckyscript),  
originally made for the USB rubber ducky. This is to make the old rubber ducky  
payloads still work with PocketAdmin. There are, however, many differences and  
extentions, so it will not work the other way around.  

Here is a quick summary of the rules:  
1. All commands must be placed at the start of the line and end  
with a newline character.  

2. Any symbol in a script that is not ASCII-printable  
causes the rest of the line to be ignored.  

3. Quotation marks used here are not parts of commands  
and are just used to indicate start and end of a command.  

4. Some commands take arguments, which follow the command keyword.  
These arguments will be denoted **n** for decimal numbers,  
**x** for hex numbers, **s** for strings or characters.  

5. Hex numbers must have 0x prefix when used in scripts, but case  
of the letters doesn't matter (eg. 0x02ac and 0x02AC are the same)  

6. Maximum length of a decimal argument is 6 symbols,  
of a hex argument 4 symbols, of string 400 symbols.  
  
### Below is a complete list of all available commands:  
  
#### special functionality commands:  
* "REM **s**"   --- skips to the next line, used for comments  
* "DEFAULT_DELAY **n**"   --- sets how many milliseconds to wait between all commands  
* "DEFAULTDELAY **n**"   --- same as "DEFAULT_DELAY **n**"  
* "DELAY **n**"   --- waits a specified number of milliseconds  
* "STRING **s**"   --- types in an ASCII-printable character string
* "REPEAT_SIZE **n**"   --- sets how many commands to repeat with "REPEAT **n**" command  
* "REPEAT **n**"   --- repeats previous command block for a specified number of times  
* "HOLD **s**"   --- keeps specified modifier keys pressed  
* "RELEASE"   --- releases modifiers set by "HOLD **s**" command  
  
#### modifier key commands:  
* "GUI **s**"   --- applies left GUI modifier to a keystroke  
* "WINDOWS **s**"   --- same as "GUI **s**"  
* "CTRL **s**"   --- applies left CTRL modifier to a keystroke  
* "CONTROL **s**"   --- same as "CTRL **s**"  
* "SHIFT **s**"   --- applies left SHIFT modifier to a keystroke  
* "ALT **s**"   --- applies left ALT modifier to a keystroke  
* "RGUI **s**"   --- applies right GUI modifier to a keystroke  
* "RCTRL **s**"   --- applies right CTRL modifier to a keystroke  
* "RSHIFT **s**"   --- applies right SHIFT modifier to a keystroke  
* "RALT **s**"   --- applies right ALT modifier to a keystroke  
  
#### single modifier key commands  
* "GUI"   --- sends left GUI modifier without keycode  
* "WINDOWS"   --- same as "GUI"  
* "CTRL"   --- sends left CTRL modifier without keycode  
* "CONTROL"   --- same as "CTRL"  
* "SHIFT"   --- sends left SHIFT modifier without keycode  
* "ALT"   --- sends left ALT modifier without keycode  
* "RGUI"   --- sends right GUI modifier without keycode  
* "RCTRL"   --- sends right CTRL modifier without keycode  
* "RSHIFT"   --- sends right SHIFT modifier without keycode  
* "RALT"   --- sends right ALT modifier without keycode  

#### press key commands:  
* "KEYCODE **n**"   --- sends a key by HID keycode (decimal)  
* "KEYCODE **x**"   --- sends a key by HID keycode (hex)  
* "MENU"  
* "APP"  
* "ENTER"  
* "RETURN"  
* "DOWN"  
* "LEFT"  
* "RIGHT"  
* "UP"  
* "DOWNARROW"  
* "LEFTARROW"  
* "RIGHTARROW"  
* "UPARROW"  
* "PAUSE"  
* "BREAK"  
* "CAPSLOCK"  
* "DELETE"  
* "END"  
* "ESC"  
* "ESCAPE"  
* "HOME"  
* "INSERT"  
* "NUMLOCK"  
* "PAGEUP"  
* "PAGEDOWN"  
* "PRINTSCREEN"  
* "SCROLLLOCK"  
* "SPACE"  
* "SPACEBAR"  
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
* single ASCII-printable character commands ("a", "b", "G", "/", etc)  
  
#### pre-configuration commands:  
* "USE_FINGERPRINTER"   --- enables OS fingerprinter  
* "USE_LAYOUT **s**"   --- loads new keyboard layout from a file  
* "HID_ONLY_MODE"   --- switches device to HID-only mode  
* "VID **x**"   --- sets VID value for enumeration  
* "PID **x**"   --- sets PID value for enumeration  
  
---

Below is a more detailed explaination for some commands:  
  
1. "DELAY **n**" command first waits extra time until host had sent at least 1 read  
command to MSD interface (in MSD+HID configuration) and received at least  
1 report from HID interface (all configurations), and only then waits for  
a specified number of milliseconds. This is done to help payloads run at the  
very first insertion, even into a completely new machine. The problem is, it  
might take many seconds before host will do its initializations (e.g. install  
drivers). This way you dont have to put huge delays at the start of the payload,  
and the "DELAY **n**" command only waits for GUI elements to update.  

2. "STRING **s**" command only accepts ASCII-printable characters, max length of the  
string is 400 characters. By default a US-compatible keyboard layout is expected.  
If you have a different layout you need to change this by "USE_LAYOUT **s**"  
pre-configuration command, otherwise symbols from script might not be  
typed in correctly. If you do have a US-compatible layout but you want to type a  
string in a different language, you should set the GUI to use your desired  
language and then use "STRING **s**" command with ASCII symbols that are bound to  
the same physical keys as the symbols that you are trying to print.  
For example, if GUI is configured to have RU input, "STRING Dtkjcbgtl CNTKC"  
command will result in "Велосипед СТЕЛС" string typed. (because you have to  
press the same key to type 'l' or 'д', same key for 'k' or 'л', etc)  

3. Single ASCII-printable character commands are available, which will type  
out that character. The appropriate modifiers (left SHIFT and right ALT keys)  
are applied automatically. That is, both "m" and "M" commands send the same  
keycode, but for "M" SHIFT modifier will be used. That means you must use  
lowercase letters for key combos such as "GUI r" ("GUI R" means "GUI SHIFT r").  
You can, however, add more modifiers, like this: "CTRL SHIFT t" or "GUI CTRL M"  

4. "KEYCODE **n**", "KEYCODE **x**" commands are available, in case you want to  
send a keystroke by HID keycode (e.g. you might want to print out a non  
ascii-printable character). The acceped keycode values are from 0 to 101 (decimal).  
Some information about keycodes is in /extra/HID_UT1_12.pdf file, pages 52-58.  
It is possible to use modifier keys with it, for example: "CTRL SHIFT KEYCODE 23"  

5. You can use up to 4 modifier key commands on one line, with separate keywords  
for left or right modifier keys. You can have a modifier key pressed without a  
keycode sent along with it. Examples: "CTRL RALT DELETE", "CTRL SHIFT t", "ALT"  

6. "HOLD **s**" command only holds modifier keys and no others. It can be  
used when you need some modifier keys continuously pressed (e.g. while in  
windows 10 language selection menu, opened with GUI + SPACE keys)  
This command affects output of single-key commands and "STRING **s**" command.  
examples: "HOLD RGUI SHIFT", "HOLD CTRL", "HOLD GUI"  
To release the "HOLD **s**" modifier keys use "RELEASE" command.  

7. "REPEAT **n**" command can repeat a block of commands (max block  
size = 400 characters). "REPEAT\_SIZE **n**" command specifies the number  
of commands in this block (at poweron REPEAT_SIZE is set to 1).  
The following script will run 3 commands right before REPEAT for  
11 times (once normally + repeated 10 times):  
"REPEAT\_SIZE 3"  
"STRING this command block of 3 commands is repeated"  
"ENTER"  
"DELAY 300"  
"REPEAT 10"  

8. Pre-configuration commands (if present) must be placed separately  
in the config.txt file and come as one contiguous block of up to 5  
commands (no other types of commands are allowed in config.txt)  

9. With "VID **x**", "PID **x**" commands user can replace default  
VID / PID values used for enumeration. (eg. "VID 0x046D", "PID 0xC228").  

10. If "HID\_ONLY\_MODE" command is present, device will either enumerate  
in HID-only mode (keyboard) if MSD-only button is released, or  
in MSD-only mode (flash disk) if MSD-only button is pressed.  
If such command is not present, device enumerates in HID+MSD mode,  
but will not type any keys if MSD-only button is pressed.  

11. "USE_FINGERPRINTER" command enables target OS detection.  
For more information check this [wiki page](https://github.com/krakrukra/PocketAdmin/wiki/fingerprinter)  

12. "USE_LAYOUT **s**" command replaces the default  
ASCII-to-HIDkeycode mapping with a new one.  
For more information check this [wiki page](https://github.com/krakrukra/PocketAdmin/wiki/layouts)  
