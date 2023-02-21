#include "string_to_keystrokes.h"
#include "keyboard.h"

#include <vector>
#include <map>
#include <string>
#include <spdlog/spdlog-inl.h>

bool shift_and_key(char c, bool &shift, uint8_t &key) {
  using namespace std;
  static map<char, pair<bool, uint8_t>> sk = {
          {'0',{false, KEY_0}},
          {'1',{false, KEY_1}},
          {'2',{false, KEY_2}},
          {'3',{false, KEY_3}},
          {'4',{false, KEY_4}},
          {'5',{false, KEY_5}},
          {'6',{false, KEY_6}},
          {'7',{false, KEY_7}},
          {'8',{false, KEY_8}},
          {'9',{false, KEY_9}},

          {'a',{false, KEY_A}},
          {'A',{true, KEY_A}},
          {'b',{false, KEY_B}},
          {'B',{true, KEY_B}},
          {'c',{false, KEY_C}},
          {'C',{true, KEY_C}},
          {'d',{false, KEY_D}},
          {'D',{true, KEY_D}},
          {'e',{false, KEY_E}},
          {'E',{true, KEY_E}},
          {'f',{false, KEY_F}},
          {'F',{true, KEY_F}},
          {'g',{false, KEY_G}},
          {'G',{true, KEY_G}},
          {'h',{false, KEY_H}},
          {'H',{true, KEY_H}},
          {'i',{false, KEY_I}},
          {'I',{true, KEY_I}},
          {'j',{false, KEY_J}},
          {'J',{true, KEY_J}},
          {'k',{false, KEY_K}},
          {'K',{true, KEY_K}},
          {'l',{false, KEY_L}},
          {'L',{true, KEY_L}},
          {'m',{false, KEY_M}},
          {'M',{true, KEY_M}},
          {'n',{false, KEY_N}},
          {'N',{true, KEY_N}},
          {'o',{false, KEY_O}},
          {'O',{true, KEY_O}},
          {'p',{false, KEY_P}},
          {'P',{true, KEY_P}},
          {'q',{false, KEY_Q}},
          {'Q',{true, KEY_Q}},
          {'r',{false, KEY_R}},
          {'R',{true, KEY_R}},
          {'s',{false, KEY_S}},
          {'S',{true, KEY_S}},
          {'t',{false, KEY_T}},
          {'T',{true, KEY_T}},
          {'u',{false, KEY_U}},
          {'U',{true, KEY_U}},
          {'v',{false, KEY_V}},
          {'V',{true, KEY_V}},
          {'w',{false, KEY_W}},
          {'W',{true, KEY_W}},
          {'x',{false, KEY_X}},
          {'X',{true, KEY_X}},
          {'y',{false, KEY_Y}},
          {'Y',{true, KEY_Y}},
          {'z',{false, KEY_Z}},
          {'Z',{true, KEY_Z}},
          {' ',{false, KEY_SPACE}},
          {'\n',{false, KEY_RETURN}},
          {',',{false, KEY_COMMA}},
          {'.',{false, KEY_PERIOD}},
          {';',{false, KEY_SEMI_COLON}},
          {'+',{true, KEY_SEMI_COLON}},
          {':',{false, KEY_COLON}},
          {'*',{true, KEY_COLON}},
          {'%',{true, KEY_5}},
          {'=',{true, KEY_MINUS}},
          {'"',{true, KEY_2}},
          {'(',{true, KEY_8}},
          {')',{true, KEY_9}},
          {'<',{true, KEY_COMMA}},
          {'>',{true, KEY_PERIOD}},
          {'$',{true, KEY_4}},
          {'/',{false, KEY_SLASH}},
          {'?',{true, KEY_SLASH}},
          {'-',{false, KEY_MINUS}},
          {'&',{true, KEY_6}},
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