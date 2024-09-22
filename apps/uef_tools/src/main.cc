
#include <iostream>
#include <cstdint>
#include <vector>
#include <spdlog/spdlog.h>
#include <iomanip>
#include <UEF/uef.h>
#include <UEF/tape_data_chunk.h>
#include <UEF/acorn_block.h>

int main(int32_t argc, const char *argv[]) {
  using namespace std;

  if (argc != 2) {
    cerr << "Expected file name." << endl;
    return EXIT_FAILURE;
  }

  auto uef = UefData::FromFile(argv[1]);

  cout << "Version " << resetiosflags << uef.StringVersion() << endl;
  for( auto c : uef.Chunks()) {
    cout << *c;
  }



  auto d = load_tape_data_from_uef(uef);
  for( auto  x: d) {
    cout << x.second->name << " " << x.second->data.size()<< endl;
  }
}