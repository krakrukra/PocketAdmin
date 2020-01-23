#ifndef HID_RODATA_H
#define HID_RODATA_H

//here HID class key codes are mapped to symbolic names

//macros for modifier byte keys
#define MOD_NONE   0b00000000
#define MOD_LCTRL  0b00000001
#define MOD_LSHIFT 0b00000010
#define MOD_LALT   0b00000100
#define MOD_LGUI   0b00001000
#define MOD_RCTRL  0b00010000
#define MOD_RSHIFT 0b00100000
#define MOD_RALT   0b01000000
#define MOD_RGUI   0b10000000

//macros for key codes (for US-compatible layouts)
#define KB_Reserved 0
#define KB_ErrorRollOver 1
#define KB_POSTfail 2
#define KB_ErrorUndefined 3
#define KB_a 4
#define KB_b 5
#define KB_c 6
#define KB_d 7
#define KB_e 8
#define KB_f 9
#define KB_g 10
#define KB_h 11
#define KB_i 12
#define KB_j 13
#define KB_k 14
#define KB_l 15
#define KB_m 16
#define KB_n 17
#define KB_o 18
#define KB_p 19
#define KB_q 20
#define KB_r 21
#define KB_s 22
#define KB_t 23
#define KB_u 24
#define KB_v 25
#define KB_w 26
#define KB_x 27
#define KB_y 28
#define KB_z 29
#define KB_1 30
#define KB_2 31
#define KB_3 32
#define KB_4 33
#define KB_5 34
#define KB_6 35
#define KB_7 36
#define KB_8 37
#define KB_9 38
#define KB_0 39
#define KB_RETURN 40
#define KB_ESCAPE 41
#define KB_BACKSPACE 42
#define KB_TAB 43
#define KB_SPACEBAR 44
#define KB_MINUS 45
#define KB_EQUALS 46
#define KB_startBRACKET 47
#define KB_endBRACKET 48
#define KB_BACKSLASH 49
#define KB_nonUS_HASH 50
#define KB_SEMICOLON 51
#define KB_APOSTROPHE 52
#define KB_GRAVE 53
#define KB_COMMA 54
#define KB_PERIOD 55
#define KB_SLASH 56
#define KB_CAPSLOCK 57
#define KB_F1 58
#define KB_F2 59
#define KB_F3 60
#define KB_F4 61
#define KB_F5 62
#define KB_F6 63
#define KB_F7 64
#define KB_F8 65
#define KB_F9 66
#define KB_F10 67
#define KB_F11 68
#define KB_F12 69
#define KB_PRINTSCREEN 70
#define KB_SCROLLLOCK 71
#define KB_PAUSE 72
#define KB_INSERT 73
#define KB_HOME 74
#define KB_PAGEUP 75
#define KB_DELETE 76
#define KB_END 77
#define KB_PAGEDOWN 78
#define KB_RIGHTARROW 79
#define KB_LEFTARROW 80
#define KB_DOWNARROW 81
#define KB_UPARROW 82
#define KP_NUMLOCK 83
#define KP_SLASH 84
#define KP_ASTERISK 85
#define KP_MINUS 86
#define KP_PLUS 87
#define KP_ENTER 88
#define KP_1 89
#define KP_2 90
#define KP_3 91
#define KP_4 92
#define KP_5 93
#define KP_6 94
#define KP_7 95
#define KP_8 96
#define KP_9 97
#define KP_0 98
#define KP_PERIOD 99
#define KB_nonUS_BACKSLASH 100
#define KB_COMPOSE 101

//macros for mouse
#define MOUSE_IDLE       0b00000000
#define MOUSE_LEFTCLICK  0b00000001
#define MOUSE_RIGHTCLICK 0b00000010
#define MOUSE_MIDCLICK   0b00000100

//----------------------------------------------------------------------------------------------------------------------

//Here ASCII encoded characters are mapped to HID class key codes
//here is how: HID key code = 7 least significant bits of Keymap[ASCII_code - 32]
//the Most Significant bit of Keymap[ASCII_code - 32] is set to 1 if SHIFT modifier key has to be pressed, 0 otherwise
//to set SHIFT modifier add this: | 0x80
unsigned char Keymap[107] =
  {
   /* HID code,// ASCII symbol */
   0x2C,       // space
   0x1E | 0x80,// !
   0x34 | 0x80,// "
   0x20 | 0x80,// #
   0x21 | 0x80,// $
   0x22 | 0x80,// %
   0x24 | 0x80,// &
   0x34,       // '
   0x26 | 0x80,// (
   0x27 | 0x80,// )
   0x25 | 0x80,// *
   0x2E | 0x80,// +
   0x36,       // ,
   0x2D,       // -
   0x37,       // .
   0x38,       // /
   0x27,       // 0
   0x1E,       // 1
   0x1F,       // 2
   0x20,       // 3
   0x21,       // 4
   0x22,       // 5
   0x23,       // 6
   0x24,       // 7
   0x25,       // 8
   0x26,       // 9
   0x33 | 0x80,// :
   0x33,       // ;
   0x36 | 0x80,// <
   0x2E,       // =
   0x37 | 0x80,// >
   0x38 | 0x80,// ?
   0x1F | 0x80,// @
   0x04 | 0x80,// A
   0x05 | 0x80,// B
   0x06 | 0x80,// C
   0x07 | 0x80,// D
   0x08 | 0x80,// E
   0x09 | 0x80,// F
   0x0A | 0x80,// G
   0x0B | 0x80,// H
   0x0C | 0x80,// I
   0x0D | 0x80,// J
   0x0E | 0x80,// K
   0x0F | 0x80,// L
   0x10 | 0x80,// M
   0x11 | 0x80,// N
   0x12 | 0x80,// O
   0x13 | 0x80,// P
   0x14 | 0x80,// Q
   0x15 | 0x80,// R
   0x16 | 0x80,// S
   0x17 | 0x80,// T
   0x18 | 0x80,// U
   0x19 | 0x80,// V
   0x1A | 0x80,// W
   0x1B | 0x80,// X
   0x1C | 0x80,// Y
   0x1D | 0x80,// Z
   0x2F,       // [
   0x31,       // backslash
   0x30,       // ]
   0x23 | 0x80,// ^
   0x2D | 0x80,// _
   0x35,       // `
   0x04,       // a
   0x05,       // b
   0x06,       // c
   0x07,       // d
   0x08,       // e
   0x09,       // f
   0x0A,       // g
   0x0B,       // h
   0x0C,       // i
   0x0D,       // j
   0x0E,       // k
   0x0F,       // l
   0x10,       // m
   0x11,       // n
   0x12,       // o
   0x13,       // p
   0x14,       // q
   0x15,       // r
   0x16,       // s
   0x17,       // t
   0x18,       // u
   0x19,       // v
   0x1A,       // w
   0x1B,       // x
   0x1C,       // y
   0x1D,       // z
   0x2F | 0x80,// {
   0x31 | 0x80,// |
   0x30 | 0x80,// }
   0x35 | 0x80,// ~
   
   //AltGr bits start here at Keymap[95] until the end
   0b00000000,
   0b00000000,
   0b00000000,
   0b00000000,
   0b00000000,
   0b00000000,
   0b00000000,
   0b00000000,
   0b00000000,
   0b00000000,
   0b00000000,
   0b00000000
  };

#endif// HID_RODATA_H
