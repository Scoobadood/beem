#include <iostream>

#include <vector>
#include <fstream>

#include "beeb.h"

int main() {
  using namespace std;

  Beeb beeb;

  while (true) {
    beeb.tick();
  }
}
