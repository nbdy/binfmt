//
// Created by nbdy on 02.09.21.
//

#include "test_common.h"

TestBinaryFile getRandomTestFile() {
  TestBinaryFile r(TEST_BINARY_FILE, TestBinaryFileHeader{});
  EXPECT_TRUE(Fs::exists(r.getFilePath()));
  return r;
}

TestBinaryEntryContainer generateRandomTestEntryContainer() {
  return TestBinaryEntryContainer(TestBinaryEntry {
      generateRandomInteger(),
      generateRandomFloat(),
  });
}

void cleanupTestFile(TestBinaryFile f) {
  f.deleteFile();
  EXPECT_FALSE(Fs::exists(f.getFilePath()));
}

// NOLINTNEXTLINE(cert-err58-cpp)
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

// NOLINTNEXTLINE(cert-err58-cpp)
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

// NOLINTNEXTLINE(cert-err58-cpp)
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

// NOLINTNEXTLINE(cert-err58-cpp)
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