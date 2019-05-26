If "USE_LAYOUT **s**" pre-configuration command is present, the default  
US keyboard layout is replaced with the new one. You need to have the  
necessary layout file in /kblayout/ directory and specify its filename  
as argument to the command, e.g. "USE_LAYOUT fr_FR". Device then will  
get new layout data from that file. Keep in mind that only underscores  
and alphanumeric characters can be in the name of these files!  
  
Keyboard layout files are used as a mapping of ASCII-printable  
characters (used in payload scripts) into HID keycodes along with  
appropriate modifier keys. The mapping is done by taking an ascii symbol  
value and subtracting 32 from it (to avoid non-printable characters),  
then using resulting number as an index to a byte array of 95 elements.  
This byte array is located at the beginning of the file, so byte 0 of  
the file corresponds to ASCII code 32 (spacebar), byte 1 to ASCII code 33  
(exclamation mark), byte 2 to ASCII 34 (quotation mark) and so on.  
  
These bytes contain corresponding HID keycodes in 7 least significant  
bits, and a SHIFT indicator in the most significant bit. If MSb is  
set, the shift modifier will be applied. As an example, in a US layout  
file, byte 33 (for ASCII "A") would contain 0x84 value, while byte 65  
(for ASCII "a") would contain 0x04 value. Both have the same  
HID keycode 0x04, but for "A" the SHIFT indicator is set.  
  
Sometimes, however, you also need to apply an AltGr modifier key.  
This is what the last 12 bytes are for. In this portion of the file  
the bit array of AltGr indicators is located. That is, in byte 95  
the least significant bit corresponds to ASCII code 32, next more  
significant bit corresponds to ASCII code 33 and so on. For ASCII  
code 40 the least significant bit of the next byte (byte 96) is used,  
and this keeps on going until the end of file.  
example: in french layout you need AltGr to type "#" symbol. ASCII  
code for is is 35, so bit 3 in AltGr array must be set. This  
corresponds to byte 95 of layout file having a value 0x08  
(because all other AltGr bits there should be 0)  
  
    
There are some existing keyboard layout files in this repository  
(in /extra/kblayout/). You can, however, make your own if necessary.  
I suggest copying an existing layout file and then inserting all the  
right values with some hex editor.  
  
For more info on which HID keycodes correspond to which US keys read  
USB HID usage tables document ( /extra/HID_UT1_12.pdf ), pages 52-58.  
The HID keycode is bound to key position, not to the symbol on the key;  
e.g. french keyboards have AZERTY layout, that means french "a" is  
on the same position as US "q", hence they will have the same keycode;  
So, whenever PocketAdmin sees ASCII "a" in the script, it needs to  
send HID keycode for US "q". To do that, go to the layout file, find  
byte 65 (for ASCII "a") and put HID keycode for US "q" in it.  
