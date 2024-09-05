#include "string_to_keystrokes.h"
#include "keyboard.h"

#include <vector>
#include <map>
#include <string>
#include <spdlog/spdlog-inl.h>

bool shift_and_key(char c, bool &shift, uint8_t &key) {
  using namespace std;
  static map<char, pair<bool, uint8_t>> sk = {
          {'1',{false, KEY_1}},
          {'2',{false, KEY_2}},
          {'3',{false, KEY_3}},
          {'4',{false, KEY_4}},
          {'5',{false, KEY_5}},
          {'6',{false, KEY_6}},
          {'7',{false, KEY_7}},
          {'8',{false, KEY_8}},
          {'9',{false, KEY_9}},
          {'0',{false, KEY_0}},
          {'-',{false, KEY_MINUS}},
          {'^',{false, KEY_CARET}},
          {'\\',{false, KEY_BACK_SLASH}},

          {'!',{true, KEY_1}},
          {'"',{true, KEY_2}},
          {'#',{true, KEY_3}},
          {'$',{true, KEY_4}},
          {'%',{true, KEY_5}},
          {'&',{true, KEY_6}},
          {'\'',{true, KEY_7}},
          {'(',{true, KEY_8}},
          {')',{true, KEY_9}},
          {'=',{true, KEY_MINUS}},
          {'~',{true, KEY_CARET}},
          {'|',{true, KEY_BACK_SLASH}},

          {'Q',{false, KEY_Q}},
          {'W',{false, KEY_W}},
          {'E',{false, KEY_E}},
          {'R',{false, KEY_R}},
          {'T',{false, KEY_T}},
          {'Y',{false, KEY_Y}},
          {'U',{false, KEY_U}},
          {'I',{false, KEY_I}},
          {'O',{false, KEY_O}},
          {'P',{false, KEY_P}},
          {'@',{false, KEY_AT}},
          {'[',{false, KEY_LT_BRACKET}},
          {'_',{false, KEY_UNDERSCORE}},


          {'q',{true, KEY_Q}},
          {'w',{true, KEY_W}},
          {'e',{true, KEY_E}},
          {'r',{true, KEY_R}},
          {'t',{true, KEY_T}},
          {'y',{true, KEY_Y}},
          {'u',{true, KEY_U}},
          {'i',{true, KEY_I}},
          {'o',{true, KEY_O}},
          {'p',{true, KEY_P}},
          {'{',{true, KEY_LT_BRACKET}},
          {'\163',{true, KEY_UNDERSCORE}},


          {'A',{false, KEY_A}},
          {'S',{false, KEY_S}},
          {'D',{false, KEY_D}},
          {'F',{false, KEY_F}},
          {'G',{false, KEY_G}},
          {'H',{false, KEY_H}},
          {'J',{false, KEY_J}},
          {'K',{false, KEY_K}},
          {'L',{false, KEY_L}},
          {';',{false, KEY_SEMI_COLON}},
          {':',{false, KEY_COLON}},
          {']',{false, KEY_RT_BRACKET}},


          {'a',{true, KEY_A}},
          {'s',{true, KEY_S}},
          {'d',{true, KEY_D}},
          {'f',{true, KEY_F}},
          {'g',{true, KEY_G}},
          {'h',{true, KEY_H}},
          {'j',{true, KEY_J}},
          {'k',{true, KEY_K}},
          {'l',{true, KEY_L}},
          {'+',{true, KEY_SEMI_COLON}},
          {'*',{true, KEY_COLON}},
          {'}',{true, KEY_RT_BRACKET}},


          {'Z',{false, KEY_Z}},
          {'X',{false, KEY_X}},
          {'C',{false, KEY_C}},
          {'V',{false, KEY_V}},
          {'B',{false, KEY_B}},
          {'N',{false, KEY_N}},
          {'M',{false, KEY_M}},
          {',',{false, KEY_COMMA}},
          {'.',{false, KEY_PERIOD}},
          {'/',{false, KEY_SLASH}},


          {'z',{true, KEY_Z}},
          {'x',{true, KEY_X}},
          {'c',{true, KEY_C}},
          {'v',{true, KEY_V}},
          {'b',{true, KEY_B}},
          {'n',{true, KEY_N}},
          {'m',{true, KEY_M}},
          {'<',{true, KEY_COMMA}},
          {'>',{true, KEY_PERIOD}},
          {'?',{true, KEY_SLASH}},

          {' ',{false, KEY_SPACE}},
          {'\n',{false, KEY_RETURN}}
  };

  auto it = sk.find(c);
  if (it == sk.end()) return false;
  shift = it->second.first;
  key = it->second.second;
  return true;
}

std::vector<uint8_t>
string_to_keystrokes(const std::string &text) {
  using namespace std;

  vector<uint8_t> keys;
  for (auto &c: text) {
    bool shift;
    uint8_t key;
    auto valid = shift_and_key(c, shift, key);
    if (!valid) {
      spdlog::error("Missing mapping for character {} 0x{:02x}", c, c);
    }
    if (shift) keys.push_back(KEY_SHIFT);
    keys.push_back(key);
  }
  return keys;
}