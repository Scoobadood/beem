/**
 * Handle mapping from QtKeyEvents to BBC Keys and
 * Shift status.
 */
#ifndef BEEB_KEY_MAPPER_H
#define BEEB_KEY_MAPPER_H

#include <QKeyCombination>

bool map_key_combination( const QKeyCombination & combo, uint8_t & bbc_key, bool & shift_pressed );

#endif // BEEB_KEY_MAPPER_H
