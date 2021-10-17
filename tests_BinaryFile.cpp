//
// Created by nbdy on 12.10.21.
//

#include "test_common.h"

// Tests
// - Creation of EntryContainer
// - Checksum generation of contained entry
// - Validity check function
TEST(Checksum, testGenerate) {
  TestBinaryEntryContainer c(TestBinaryEntry{1, 2, 3.14});
  EXPECT_GT(c.checksum, 0);
  EXPECT_TRUE(c.isEntryValid());
}

TEST(BinaryFile, testDeleteFile) {
  TestBinaryFile t(TEST_BINARY_FILE, TestBinaryFileHeader{});
  EXPECT_EQ(t.getHeaderSize(), sizeof(TestBinaryFileHeader));
  EXPECT_EQ(t.getEntrySize(), sizeof(TestBinaryEntry));
  EXPECT_EQ(t.getContainerSize(), sizeof(TestBinaryEntryContainer));
  EXPECT_EQ(t.getCurrentHeader().magic, t.getExpectedHeader().magic);
  EXPECT_EQ(t.getErrorCode(), ErrorCode::HEADER_OK);
  t.deleteFile();
}

TEST(BinaryFile, testReopen) {
  {
    TestBinaryFile t(TEST_BINARY_FILE, TestBinaryFileHeader{});
    EXPECT_EQ(t.getHeaderSize(), sizeof(TestBinaryFileHeader));
    EXPECT_EQ(t.getEntrySize(), sizeof(TestBinaryEntry));
    EXPECT_EQ(t.getContainerSize(), sizeof(TestBinaryEntryContainer));
    EXPECT_EQ(t.getCurrentHeader().magic, t.getExpectedHeader().magic);
    EXPECT_EQ(t.getErrorCode(), ErrorCode::HEADER_OK);
  }
  {
    TestBinaryFile t(TEST_BINARY_FILE, TestBinaryFileHeader{});
    EXPECT_EQ(t.getHeaderSize(), sizeof(TestBinaryFileHeader));
    EXPECT_EQ(t.getEntrySize(), sizeof(TestBinaryEntry));
    EXPECT_EQ(t.getContainerSize(), sizeof(TestBinaryEntryContainer));
    EXPECT_EQ(t.getCurrentHeader().magic, t.getExpectedHeader().magic);
    EXPECT_EQ(t.getErrorCode(), ErrorCode::HEADER_OK);
    t.deleteFile();
  }
}

TEST(BinaryFile, testClear) {
  TestBinaryFile t(TEST_BINARY_FILE, TestBinaryFileHeader{});
  auto a = appendExactAmountOfEntriesV(t, 20);
  EXPECT_GT(a.size(), 0);
  EXPECT_EQ(t.getEntryCount(), a.size());
  EXPECT_TRUE(t.clear());
  EXPECT_TRUE(t.isEmpty());
  EXPECT_EQ(t.getEntryCount(), 0);
  t.deleteFile();
}

TEST(BinaryFile, testGetAllEntries) {
  TestBinaryFile t(TEST_BINARY_FILE, TestBinaryFileHeader{});
  auto ae = appendRandomAmountOfEntriesV(t);
  auto entries = t.getAllEntries();
  EXPECT_FALSE(entries.empty());
  for(int i = 0; i < ae.size(); i++){
    auto v = entries[i];
    EXPECT_TRUE(v.isEntryValid());
    auto o = ae[i];
    EXPECT_TRUE(o.isEntryValid());
    auto eq = v.checksum == o.checksum;
    EXPECT_TRUE(eq);
  }
  EXPECT_EQ(entries.size(), ae.size());
  t.deleteFile();
}

TEST(BinaryFile, testRollover) {
  TestBinaryFile t(TEST_BINARY_FILE, TestBinaryFileHeader{});
  auto ae = appendExactAmountOfEntriesV(t, TEST_MAX_ENTRIES);
  EXPECT_EQ(t.getEntryCount(), TEST_MAX_ENTRIES);
  auto ta = TestBinaryEntryContainer {TestBinaryEntry {4444, -4444, 44.44, '4'}};
  auto ar = t.append(ta);
  EXPECT_TRUE(ar.rewind);
  EXPECT_TRUE(ar.ok);
  EXPECT_EQ(ar.offset, t.getContainerSize() + t.getHeaderSize());
  TestBinaryEntryContainer ta0;
  auto ok = t.getEntryAt(ta0, 0);
  EXPECT_TRUE(ok);
  EXPECT_EQ(ta0.checksum, ta.checksum);
  t.deleteFile();
}

TEST(BinaryFile, testGetEntriesFrom) {
  TestBinaryFile t(TEST_BINARY_FILE, TestBinaryFileHeader{});
  auto ae = appendExactAmountOfEntriesV(t, 20);
  EXPECT_EQ(ae.size(), 20);
  auto entries = t.getEntriesFrom(5, 10);
  EXPECT_FALSE(entries.empty()); // here is where the 5 comes from, also we take the next 10 entries and only the next 10 entries
  EXPECT_EQ(entries.size(), 10); // yep, those are actually just 10 entries
  auto entry = entries.at(6); // 5 + 6 = 11
  auto o = ae.at(11); // 11 = 5 + 6
  EXPECT_EQ(entry.checksum, o.checksum); // also their contents are equal
  t.deleteFile();
}

TEST(BinaryFile, testRemoveEntryAt) {
  TestBinaryFile t(TEST_BINARY_FILE, TestBinaryFileHeader{});
  auto ae = appendExactAmountOfEntriesV(t, TEST_MAX_ENTRIES);
  EXPECT_EQ(ae.size(), TEST_MAX_ENTRIES);
  EXPECT_TRUE(t.removeEntryAt(42));
  auto ec = t.getEntryCount();
  EXPECT_EQ(ec, TEST_MAX_ENTRIES - 1);
  auto t0 = TestBinaryEntryContainer(TestBinaryEntry{1337, -1337, 13.37, 'l'});
  auto ok = t.setEntryAt(t0, 50);
  EXPECT_TRUE(ok);
  TestBinaryEntryContainer t1;
  TestBinaryEntry te1;
  ok = t.getEntryAt(t1, 50);
  EXPECT_TRUE(ok);
  ok = t.getEntryAt(te1, 50);
  EXPECT_TRUE(ok);
  EXPECT_EQ(t0.checksum, t1.checksum);
  TestBinaryEntryContainer tbec(te1);
  EXPECT_EQ(tbec.checksum, t0.checksum);
  t.deleteFile();
}

TEST(TimeUtils, GetSecondsSinceEpoch) {
  EXPECT_GT(TimeUtils::GetSecondsSinceEpoch(), 0);
}

TEST(BinaryFile, testNoHeader) {
  static std::string filePath("/tmp/binfmt_no_hdr_test.bin");
  static std::string touchCmd("touch ");
  static std::string cmd(touchCmd + filePath);
  std::ofstream o(filePath, std::ios::binary | std::ios::out);
  o.write(" ", 1);
  o.close();
  TestBinaryFile f(filePath, TestBinaryFileHeader {});
  EXPECT_EQ(f.getErrorCode(), ErrorCode::HEADER_OK);
}

TEST(BinaryFile, testAppend) {
  TestBinaryFile t(TEST_BINARY_FILE, TestBinaryFileHeader{});
  t.append(generateRandomTestEntryContainer());
  t.append(generateRandomTestEntry());
  EXPECT_EQ(t.getEntryCount(), 2);
  std::vector<TestBinaryEntry> entries;
  entries.push_back(generateRandomTestEntry());
  entries.push_back(generateRandomTestEntry());
  t.append(entries);
  EXPECT_EQ(t.getEntryCount(), 4);
  appendExactAmountOfEntriesV(t, 996);
  EXPECT_EQ(t.getEntryCount(), TEST_MAX_ENTRIES);
  entries.clear();
  entries.push_back(generateRandomTestEntry());
  entries.push_back(generateRandomTestEntry());
  t.append(entries);
  auto tc = t.getEntryCount();
  EXPECT_EQ(tc, TEST_MAX_ENTRIES);
  t.deleteFile();
}

TEST(BinaryFile, benchmark) {
  TIMEIT_START("Constructor")
  TestBinaryFile f(TEST_BINARY_FILE, TestBinaryFileHeader {});
  TIMEIT_END
  TIMEIT_RESULT

  TestBinaryEntry e0 {};
  TIMEIT_START("append")
  f.append(e0);
  TIMEIT_END
  TIMEIT_RESULT

  std::vector
}