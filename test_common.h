//
// Created by nbdy on 25.09.21.
//

#ifndef BINFMT__TEST_COMMON_H_
#define BINFMT__TEST_COMMON_H_

#include <cstdint>
#include <ctime>
#include <fstream>

#include "gtest/gtest.h"

#include "binfmt.h"

#define TEST_MAX_ENTRIES 2000
#define TEST_DIRECTORY "/tmp/binfmt_test"
#define TEST_BINARY_FILE "/tmp/binfmt_test/test_binary"
#define TEST_BINARY_FILE_IN_NON_EXISTENT_DIRECTORY "/tmp/this_directory_probably_does_not_exist/test_binary"

struct TestBinaryEntry {
  uint32_t m_u32Number;
};

using TestBinaryHeader = binfmt::BinaryFileHeaderBase;
using TestBinaryEntryContainer = binfmt::BinaryEntryContainer<TestBinaryEntry>;
using TestBinaryFile = binfmt::BinaryFile<TestBinaryHeader, TestBinaryEntry, TestBinaryEntryContainer>;

int generateRandomInteger() {
  return std::rand() % 10000000; // NOLINT(cert-msc50-cpp)
}

TestBinaryEntry generateRandomTestEntry() {
  return TestBinaryEntry{
      static_cast<uint32_t>(generateRandomInteger()),
  };
}

TestBinaryEntryContainer generateRandomTestEntryContainer() {
  return TestBinaryEntryContainer(generateRandomTestEntry());
}

int appendRandomAmountOfEntries(TestBinaryFile& f, int max = 20) {
  int r = std::rand() % max; // NOLINT(cert-msc50-cpp)
  for (int i = 0; i < r; i++) {
    EXPECT_EQ(f.append(generateRandomTestEntryContainer()), binfmt::ErrorCode::OK);
  }
  return r;
}

std::vector<TestBinaryEntryContainer>
appendRandomAmountOfEntriesV(TestBinaryFile& f, int max = 20) {
  int x = std::rand() % max; // NOLINT(cert-msc50-cpp)
  std::vector<TestBinaryEntryContainer> r;
  for (int i = 0; i < x; i++) {
    auto v = generateRandomTestEntryContainer();
    r.push_back(v);
    EXPECT_EQ(f.append(v), binfmt::ErrorCode::OK);
  }
  return r;
}

template <typename BinaryFileType>
std::vector<TestBinaryEntryContainer>
appendExactAmountOfEntriesV(BinaryFileType &f, uint32_t count = 42) {
  std::vector<TestBinaryEntryContainer> r;
  for (int i = 0; i < count; i++) {
    auto v = generateRandomTestEntryContainer();
    r.push_back(v);
    EXPECT_EQ(f.append(v), binfmt::ErrorCode::OK);
  }
  return r;
}

TestBinaryFile getRandomTestFile() {
  TestBinaryFile r("/tmp/test.bin", TestBinaryHeader {});
  EXPECT_TRUE(std::filesystem::exists(r.getPath()));
  return r;
}

TestBinaryFile getRandomTestEntryLimitedFile() {
  TestBinaryFile r("/tmp/test.bin", TestBinaryHeader {0, 0, TEST_MAX_ENTRIES});
  EXPECT_TRUE(std::filesystem::exists(r.getPath()));
  return r;
}

template <typename BinaryFileType> void cleanupTestFile(BinaryFileType& f) {
  f.deleteFile();
  EXPECT_FALSE(std::filesystem::exists(f.getPath()));
}

void cleanup(const std::string &FilePath) {
  EXPECT_TRUE(std::filesystem::remove(FilePath));
  EXPECT_FALSE(std::filesystem::exists(FilePath));
}

#endif // BINFMT__TEST_COMMON_H_
