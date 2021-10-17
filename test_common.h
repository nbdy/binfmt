//
// Created by nbdy on 13.10.21.
//

#ifndef BINFMT__TEST_COMMON_H_
#define BINFMT__TEST_COMMON_H_

#include <cstdint>
#include <ctime>

#include "gtest/gtest.h"

#include "binfmtv2.h"

#define TEST_DIRECTORY "/tmp/xxxxxxxxxx__xxxxxxxxxxx__xx"
#define TEST_BINARY_FILE "/tmp/xxxxxxx__xxxxxxxxx.bin"
#define TEST_MAX_ENTRIES 1000
#define TEST_BINARY_FILE_IN_NON_EXISTENT_DIRECTORY "/tmp/xxxxxxxxxxxxxxxxxxx/xxxxxxx.bin"

#define TIMESTAMP std::chrono::high_resolution_clock::to_time_t(std::chrono::high_resolution_clock::now())

std::string TIMEIT_NAME = "TIMEIT";
time_t TIMEIT_START_TIMESTAMP = 0;
time_t TIMEIT_END_TIMESTAMP = 0;
time_t TIMEIT_DIFF = 0;

#define TIMEIT_START(name) \
TIMEIT_NAME = name;        \
TIMEIT_START_TIMESTAMP = TIMESTAMP;

#define TIMEIT_END \
TIMEIT_END_TIMESTAMP = TIMESTAMP;

#define TIMEIT_RESULT \
TIMEIT_DIFF = TIMEIT_END_TIMESTAMP - TIMEIT_START_TIMESTAMP; \
std::cout << TIMEIT_NAME << ": " << std::to_string(TIMEIT_DIFF) << " s" << std::endl;

struct TestBinaryFileHeader : public BinaryFileHeaderBase {
  TestBinaryFileHeader(): BinaryFileHeaderBase(1, 42) {}
  explicit TestBinaryFileHeader(char* data): BinaryFileHeaderBase(data) {}
};

struct TestBinaryEntry {
  uint32_t uNumber;
  int32_t  iNumber;
  float    fNumber;
  char     cChar;
};

typedef BinaryEntryContainer<TestBinaryEntry> TestBinaryEntryContainer;
typedef BinaryFile<TestBinaryFileHeader, TestBinaryEntry, TestBinaryEntryContainer, TEST_MAX_ENTRIES> TestBinaryFile;

char generateRandomChar(){
  return 'A' + std::rand() % 24; // NOLINT(cert-msc50-cpp,cppcoreguidelines-narrowing-conversions)
}

int generateRandomInteger() {
  return std::rand() % 10000000; // NOLINT(cert-msc50-cpp)
}

float generateRandomFloat() {
  return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); // NOLINT(cert-msc50-cpp)
}

TestBinaryEntryContainer generateRandomTestEntryContainer() {
  return TestBinaryEntryContainer(TestBinaryEntry {
      static_cast<uint32_t>(generateRandomInteger()),
      generateRandomInteger(),
      generateRandomFloat(),
      generateRandomChar()
  });
}

void cleanupTestFile(TestBinaryFile f) {
  f.deleteFile();
  EXPECT_FALSE(Fs::exists(f.getFilePath()));
}


int appendRandomAmountOfEntries(TestBinaryFile& f, int max = 50) {
  int r = std::rand() % max; // NOLINT(cert-msc50-cpp)
  for(int i = 0; i < r; i++) {
    EXPECT_TRUE(f.append(generateRandomTestEntryContainer()).ok);
  }
  return r;
}

std::vector<TestBinaryEntryContainer> appendRandomAmountOfEntriesV(TestBinaryFile& f, int max = 20) {
  int x = std::rand() % max; // NOLINT(cert-msc50-cpp)
  std::vector<TestBinaryEntryContainer> r;
  for(int i = 0; i < x; i++) {
    auto v = generateRandomTestEntryContainer();
    r.push_back(v);
    EXPECT_TRUE(f.append(v).ok);
  }
  return r;
}

std::vector<TestBinaryEntryContainer> appendExactAmountOfEntriesV(TestBinaryFile& f, uint32_t count = 42) {
  std::vector<TestBinaryEntryContainer> r;
  for(int i = 0; i < count; i++) {
    auto v = generateRandomTestEntryContainer();
    r.push_back(v);
    EXPECT_TRUE(f.append(v).ok);
  }
  return r;
}

TestBinaryEntry generateRandomTestEntry() {
  return TestBinaryEntry {
      static_cast<uint32_t>(generateRandomInteger()),
      generateRandomInteger(),
      generateRandomFloat(),
      generateRandomChar()
  };
}

#endif // BINFMT__TEST_COMMON_H_
