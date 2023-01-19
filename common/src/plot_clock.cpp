#include "clock.h"
#include <iostream>

void plot(Clock &c, bool transition_line) {
  using namespace std;
  if( transition_line) {
    cout << (c.changed(CLK_1_MHZ) ? "+--+" : (c.is_high(CLK_1_MHZ) ? "   |" : "|   ")) << "  ";
    cout << (c.changed(CLK_2_MHZ) ? "+--+" : (c.is_high(CLK_2_MHZ) ? "   |" : "|   ")) << "  ";
    cout << (c.changed(CLK_4_MHZ) ? "+--+" : (c.is_high(CLK_4_MHZ) ? "   |" : "|   ")) << "  ";
    cout << (c.changed(CLK_8_MHZ) ? "+--+" : (c.is_high(CLK_8_MHZ) ? "   |" : "|   ")) << "  ";
    cout << (c.changed(CLK_16_MHZ) ? "+--+" : (c.is_high(CLK_16_MHZ) ? "   |" : "|   ")) << "  ";
    cout << endl;
    return;
  }

  cout << (c.is_high(CLK_1_MHZ) ? "   |" : "|   ") << "  ";
  cout << (c.is_high(CLK_2_MHZ) ? "   |" : "|   ") << "  ";
  cout << (c.is_high(CLK_4_MHZ) ? "   |" : "|   ") << "  ";
  cout << (c.is_high(CLK_8_MHZ) ? "   |" : "|   ") << "  ";
  cout << (c.is_high(CLK_16_MHZ) ? "   |" : "|   ") << "  ";
  cout << endl;
}

int main() {
  Clock c;
  std::cout << "  1     2     4     8    16" << std::endl;
  std::cout << "----  ----  ----  ----  ----" << std::endl;
  for (int i = 0; i < 35; ++i) {
    plot(c, false);
    c.tick();
    plot(c, true);
  }
}