a plugin to able to paste the string formatted binary data into the x64dbg. 

you can use 

c style byte array, 
***{0x90, 0x90, 0x90};***

c style shellcode,
***"\x90\x90\x90"***

decimal array
***144, 144, 144*** (can be surrounded with curlies).

or sequence of a decimal numbers.

***144 144 144***

***installation*** 💾
drop the plugin binary into x64dbg's plugin directory. 
use dp32 for 32, dp64 for 64 bit of the debugger.

***usage*** ⌨
just copy the text and paste it using yummyPaste's right-click menu.
you can paste either to the disassembler or the dump window.

![demo](https://user-images.githubusercontent.com/437161/90892729-74278c00-e3c6-11ea-8a5b-5c31bdef2b09.gif)

## wear mask 😷, stay safe
