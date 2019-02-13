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

//macros for key codes
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

//----------------------------------------------------------------------------------------------------------------------

//Here ASCII encoded characters are mapped to HID class key codes
//here is how: HID key code = 7 least significant bits of Keymap[ASCII_code - 32]
//the Most Significant bit of Keymap[ASCII_code - 32] is set to 1 if SHIFT modifier key has to be pressed, 0 otherwise
//to set SHIFT modifier bit add 128 to HID keycode value
static unsigned char Keymap[95] __attribute__(( section(".rodata,\"a\",%progbits@") )) =
  {
    KB_SPACEBAR,// space
    KB_1 + 128,// !
    KB_APOSTROPHE + 128,// "
    KB_3 + 128,// #
    KB_4 + 128,// $
    KB_5 + 128,// %
    KB_7 + 128,// &
    KB_APOSTROPHE, // '
    KB_9 + 128,// (
    KB_0 + 128,// )
    KB_8 + 128,// *
    KB_EQUALS + 128,// +
    KB_COMMA,// ,
    KB_MINUS,// -
    KB_PERIOD,// .
    KB_SLASH,// /
    KB_0,// 0
    KB_1,// 1
    KB_2,// 2
    KB_3,// 3
    KB_4,// 4
    KB_5,// 5
    KB_6,// 6
    KB_7,// 7
    KB_8,// 8
    KB_9,// 9
    KB_SEMICOLON + 128,// :
    KB_SEMICOLON,// ;
    KB_COMMA + 128,// <
    KB_EQUALS,// =
    KB_PERIOD + 128,// >
    KB_SLASH + 128,// ?
    KB_2 + 128,// @
    KB_a + 128,// A
    KB_b + 128,// B
    KB_c + 128,// C
    KB_d + 128,// D
    KB_e + 128,// E
    KB_f + 128,// F
    KB_g + 128,// G
    KB_h + 128,// H
    KB_i + 128,// I
    KB_j + 128,// J
    KB_k + 128,// K
    KB_l + 128,// L
    KB_m + 128,// M
    KB_n + 128,// N
    KB_o + 128,// O
    KB_p + 128,// P
    KB_q + 128,// Q
    KB_r + 128,// R
    KB_s + 128,// S
    KB_t + 128,// T
    KB_u + 128,// U
    KB_v + 128,// V
    KB_w + 128,// W
    KB_x + 128,// X
    KB_y + 128,// Y
    KB_z + 128,// Z
    KB_startBRACKET,// [
    KB_BACKSLASH,// \ .
    KB_endBRACKET,// ]
    KB_6 + 128,// ^
    KB_MINUS + 128,// _
    KB_GRAVE,// `
    KB_a,// a
    KB_b,// b
    KB_c,// c
    KB_d,// d
    KB_e,// e
    KB_f,// f
    KB_g,// g
    KB_h,// h
    KB_i,// i
    KB_j,// j
    KB_k,// k
    KB_l,// l
    KB_m,// m
    KB_n,// n
    KB_o,// o
    KB_p,// p
    KB_q,// q
    KB_r,// r
    KB_s,// s
    KB_t,// t
    KB_u,// u
    KB_v,// v
    KB_w,// w
    KB_x,// x
    KB_y,// y
    KB_z,// z
    KB_startBRACKET + 128,// {
    KB_BACKSLASH + 128,// |
    KB_endBRACKET + 128,// }
    KB_GRAVE + 128// ~
  };

#endif// HID_RODATA_H
