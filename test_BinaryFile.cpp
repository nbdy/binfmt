//
// Created by nbdy on 14.01.22.
//

#include <gtest/gtest.h>

#include "FileUtils.h"
#include "binfmt.h"
#include "test_common.h"

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(binfmt, initializeNonExistingFile) {
  TestBinaryFile file("/tmp/test.bin", TestBinaryHeader{});
  EXPECT_EQ(file.getErrorCode(), binfmt::ErrorCode::OK);
  EXPECT_TRUE(file.deleteFile());
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(binfmt, initializeExistingFile) {
  system("touch /tmp/test.bin");
  TestBinaryFile file("/tmp/test.bin", TestBinaryHeader{});
  EXPECT_EQ(file.getErrorCode(), binfmt::ErrorCode::OK);
  EXPECT_TRUE(file.deleteFile());
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(binfmt, appendEntry) {
  TestBinaryFile file("/tmp/test.bin", TestBinaryHeader{});
  EXPECT_EQ(file.getErrorCode(), binfmt::ErrorCode::OK);
  EXPECT_EQ(file.append(TestBinaryEntry{1}), binfmt::ErrorCode::OK);
  EXPECT_EQ(file.getOffset(), 1);
  EXPECT_TRUE(file.deleteFile());
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(binfmt, appendContainer) {
  TestBinaryFile file("/tmp/test.bin", TestBinaryHeader{});
  EXPECT_EQ(file.getErrorCode(), binfmt::ErrorCode::OK);
  EXPECT_EQ(file.append(TestBinaryEntryContainer{TestBinaryEntry{1}}), binfmt::ErrorCode::OK);
  EXPECT_EQ(file.getOffset(), 1);
  EXPECT_TRUE(file.deleteFile());
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(binfmt, appendEntries) {
  TestBinaryFile file("/tmp/test.bin", TestBinaryHeader{});
  EXPECT_EQ(file.getErrorCode(), binfmt::ErrorCode::OK);
  EXPECT_EQ(file.append(
      {TestBinaryEntry{1}, TestBinaryEntry{2}, TestBinaryEntry{3}}), binfmt::ErrorCode::OK);
  EXPECT_EQ(file.getOffset(), 3);
  EXPECT_TRUE(file.deleteFile());
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(binfmt, appendContainers) {
  TestBinaryFile file("/tmp/test.bin", TestBinaryHeader{});
  EXPECT_EQ(file.getErrorCode(), binfmt::ErrorCode::OK);
  std::vector<TestBinaryEntryContainer> containers = {TestBinaryEntryContainer{TestBinaryEntry{1}},
                                                       TestBinaryEntryContainer{TestBinaryEntry{2}},
                                                       TestBinaryEntryContainer{TestBinaryEntry{3}}};
  EXPECT_EQ(file.append(containers), binfmt::ErrorCode::OK);
  EXPECT_EQ(file.getOffset(), 3);
  EXPECT_TRUE(file.deleteFile());
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(binfmt, removeEntryAtEnd) {
  TestBinaryFile file("/tmp/test.bin", TestBinaryHeader{});
  EXPECT_EQ(file.getErrorCode(), binfmt::ErrorCode::OK);
  std::vector<TestBinaryEntry> entries = {TestBinaryEntry{1}, TestBinaryEntry{2}, TestBinaryEntry{3}};
  EXPECT_EQ(file.append(entries), binfmt::ErrorCode::OK);
  auto fileSize = file.getFileSize();
  EXPECT_EQ(file.getOffset(), 3);
  EXPECT_TRUE(file.removeEntryAtEnd());
  EXPECT_EQ(file.getOffset(), 2);
  EXPECT_LT(file.getFileSize(), fileSize);
  EXPECT_TRUE(file.deleteFile());
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(binfmt, rollover10Entries) {
  TestBinaryFile file("/tmp/test.bin", TestBinaryHeader(0xABC, 0, 10));
  EXPECT_EQ(file.getErrorCode(), binfmt::ErrorCode::OK);
  // Check if our max entries are 10
  EXPECT_EQ(file.getHeader().maxEntries, 10);
  // Append 10 entries
  std::vector<TestBinaryEntry> entries = {
      TestBinaryEntry{1}, TestBinaryEntry{2}, TestBinaryEntry{3},
      TestBinaryEntry{4}, TestBinaryEntry{5}, TestBinaryEntry{6},
      TestBinaryEntry{7}, TestBinaryEntry{8}, TestBinaryEntry{9},
      TestBinaryEntry{10}};
  EXPECT_EQ(file.append(entries), binfmt::ErrorCode::OK);
  // Check if we have 10 entries
  EXPECT_EQ(file.getHeader().count, 10);
  // the offset should be 0, since we rolled over with the tenth entry
  EXPECT_EQ(file.getOffset(), 0);
  // Append another entry to first position (rollover)
  auto eleventhEntry = TestBinaryEntry{11};
  TestBinaryEntryContainer eleventhEntryContainer{eleventhEntry};
  EXPECT_EQ(file.append(eleventhEntry), binfmt::ErrorCode::OK);
  // Check if our count is 11
  EXPECT_EQ(file.getHeader().count, 11);
  // Our offset should be 1 now
  EXPECT_EQ(file.getOffset(), 1);
  // Check if the contents are equal
  TestBinaryEntryContainer readEleventhEntryContainer{};
  EXPECT_TRUE(file.getEntry(0, readEleventhEntryContainer));
  EXPECT_EQ(readEleventhEntryContainer.checksum,
            eleventhEntryContainer.checksum);
  // Add 10 more entries to trigger a rollover with a list
  std::vector<TestBinaryEntry> entries2 = {
      TestBinaryEntry{12}, TestBinaryEntry{13}, TestBinaryEntry{14},
      TestBinaryEntry{15}, TestBinaryEntry{16}, TestBinaryEntry{17},
      TestBinaryEntry{18}, TestBinaryEntry{19}, TestBinaryEntry{20},
      TestBinaryEntry{21}};
  // We should end up at 1 again, since we will roll over
  EXPECT_EQ(file.append(entries2), binfmt::ErrorCode::OK);
  // The count should be 21
  EXPECT_EQ(file.getHeader().count, 21);
  // Our offset should be 1 again
  EXPECT_EQ(file.getOffset(), 1);
  // Check if the contents are equal
  EXPECT_TRUE(file.deleteFile());
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(binfmt, getEntriesFromTo) {
  TestBinaryFile file("/tmp/test.bin", TestBinaryHeader{});
  EXPECT_EQ(file.getErrorCode(), binfmt::ErrorCode::OK);
  std::vector<TestBinaryEntryContainer> cache {};
  for (uint32_t i = 0; i < 2000; i++) {
    cache.emplace_back(TestBinaryEntry{i});
    EXPECT_EQ(file.append(cache[i]), binfmt::ErrorCode::OK);
  }
  std::vector<TestBinaryEntryContainer> entries;
  EXPECT_TRUE(file.getEntriesFromTo(entries, 0, 1000));
  for (uint32_t i = 0; i < 1000; i++) {
    EXPECT_EQ(entries[i].checksum, cache[i].checksum);
  }
  EXPECT_EQ(entries.size(), 1000);
  EXPECT_TRUE(file.deleteFile());
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(binfmt, getEntriesChunked) {
  TestBinaryFile file("/tmp/test.bin", TestBinaryHeader{});
  EXPECT_EQ(file.getErrorCode(), binfmt::ErrorCode::OK);
  std::vector<TestBinaryEntryContainer> cache;
  for (uint32_t i = 0; i < 2000; i++) {
    cache.emplace_back(TestBinaryEntry{i});
    EXPECT_EQ(file.append(cache[i]), binfmt::ErrorCode::OK);
  }
  std::vector<TestBinaryEntryContainer> entries;
  file.getEntriesChunked(
      [&entries](const std::vector<TestBinaryEntryContainer> &i_Entries) {
        for (auto &e : i_Entries) {
          entries.emplace_back(e);
        }
      },
      0, 1000, 100);

  EXPECT_EQ(entries.size(), 1000);
  for (uint32_t i = 0; i < 1000; i++) {
    EXPECT_EQ(entries[i].checksum, cache[i].checksum);
  }
  EXPECT_EQ(entries.size(), 1000);
  EXPECT_TRUE(file.deleteFile());
}

// ----------------------- BinaryFile tests
// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, testDeleteFile) {
  auto t = getRandomTestFile();
  EXPECT_EQ(t.getHeaderSize(), sizeof(TestBinaryHeader));
  EXPECT_EQ(t.getEntrySize(), sizeof(TestBinaryEntry));
  EXPECT_EQ(t.getContainerSize(), sizeof(TestBinaryEntryContainer));
  EXPECT_EQ(t.getHeader().magic, TestBinaryHeader{}.magic);
  EXPECT_EQ(t.getErrorCode(), binfmt::ErrorCode::OK);
  cleanupTestFile(t);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, testClear) {
  auto t = getRandomTestFile();
  int a = appendRandomAmountOfEntries(t);
  EXPECT_GT(a, 0);
  EXPECT_EQ(t.getEntryCount(), a);
  EXPECT_TRUE(t.clear());
  EXPECT_TRUE(t.isEmpty());
  EXPECT_EQ(t.getEntryCount(), 0);
  cleanupTestFile(t);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, testGetFileSize) {
  auto t = getRandomTestFile();
  EXPECT_EQ(t.getErrorCode(), binfmt::ErrorCode::OK);
  auto a = appendRandomAmountOfEntries(t);
  auto s = binfmt::FileUtils::GetFileSize<TestBinaryHeader,
                                          TestBinaryEntryContainer>(a);
  auto se = binfmt::FileUtils::GetFileSize<TestBinaryHeader,
                                           TestBinaryEntryContainer>(0);
  EXPECT_EQ(t.getFileSize(), s);
  t.clear();
  EXPECT_EQ(t.getFileSize(), se);
  cleanupTestFile(t);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, testGetAllEntries) {
  auto t = getRandomTestFile();
  auto ae = appendRandomAmountOfEntriesV(t);
  std::vector<TestBinaryEntryContainer> entries(ae.size());
  auto ok = t.getAllEntries(entries);
  EXPECT_TRUE(ok);
  for (int i = 0; i < ae.size(); i++) {
    auto v = entries[i];
    EXPECT_TRUE(v.isEntryValid());
    auto o = ae[i];
    EXPECT_TRUE(o.isEntryValid());
    auto eq = v.checksum == o.checksum;
    EXPECT_TRUE(eq);
  }
  EXPECT_EQ(entries.size(), ae.size());
  cleanupTestFile(t);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, testGetEntriesFrom) {
  auto t = getRandomTestFile();
  auto ae = appendExactAmountOfEntriesV(t, 20);
  EXPECT_EQ(ae.size(), 20);
  std::vector<TestBinaryEntryContainer> entries;
  EXPECT_TRUE(t.getEntriesFrom(
      entries, 5, 10)); // here is where the 5 comes from, also we take the next
                        // 10 entries and only the next 10 entries
  EXPECT_EQ(entries.size(), 10); // yep, those are actually just 10 entries
  auto entry = entries.at(6);    // 5 + 6 = 11
  auto o = ae.at(11);            // 11 = 5 + 6
  EXPECT_EQ(entry.checksum, o.checksum); // also their contents are equal
  cleanupTestFile(t);
}

/*
// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, testRemoveEntryAt) {
  auto t = getRandomTestFile();
  auto ae = appendExactAmountOfEntriesV(t, TEST_MAX_ENTRIES);
  EXPECT_EQ(ae.size(), TEST_MAX_ENTRIES);
  EXPECT_TRUE(t.removeEntryAt(42));
  auto ec = t.getEntryCount();
  EXPECT_EQ(ec, TEST_MAX_ENTRIES - 1);
  auto t0 = TestBinaryEntryContainer(TestBinaryEntry{-1337, 13.37});
  auto ok = t.setEntryAt(t0, 50);
  EXPECT_TRUE(ok);
  TestBinaryEntryContainer t1{};
  TestBinaryEntry te1{};
  ok = t.getEntryAt(t1, 50);
  EXPECT_TRUE(ok);
  ok = t.getEntryAt(te1, 50);
  EXPECT_TRUE(ok);
  EXPECT_EQ(t0.checksum, t1.checksum);
  TestBinaryEntryContainer tbec(te1);
  EXPECT_EQ(tbec.checksum, t0.checksum);
  cleanupTestFile(t);
}
*/

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, testNoHeader) {
  static std::string filePath("/tmp/binfmt_no_hdr_test.bin");
  static std::string touchCmd("touch ");
  static std::string cmd(touchCmd + filePath);
  std::ofstream o(filePath, std::ios::binary | std::ios::out);
  o.write(" ", 1);
  o.close();
  TestBinaryFile f(filePath, TestBinaryHeader{});
  EXPECT_EQ(f.getErrorCode(), binfmt::ErrorCode::OK);
  cleanupTestFile(f);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, testAppend) {
  auto t = getRandomTestFile();
  t.append(generateRandomTestEntryContainer());
  t.append(generateRandomTestEntry());
  EXPECT_EQ(t.getEntryCount(), 2);
  std::vector<TestBinaryEntry> entries;
  entries.push_back(generateRandomTestEntry());
  entries.push_back(generateRandomTestEntry());
  t.append(entries);
  EXPECT_EQ(t.getEntryCount(), 4);
  appendExactAmountOfEntriesV(t, 100 - 4);
  EXPECT_EQ(t.getEntryCount(), 100);
  entries.clear();
  entries.push_back(generateRandomTestEntry());
  entries.push_back(generateRandomTestEntry());
  t.append(entries);
  auto tc = t.getEntryCount();
  EXPECT_EQ(tc, 102);
  cleanupTestFile(t);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(EntryLimitedBinaryFile, testRollover) {
  auto t = getRandomTestEntryLimitedFile();
  // Can it actually store the desired amount of entries?
  EXPECT_EQ(t.getHeader().maxEntries, TEST_MAX_ENTRIES);
  // Append some entries
  auto ae = appendExactAmountOfEntriesV(t, t.getHeader().maxEntries - 1);
  // Did the right amount of entries get appended?
  EXPECT_EQ(t.getOffset(), t.getHeader().maxEntries - 1);

  // Append a few more entries
  auto ee = appendExactAmountOfEntriesV(t, 43);
  // Has the file rolled over?
  EXPECT_EQ(t.getOffset(), 42);

  // Are the contents of the appended entries correct?
  std::vector<TestBinaryEntryContainer> entries;
  EXPECT_TRUE(t.getEntriesFrom(entries, 0, 42));
  // Are the contents of the appended entries correct?
  for (uint32_t idx = 1; idx < ee.size(); idx++) {
    EXPECT_EQ(ee[idx].checksum, entries[idx - 1].checksum);
  }

  // Cleanup
  cleanupTestFile(t);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, testReopen) {
  {
    TestBinaryFile t = getRandomTestFile();
    t.append(generateRandomTestEntryContainer());
    EXPECT_EQ(t.getOffset(), 1);
  }

  {
    TestBinaryFile t = getRandomTestFile();
    t.append(generateRandomTestEntryContainer());
    EXPECT_EQ(t.getOffset(), 2);
  }


  getRandomTestFile().deleteFile();
}