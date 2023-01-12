//
// Created by Dave Durbin on 2/1/2023.
//

#ifndef BEEB_HW_KEYBOARD_H_
#define BEEB_HW_KEYBOARD_H_

#include "data_connectors.h"

#include <vector>

/*
 * The BBC Micro has 74 keys, and 8 DIP switches. All keys can be read as separate values
 * except that the two SHIFT keys cannot be distinguished from each other, and the BREAK key
 * is hardwired to RESET the machine.
 *
 * Internal keyboard numbers:
 *                                       column
 *        | 0           1      2       3       4       5       6       7       8       9
 *    ----+----------------------------------------------------------------------------------
 *    $00 | SHIFT       CTRL   bit 7   bit 6   bit 5   bit 4   bit 3   bit 2   bit 1   bit 0
 *    $10 | Q           3      4       5       f4      8       f7      -       ^       LEFT
 * r  $20 | f0          W      E       T       7       I       9       0       _       DOWN
 * o  $30 | 1           2      D       R       6       U       O       P       [       UP
 * w  $40 | CAPS LOCK   A      X       F       Y       J       K       @       :       RETURN
 *    $50 | SHIFT LOCK  S      C       G       H       N       L       ;       ]       DELETE
 *    $60 | TAB         Z      SPACE   V       B       M       ,       .       /       COPY
 *    $70 | ESCAPE      f1     f2      f3      f5      f6      f8      f9      \       RIGHT
 *
 * 'bit n' refers to the row of eight hardware DIP switches inside the case at the bottom
 * right of the keyboard:
 *
 * See dips.png
 *
 *  dip   bit      Internal Key Number   Description
 *  ----------------------------------------------------------------------------------
 *  8-6   bit 0-2        9,8,7           Together these bits determine the startup MODE
 *  5     bit 3            6             Set if the SHIFT-BREAK action is reversed with BREAK
 *  4-3   bit 4-5         5,4            Sets disc drive timings (depends on make of drive)
 *  2-1   bit 6-7         3,2            unused
 *
 * The keyboard is scanned by first looping through the columns (see .loopKeyboardColumns
 * below). Each time around the loop we check to see if *any* key in that column is pressed.
 * If not we skip to the next column. If we find a column with a key pressed, then we must
 * check each row in that column to check which specific key is pressed
 * (see .loopKeyboardRows).
 *
 * When scanning the entire keyboard, the first row (including SHIFT and CTRL) are not
 * reported since these keys do not generate interrupts. They can instead be tested
 * individually (X negative on entry).
 *
 * For the keyboard circuit diagram, See Chapter 25.
 *
 * On Entry:
 *       X is the first key to scan from (if positive)
 *       X is the specific key to check (if negative)
 *
 *       Carry set if entry is via an OSBYTE
 *        or clear if via .clearCarryKeyboardScan
 *
 * ***************************************************************************************
 */

const uint8_t KEY_F0 = 0x20;
const uint8_t KEY_F1 = 0x71;
const uint8_t KEY_F2 = 0x72;
const uint8_t KEY_F3 = 0x73;
const uint8_t KEY_F4 = 0x14;
const uint8_t KEY_F5 = 0x74;
const uint8_t KEY_F6 = 0x75;
const uint8_t KEY_F7 = 0x16;
const uint8_t KEY_F8 = 0x76;
const uint8_t KEY_F9 = 0x77;

const uint8_t KEY_DIP_7 = 0x02;
const uint8_t KEY_DIP_6 = 0x03;
const uint8_t KEY_DIP_5 = 0x04;
const uint8_t KEY_DIP_4 = 0x05;
const uint8_t KEY_DIP_3 = 0x06;
const uint8_t KEY_DIP_2 = 0x07;
const uint8_t KEY_DIP_1 = 0x08;
const uint8_t KEY_DIP_0 = 0x09;

const uint8_t KEY_A = 0x41;
const uint8_t KEY_B = 0x64;
const uint8_t KEY_C = 0x52;
const uint8_t KEY_D = 0x32;
const uint8_t KEY_E = 0x22;
const uint8_t KEY_F = 0x43;
const uint8_t KEY_G = 0x53;
const uint8_t KEY_H = 0x54;
const uint8_t KEY_I = 0x25;
const uint8_t KEY_J = 0x45;
const uint8_t KEY_K = 0x46;
const uint8_t KEY_L = 0x56;
const uint8_t KEY_M = 0x65;
const uint8_t KEY_N = 0x55;
const uint8_t KEY_O = 0x36;
const uint8_t KEY_P = 0x37;
const uint8_t KEY_Q = 0x10;
const uint8_t KEY_R = 0x33;
const uint8_t KEY_S = 0x51;
const uint8_t KEY_T = 0x23;
const uint8_t KEY_U = 0x35;
const uint8_t KEY_V = 0x63;
const uint8_t KEY_W = 0x21;
const uint8_t KEY_X = 0x42;
const uint8_t KEY_Y = 0x44;
const uint8_t KEY_Z = 0x61;

const uint8_t KEY_ESC = 0x70;
const uint8_t KEY_SPACE = 0x62;
const uint8_t KEY_TAB = 0x60;
const uint8_t KEY_PLUS = 0x57;
const uint8_t KEY_RT_BRACE = 0x58;
const uint8_t KEY_AT = 0x47;
const uint8_t KEY_COLON = 0x48;
const uint8_t KEY_LT_BRACE = 0x38;
const uint8_t KEY_1 = 0x30;
const uint8_t KEY_2 = 0x31;
const uint8_t KEY_3 = 0x11;
const uint8_t KEY_4 = 0x12;
const uint8_t KEY_5 = 0x13;
const uint8_t KEY_6 = 0x34;
const uint8_t KEY_7 = 0x24;
const uint8_t KEY_8 = 0x15;
const uint8_t KEY_9 = 0x26;
const uint8_t KEY_0 = 0x27;

const uint8_t KEY_POUND = 0x28;
const uint8_t KEY_EQUALS = 0x17;
const uint8_t KEY_TILDE = 0x18;
const uint8_t KEY_SHIFT = 0x00;
const uint8_t KEY_CTL = 0x01;
const uint8_t KEY_COMMA = 0x66;
const uint8_t KEY_PERIOD = 0x67;
const uint8_t KEY_SLASH = 0x68;
const uint8_t KEY_BACK_SLASH = 0x78;
const uint8_t KEY_COPY = 0x69;
const uint8_t KEY_DELETE = 0x59;
const uint8_t KEY_RETURN = 0x49;
const uint8_t KEY_LT_ARROW = 0x19;
const uint8_t KEY_RT_ARROW = 0x79;
const uint8_t KEY_UP_ARROW = 0x39;
const uint8_t KEY_DN_ARROW = 0x29;
const uint8_t KEY_SHIFT_LOCK = 0x50;
const uint8_t KEY_CAPS_LOCK = 0x40;

class Keyboard {
 public:
  Keyboard();

  void tick();

  [[nodiscard]] inline data_subscriber_8_bit_ptr we_src() const { return we_src_; }
  [[nodiscard]] inline data_subscriber_8_bit_ptr data_src() const { return data_src_; }
  [[nodiscard]] inline data_provider_8_bit_ptr provider() const  { return provider_; }

 private:
  void check_we();
  void handle_command(uint8_t key_code);

  data_subscriber_8_bit_ptr we_src_;
  data_subscriber_8_bit_ptr data_src_;
  data_provider_8_bit_ptr provider_;

  // DIP switches. Default open (0)
  uint8_t dips_;
  bool auto_scan_enabled_;
};

#endif // BEEB_HW_KEYBOARD_H_
