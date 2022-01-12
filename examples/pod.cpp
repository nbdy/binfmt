//
// Created by nbdy on 20.08.21.
//

#include <binfmt.h>
#include <iostream>

struct MyPODHeader : public BinaryFileHeaderBase {
  MyPODHeader(): BinaryFileHeaderBase(420, 69) {}
};

struct MyPODEntry {
  float temperature;
  uint32_t timestamp;
};

constexpr uint32_t MaxEntryCount = 1337;
using MyPODContainer = BinaryEntryContainer<MyPODEntry>;
using MyBinaryFile = BinaryFile<MyPODHeader, MyPODEntry, MyPODContainer, MaxEntryCount>;

int main() {
  MyBinaryFile f("MyFile.bin");

  MyPODEntry e0 {4.2, TimeUtils::GetSecondsSinceEpoch()};
  f.append(e0);

  std::cout << "Appended reading with Temperature '" << std::to_string(e0.temperature) << "' to log file" << std::endl;
  BinaryEntryContainer<MyPODEntry> e1 {};
  f.getEntryAt(e1, 0);

  std::cout << "Timestamps are equal: " << (e0.timestamp == e1.entry.timestamp ? "true" : "false") << std::endl;

  return 0;
}
