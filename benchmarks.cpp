//
// Created by nbdy on 02.09.21.
//

#include "FunctionTimer/FunctionTimer.h"
#include "test_common.h"

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, test1kInsert) {
  auto t = getRandomTestFile();
  std::vector<TestBinaryEntry> entries;
  for (uint32_t i = 0; i < 1000; i++) {
    entries.push_back(generateRandomTestEntryContainer().entry);
  }
  FunctionTimer ft(
      [&t, entries]() { EXPECT_EQ(t.append(entries), binfmt::ErrorCode::OK); });
  std::cout << "1k insert: " << ft.getExecutionTimeMs() << "ms" << std::endl;
  EXPECT_EQ(t.getFileSize(),
            1000 * sizeof(TestBinaryEntryContainer) + sizeof(TestBinaryHeader));
  cleanupTestFile(t);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, test10kInsert) {
  auto t = getRandomTestFile();
  std::vector<TestBinaryEntry> entries;
  for (uint32_t i = 0; i < 10000; i++) {
    entries.push_back(generateRandomTestEntryContainer().entry);
  }
  FunctionTimer ft(
      [&t, entries]() { EXPECT_EQ(t.append(entries), binfmt::ErrorCode::OK); });
  std::cout << "10k insert: " << ft.getExecutionTimeMs() << "ms" << std::endl;
  EXPECT_EQ(t.getFileSize(), 10000 * sizeof(TestBinaryEntryContainer) +
                                 sizeof(TestBinaryHeader));
  int ec = 0;
  std::vector<TestBinaryEntryContainer> allEntries;
  EXPECT_TRUE(t.getAllEntries(allEntries));
  for (const auto &entry : allEntries) {
    EXPECT_EQ(entry.checksum, TestBinaryEntryContainer(entries[ec]).checksum);
    ec++;
  }
  cleanupTestFile(t);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, test100kInsert) {
  auto t = getRandomTestFile();
  std::vector<TestBinaryEntry> entries;
  for (uint32_t i = 0; i < 100000; i++) {
    entries.push_back(generateRandomTestEntryContainer().entry);
  }
  FunctionTimer ft(
      [&t, entries]() { EXPECT_EQ(t.append(entries), binfmt::ErrorCode::OK); });
  std::cout << "100k insert: " << ft.getExecutionTimeMs() << "ms" << std::endl;
  EXPECT_EQ(t.getFileSize(), 100000 * sizeof(TestBinaryEntryContainer) +
                                 sizeof(TestBinaryHeader));
  cleanupTestFile(t);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, test1MInsert) {
  auto t = getRandomTestFile();
  std::vector<TestBinaryEntry> entries;
  uint32_t insertCount = 10000000;
  std::cout << "Generating file of size "
            << std::to_string(insertCount * sizeof(TestBinaryEntryContainer) +
                              sizeof(TestBinaryHeader))
            << std::endl;
  for (uint32_t i = 0; i < insertCount; i++) {
    entries.push_back(generateRandomTestEntryContainer().entry);
  }
  FunctionTimer ft(
      [&t, entries]() { EXPECT_EQ(t.append(entries), binfmt::ErrorCode::OK); });
  std::cout << "1M insert: " << ft.getExecutionTimeMs() << "ms" << std::endl;
  EXPECT_EQ(t.getFileSize(), insertCount * sizeof(TestBinaryEntryContainer) +
                                 sizeof(TestBinaryHeader));
  int ec = 0;
  std::vector<TestBinaryEntryContainer> allEntries;
  FunctionTimer ft1(
      [&t, &allEntries]() { EXPECT_TRUE(t.getAllEntries(allEntries)); });
  std::cout << "1M getAllEntries: " << ft1.getExecutionTimeMs() << "ms"
            << std::endl;

  for (const auto &entry : allEntries) {
    EXPECT_EQ(entry.checksum, TestBinaryEntryContainer(entries[ec]).checksum);
    ec++;
  }
  // cleanupTestFile(t);
}