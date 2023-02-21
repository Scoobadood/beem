//
// Created by Dave Durbin on 21/2/2023.
//

#ifndef BEEB_STRING_TO_KEYSTROKES_H
#define BEEB_STRING_TO_KEYSTROKES_H

#include <vector>
#include <string>

std::vector<uint8_t>
string_to_keystrokes(const std::string &text);

#endif //BEEB_STRING_TO_KEYSTROKES_H
