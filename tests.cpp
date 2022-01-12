//
// Created by nbdy on 20.08.21.
//

#include <fstream>
#include "test_common.h"


TestBinaryEntry generateRandomTestEntry() {
  return TestBinaryEntry {
      generateRandomInteger(),
      generateRandomFloat(),
  };
}

TestBinaryEntryContainer generateRandomTestEntryContainer() {
  return TestBinaryEntryContainer(generateRandomTestEntry());
}

int appendRandomAmountOfEntries(TestBinaryFile f, int max = 20) {
  int r = std::rand() % max; // NOLINT(cert-msc50-cpp)
  for(int i = 0; i < r; i++) {
    EXPECT_TRUE(f.append(generateRandomTestEntryContainer()).ok);
  }
  return r;
}

std::vector<TestBinaryEntryContainer> appendRandomAmountOfEntriesV(TestBinaryFile f, int max = 20) {
  int x = std::rand() % max; // NOLINT(cert-msc50-cpp)
  std::vector<TestBinaryEntryContainer> r;
  for(int i = 0; i < x; i++) {
    auto v = generateRandomTestEntryContainer();
    r.push_back(v);
    EXPECT_TRUE(f.append(v).ok);
  }
  return r;
}

std::vector<TestBinaryEntryContainer> appendExactAmountOfEntriesV(TestBinaryFile f, uint32_t count = 42) {
  std::vector<TestBinaryEntryContainer> r;
  for(int i = 0; i < count; i++) {
    auto v = generateRandomTestEntryContainer();
    r.push_back(v);
    EXPECT_TRUE(f.append(v).ok);
  }
  return r;
}

TestBinaryFile getRandomTestFile() {
  TestBinaryFile r(TEST_BINARY_FILE, TestBinaryFileHeader{});
  EXPECT_TRUE(Fs::exists(r.getFilePath()));
  return r;
}

void cleanupTestFile(TestBinaryFile f) {
  f.deleteFile();
  EXPECT_FALSE(Fs::exists(f.getFilePath()));
}


void cleanup(const std::string& FilePath) {
  EXPECT_TRUE(Fs::remove(FilePath));
  EXPECT_FALSE(Fs::exists(FilePath));
}

// ---------------------- FileUtils tests

// Tests
// - Creation of EntryContainer
// - Checksum generation of contained entry
// - Validity check function
// NOLINTNEXTLINE(cert-err58-cpp)
TEST(Checksum, testGenerate) {
  TestBinaryEntryContainer c(TestBinaryEntry{2, 3.14});
  EXPECT_GT(c.checksum, 0);
  EXPECT_TRUE(c.isEntryValid());
}

// Tests
// - CreateDirectory
// - DeleteDirectory
// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testCreateAndDeleteDirectory) {
  EXPECT_FALSE(Fs::exists(TEST_DIRECTORY));
  EXPECT_TRUE(binfmt::FileUtils::CreateDirectory(TEST_DIRECTORY));
  EXPECT_TRUE(Fs::exists(TEST_DIRECTORY));
  EXPECT_TRUE(binfmt::FileUtils::DeleteDirectory(TEST_DIRECTORY));
  EXPECT_FALSE(Fs::exists(TEST_DIRECTORY));
}

// Tests
// - OpenBinaryFile
// - CloseBinaryFile
// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testOpenAndCloseBinaryFile) {
  if(Fs::exists(TEST_BINARY_FILE)){
    Fs::remove(TEST_BINARY_FILE);
  }
  EXPECT_EQ(binfmt::FileUtils::OpenBinaryFile(TEST_BINARY_FILE), nullptr);
  auto *fp = binfmt::FileUtils::OpenBinaryFile(TEST_BINARY_FILE, true);
  EXPECT_NE(fp, nullptr);
  EXPECT_TRUE(binfmt::FileUtils::CloseBinaryFile(fp));
  EXPECT_TRUE(Fs::exists(TEST_BINARY_FILE));
  cleanup(TEST_BINARY_FILE);
}

// Tests
// - WriteHeader
// - GetHeader
// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testWriteHeader) {
  EXPECT_FALSE(Fs::exists(TEST_BINARY_FILE));
  TestBinaryFileHeader hdr;
  EXPECT_TRUE(binfmt::FileUtils::WriteHeader(TEST_BINARY_FILE, hdr));
  EXPECT_TRUE(Fs::exists(TEST_BINARY_FILE));
  TestBinaryFileHeader tmp;
  EXPECT_TRUE(binfmt::FileUtils::GetHeader(TEST_BINARY_FILE, tmp));
  EXPECT_EQ(hdr.version, tmp.version);
  cleanup(TEST_BINARY_FILE);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testWriteNotExistentFile) {
  EXPECT_FALSE(Fs::exists(TEST_BINARY_FILE));
  TestBinaryEntryContainer container;
  auto r = binfmt::FileUtils::WriteData<TestBinaryFileHeader, TestBinaryEntryContainer>(TEST_BINARY_FILE, container);
  EXPECT_FALSE(r);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testOpenInNonExistentDirectoryNoCreate) {
  EXPECT_FALSE(Fs::exists(TEST_BINARY_FILE));
  auto r = binfmt::FileUtils::OpenBinaryFile(TEST_BINARY_FILE_IN_NON_EXISTENT_DIRECTORY, false);
  EXPECT_EQ(r, nullptr);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testOpenInNonExistentDirectoryCreate) {
  EXPECT_FALSE(Fs::exists(TEST_BINARY_FILE));
  auto r = binfmt::FileUtils::OpenBinaryFile(TEST_BINARY_FILE_IN_NON_EXISTENT_DIRECTORY, true);
  EXPECT_NE(r, nullptr);
  auto c = binfmt::FileUtils::CloseBinaryFile(r);
  EXPECT_TRUE(c);
  c = binfmt::FileUtils::DeleteDirectory(TEST_BINARY_FILE_IN_NON_EXISTENT_DIRECTORY);
  EXPECT_TRUE(c);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testDeleteDirectory) {
  auto r = binfmt::FileUtils::DeleteDirectory("/tmp/ThisDirectoryProbablyDoesNotExistAndIHopeItDoesNot");
  EXPECT_TRUE(r);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testExistentDirectory) {
  auto r = binfmt::FileUtils::CreateDirectory("/tmp/");
  EXPECT_TRUE(r);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testDeleteNotExistentFile) {
  auto r = binfmt::FileUtils::DeleteFile("/tmp/ThisFileProbablyDoesNotExistAndIHopeItDoesNot");
  EXPECT_FALSE(r);
}

// Tests
// - WriteHeader
// - WriteData
// - ReadDataAt
void createReadCompareTwo(const std::string& FilePath) {
  EXPECT_FALSE(Fs::exists(FilePath));
  TestBinaryFileHeader hdr;
  EXPECT_TRUE(binfmt::FileUtils::WriteHeader(FilePath, hdr));
  EXPECT_TRUE(Fs::exists(FilePath));
  TestBinaryEntryContainer c1(TestBinaryEntry{4, 5.32});
  TestBinaryEntryContainer c2(TestBinaryEntry{-2, 9.1});
  EXPECT_TRUE(binfmt::FileUtils::WriteData<TestBinaryFileHeader>(FilePath, c1, 0));
  EXPECT_TRUE(binfmt::FileUtils::WriteData<TestBinaryFileHeader>(FilePath, c2, 1));
  TestBinaryEntryContainer r2;
  binfmt::FileUtils::ReadDataAt<TestBinaryFileHeader, TestBinaryEntryContainer>(FilePath, 1, r2);
  EXPECT_TRUE(r2.isEntryValid());
  EXPECT_EQ(c2.entry.iNumber, r2.entry.iNumber);
  EXPECT_EQ(c2.entry.fNumber, r2.entry.fNumber);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testWriteData) {
  createReadCompareTwo(TEST_BINARY_FILE);
  cleanup(TEST_BINARY_FILE);
}

// Tests
// - GetFileSize
// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testGetFileSize) {
  createReadCompareTwo(TEST_BINARY_FILE);
  auto fs1 = binfmt::FileUtils::GetFileSize<TestBinaryFileHeader, TestBinaryEntryContainer>(2);
  auto fs2 = binfmt::FileUtils::GetFileSize(TEST_BINARY_FILE);
  EXPECT_EQ(fs1, fs2);
  cleanup(TEST_BINARY_FILE);
}

// Tests
// - RemoveAt
// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testRemoveAt) {
  createReadCompareTwo(TEST_BINARY_FILE);
  auto r = binfmt::FileUtils::RemoveAt<TestBinaryFileHeader, TestBinaryEntryContainer>(TEST_BINARY_FILE, 1);
  EXPECT_TRUE(r);
  auto fs1 = binfmt::FileUtils::GetFileSize(TEST_BINARY_FILE);
  auto fs2 = binfmt::FileUtils::GetFileSize<TestBinaryFileHeader, TestBinaryEntryContainer>(1);
  EXPECT_EQ(fs1, fs2);
  cleanup(TEST_BINARY_FILE);
}

// ----------------------- BinaryFile tests
// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, testDeleteFile) {
  auto t = getRandomTestFile();
  EXPECT_EQ(t.getHeaderSize(), sizeof(TestBinaryFileHeader));
  EXPECT_EQ(t.getEntrySize(), sizeof(TestBinaryEntry));
  EXPECT_EQ(t.getContainerSize(), sizeof(TestBinaryEntryContainer));
  EXPECT_EQ(t.getHeader().magic, TestBinaryFileHeader{}.magic);
  EXPECT_EQ(t.checkHeader(), binfmt::ErrorCode::HEADER_OK);
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
  EXPECT_EQ(t.getErrorCode(), binfmt::ErrorCode::HEADER_OK);
  auto a = appendRandomAmountOfEntries(t);
  auto s = binfmt::FileUtils::GetFileSize<TestBinaryFileHeader, TestBinaryEntryContainer>(a);
  auto se = binfmt::FileUtils::GetFileSize<TestBinaryFileHeader, TestBinaryEntryContainer>(0);
  EXPECT_EQ(t.getFileSize(), s);
  t.clear();
  EXPECT_EQ(t.getFileSize(), se);
  cleanupTestFile(t);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, testGetAllEntries) {
  auto t = getRandomTestFile();
  auto ae = appendRandomAmountOfEntriesV(t);
  std::vector<TestBinaryEntryContainer> entries;
  auto ok = t.getAllEntries(entries);
  EXPECT_TRUE(ok);
  for(int i = 0; i < ae.size(); i++){
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
TEST(BinaryFile, testRollover) {
  auto t = getRandomTestFile();
  auto ae = appendExactAmountOfEntriesV(t, TEST_MAX_ENTRIES);
  EXPECT_EQ(t.getEntryCount(), TEST_MAX_ENTRIES);
  auto ta = TestBinaryEntryContainer {TestBinaryEntry {-4444, 44.44}};
  auto ar = t.append(ta);
  EXPECT_TRUE(ar.rewind);
  EXPECT_TRUE(ar.ok);
  EXPECT_EQ(ar.offset, 1);
  TestBinaryEntryContainer ta0;
  auto ok = t.getEntryAt(ta0, 0);
  EXPECT_TRUE(ok);
  EXPECT_EQ(ta0.checksum, ta.checksum);
  cleanupTestFile(t);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, testGetEntriesFrom) {
  auto t = getRandomTestFile();
  auto ae = appendExactAmountOfEntriesV(t, 20);
  EXPECT_EQ(ae.size(), 20);
  std::vector<TestBinaryEntryContainer> entries;
  EXPECT_TRUE(t.getEntriesFrom(entries, 5, 10)); // here is where the 5 comes from, also we take the next 10 entries and only the next 10 entries
  EXPECT_EQ(entries.size(), 10); // yep, those are actually just 10 entries
  auto entry = entries.at(6); // 5 + 6 = 11
  auto o = ae.at(11); // 11 = 5 + 6
  EXPECT_EQ(entry.checksum, o.checksum); // also their contents are equal
  cleanupTestFile(t);
}

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
  TestBinaryEntryContainer t1 {};
  TestBinaryEntry te1 {};
  ok = t.getEntryAt(t1, 50);
  EXPECT_TRUE(ok);
  ok = t.getEntryAt(te1, 50);
  EXPECT_TRUE(ok);
  EXPECT_EQ(t0.checksum, t1.checksum);
  TestBinaryEntryContainer tbec(te1);
  EXPECT_EQ(tbec.checksum, t0.checksum);
  cleanupTestFile(t);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, testNoHeader) {
  static std::string filePath("/tmp/binfmt_no_hdr_test.bin");
  static std::string touchCmd("touch ");
  static std::string cmd(touchCmd + filePath);
  std::ofstream o(filePath, std::ios::binary | std::ios::out);
  o.write(" ", 1);
  o.close();
  TestBinaryFile f(filePath);
  EXPECT_EQ(f.getErrorCode(), binfmt::ErrorCode::HEADER_MISMATCH);
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
  appendExactAmountOfEntriesV(t, 996);
  EXPECT_EQ(t.getEntryCount(), TEST_MAX_ENTRIES);
  entries.clear();
  entries.push_back(generateRandomTestEntry());
  entries.push_back(generateRandomTestEntry());
  t.append(entries);
  auto tc = t.getEntryCount();
  EXPECT_EQ(tc, TEST_MAX_ENTRIES);
  cleanupTestFile(t);
}