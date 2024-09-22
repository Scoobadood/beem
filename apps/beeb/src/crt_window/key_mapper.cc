#include "key_mapper.h"
#include "keyboard.h"

#include <map>

std::map<Qt::Key, std::pair<uint8_t, bool>>
keymap() {
  static std::map<Qt::Key, std::pair<uint8_t, bool>>
          the_keymap
          {
              {Qt::Key_F10,              {KEY_F0,         false}},
              {Qt::Key_F1,               {KEY_F1,         false}},
              {Qt::Key_F2,               {KEY_F2,         false}},
              {Qt::Key_F3,               {KEY_F3,         false}},
              {Qt::Key_F4,               {KEY_F4,         false}},
              {Qt::Key_F5,               {KEY_F5,         false}},
              {Qt::Key_F6,               {KEY_F6,         false}},
              {Qt::Key_F7,               {KEY_F7,         false}},
              {Qt::Key_F8,               {KEY_F8,         false}},
              {Qt::Key_F9,               {KEY_F9,         false}},
//              {Qt::Key_F11,              {KEY_BREAK,      false}},
              {Qt::Key_Escape,           {KEY_ESC,        false}},
              {Qt::Key_1,                {KEY_1,          false}},
              {Qt::Key_Exclam,           {KEY_1,          true}},
              {Qt::Key_2,                {KEY_2,          false}},
              {Qt::Key_QuoteDbl,         {KEY_2,          true}},
              {Qt::Key_3,                {KEY_3,          false}},
              {Qt::Key_NumberSign,       {KEY_3,          true}},
              {Qt::Key_4,                {KEY_4,          false}},
              {Qt::Key_Dollar,           {KEY_4,          true}},
              {Qt::Key_5,                {KEY_5,          false}},
              {Qt::Key_Percent,          {KEY_5,          true}},
              {Qt::Key_6,                {KEY_6,          false}},
              {Qt::Key_Ampersand,        {KEY_6,          true}},
              {Qt::Key_7,                {KEY_7,          false}},
              {Qt::Key_Apostrophe,       {KEY_7,          true}},
              {Qt::Key_8,                {KEY_8,          false}},
              {Qt::Key_ParenLeft,        {KEY_8,          true}},
              {Qt::Key_9,                {KEY_9,          false}},
              {Qt::Key_ParenRight,       {KEY_9,          true}},
              {Qt::Key_0,                {KEY_0,          false}},
              {Qt::Key_Minus,            {KEY_MINUS,      false}},
              {Qt::Key_Equal,            {KEY_MINUS,      true}},
              {Qt::Key_AsciiCircum,      {KEY_CARET,      false}},
              {Qt::Key_AsciiTilde,       {KEY_CARET,      true}},
              {Qt::Key_Backslash,        {KEY_BACK_SLASH, false}},
              {Qt::Key_Bar,              {KEY_BACK_SLASH, true}},
              {Qt::Key_Left,             {KEY_LT_ARROW,   false}},
              {Qt::Key_Right,            {KEY_RT_ARROW,   false}},
              {Qt::Key_Tab,              {KEY_TAB,        false}},
              {Qt::Key_Q,                {KEY_Q,          false}},
              {Qt::Key_W,                {KEY_W,          false}},
              {Qt::Key_E,                {KEY_E,          false}},
              {Qt::Key_R,                {KEY_R,          false}},
              {Qt::Key_T,                {KEY_T,          false}},
              {Qt::Key_Y,                {KEY_Y,          false}},
              {Qt::Key_U,                {KEY_U,          false}},
              {Qt::Key_I,                {KEY_I,          false}},
              {Qt::Key_O,                {KEY_O,          false}},
              {Qt::Key_P,                {KEY_P,          false}},
              {Qt::Key_At,               {KEY_AT,          false}},
              {Qt::Key_BracketLeft,      {KEY_LT_BRACKET, false}},
              {Qt::Key_BraceLeft,        {KEY_LT_BRACKET, true}},
              {Qt::Key_Underscore,       {KEY_UNDERSCORE, false}},
              {Qt::Key_sterling,       {KEY_UNDERSCORE, true}},
              {Qt::Key_Up,               {KEY_UP_ARROW,   false}},
              {Qt::Key_Down,             {KEY_DN_ARROW,   false}},
              {Qt::Key_CapsLock,         {KEY_CAPS_LOCK,  false}},
              {Qt::Key_Control,          {KEY_CTL,        false}},
              {Qt::Key_A,                {KEY_A,          false}},
              {Qt::Key_S,                {KEY_S,          false}},
              {Qt::Key_D,                {KEY_D,          false}},
              {Qt::Key_F,                {KEY_F,          false}},
              {Qt::Key_G,                {KEY_G,          false}},
              {Qt::Key_H,                {KEY_H,          false}},
              {Qt::Key_J,                {KEY_J,          false}},
              {Qt::Key_K,                {KEY_K,          false}},
              {Qt::Key_L,                {KEY_L,          false}},
              {Qt::Key_Semicolon,        {KEY_SEMI_COLON, false}},
              {Qt::Key_Plus,             {KEY_SEMI_COLON, true}},
              {Qt::Key_Colon,            {KEY_COLON,      false}},
              {Qt::Key_Asterisk,         {KEY_COLON,      true}},
              {Qt::Key_BracketRight,     {KEY_RT_BRACKET, false}},
              {Qt::Key_BraceRight,       {KEY_RT_BRACKET, true}},
              {Qt::Key_Return,           {KEY_RETURN,     false}},
//                  {Qt::Key_ApplicationRight, {KEY_SHIFT_LOCK, false}},
              {Qt::Key_Shift,            {KEY_SHIFT,      false}},
              {Qt::Key_Z,                {KEY_Z,          false}},
              {Qt::Key_X,                {KEY_X,          false}},
              {Qt::Key_C,                {KEY_C,          false}},
              {Qt::Key_V,                {KEY_V,          false}},
              {Qt::Key_B,                {KEY_B,          false}},
              {Qt::Key_N,                {KEY_N,          false}},
              {Qt::Key_M,                {KEY_M,          false}},
              {Qt::Key_Comma,            {KEY_COMMA,      false}},
              {Qt::Key_Less,             {KEY_COMMA,      true}},
              {Qt::Key_Period,           {KEY_PERIOD,     false}},
              {Qt::Key_Greater,          {KEY_PERIOD,     true}},
              {Qt::Key_Slash,            {KEY_SLASH,      false}},
              {Qt::Key_Question,         {KEY_SLASH,      true}},
              {Qt::Key_Backspace,        {KEY_DELETE,     false}},
              {Qt::Key_F12,              {KEY_COPY,      false}},
              {Qt::Key_Space,            {KEY_SPACE,      false}}
          };

  return the_keymap;
}

bool map_key_combination(const QKeyCombination &combo, uint8_t &bbc_key, bool &shift_pressed) {
  auto km = keymap();
  auto it = km.find(combo.key());
  if (it == km.end())
    return false;

  bbc_key = it->second.first;
  shift_pressed = it->second.second;
  return true;
}
