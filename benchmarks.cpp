//
// Created by nbdy on 02.09.21.
//

#include <chrono>
#include <gtest/gtest.h>
#include "binfmt.h"

#define TEST_DIRECTORY "/tmp/xxxxxxxxxx__xxxxxxxxxxx__xx"
#define TEST_BINARY_FILE "/tmp/xxxxxxx__xxxxxxxxx.bin"
#define TEST_MAX_ENTRIES 1000000000

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
  TestBinaryFileHeader(): BinaryFileHeaderBase(0x7357, 0x8888) {}
};

struct TestBinaryEntry {
  uint32_t uNumber;
  int32_t  iNumber;
  float    fNumber;
  char     cChar;
};

typedef BinaryEntryContainer<TestBinaryEntry> TestBinaryEntryContainer;
typedef BinaryFile<TestBinaryFileHeader, TestBinaryEntry, TestBinaryEntryContainer, TEST_MAX_ENTRIES> TestBinaryFile;


TestBinaryFile getRandomTestFile() {
  TestBinaryFile r(TEST_BINARY_FILE, TestBinaryFileHeader{});
  EXPECT_TRUE(Fs::exists(r.getFilePath()));
  return r;
}

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

TEST(BinaryFile, test1kInsert) {
  auto t = getRandomTestFile();
  std::vector<TestBinaryEntry> entries;
  for(uint32_t i = 0; i < 1000; i++) {
    entries.push_back(generateRandomTestEntryContainer().entry);
  }
  TIMEIT_START("1kWrite")
  auto r = t.append(entries);
  TIMEIT_END
  TIMEIT_RESULT
  EXPECT_TRUE(r.ok);
  EXPECT_EQ(t.getFileSize(), 1000 * sizeof(TestBinaryEntryContainer) + sizeof(TestBinaryFileHeader));
  cleanupTestFile(t);
}

TEST(BinaryFile, test10kInsert) {
  auto t = getRandomTestFile();
  std::vector<TestBinaryEntry> entries;
  for(uint32_t i = 0; i < 10000; i++) {
    entries.push_back(generateRandomTestEntryContainer().entry);
  }
  TIMEIT_START("10kWrite")
  auto r = t.append(entries);
  TIMEIT_END
  TIMEIT_RESULT
  EXPECT_TRUE(r.ok);
  EXPECT_EQ(t.getFileSize(), 10000 * sizeof(TestBinaryEntryContainer) + sizeof(TestBinaryFileHeader));
  int ec = 0;
  std::vector<TestBinaryEntryContainer> allEntries;
  EXPECT_TRUE(t.getAllEntries(allEntries));
  for(const auto& entry : allEntries) {
    EXPECT_EQ(entry.checksum, TestBinaryEntryContainer(entries[ec]).checksum);
    ec++;
  }
  cleanupTestFile(t);
}

TEST(BinaryFile, test100kInsert) {
  auto t = getRandomTestFile();
  std::vector<TestBinaryEntry> entries;
  for(uint32_t i = 0; i < 100000; i++) {
    entries.push_back(generateRandomTestEntryContainer().entry);
  }
  TIMEIT_START("100kWrite")
  auto r = t.append(entries);
  TIMEIT_END
  TIMEIT_RESULT
  EXPECT_TRUE(r.ok);
  EXPECT_EQ(t.getFileSize(), 100000 * sizeof(TestBinaryEntryContainer) + sizeof(TestBinaryFileHeader));
  cleanupTestFile(t);
}

TEST(BinaryFile, test1MInsert) {
  auto t = getRandomTestFile();
  std::vector<TestBinaryEntry> entries;
  uint32_t insertCount = 10000000;
  std::cout << "Generating file of size " << std::to_string(insertCount * sizeof(TestBinaryEntryContainer) + sizeof(TestBinaryFileHeader)) << std::endl;
  for(uint32_t i = 0; i < insertCount; i++) {
    entries.push_back(generateRandomTestEntryContainer().entry);
  }
  TIMEIT_START("1MWrite")
  auto r = t.append(entries);
  TIMEIT_END
  TIMEIT_RESULT
  EXPECT_TRUE(r.ok);
  EXPECT_EQ(t.getFileSize(), insertCount * sizeof(TestBinaryEntryContainer) + sizeof(TestBinaryFileHeader));
  int ec = 0;
  std::vector<TestBinaryEntryContainer> allEntries;
  TIMEIT_START("1MRead")
  EXPECT_TRUE(t.getAllEntries(allEntries));
  TIMEIT_END
  TIMEIT_RESULT
  for(const auto& entry : allEntries) {
    EXPECT_EQ(entry.checksum, TestBinaryEntryContainer(entries[ec]).checksum);
    ec++;
  }
  //cleanupTestFile(t);
}