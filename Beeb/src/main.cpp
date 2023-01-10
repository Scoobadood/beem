#include "beeb.h"

int main() {
  using namespace std;

  Beeb beeb;

  beeb.reset();
  while (true) {
    beeb.tick();
  }
}
