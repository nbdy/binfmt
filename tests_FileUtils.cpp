//
// Created by nbdy on 12.10.21.
//

#include "test_common.h"

#include "FileUtils.h"

// Tests
// - CreateDirectory
// - DeleteDirectory
TEST(FileUtils, testCreateAndDeleteDirectory) {
  EXPECT_FALSE(Fs::exists(TEST_DIRECTORY));
  EXPECT_TRUE(FileUtils::CreateDirectory(TEST_DIRECTORY));
  EXPECT_TRUE(Fs::exists(TEST_DIRECTORY));
  EXPECT_TRUE(FileUtils::DeleteDirectory(TEST_DIRECTORY));
  EXPECT_FALSE(Fs::exists(TEST_DIRECTORY));
}

// Tests
// - OpenBinaryFile
// - CloseBinaryFile
TEST(FileUtils, testOpenAndCloseBinaryFile) {
  if(Fs::exists(TEST_BINARY_FILE)){
    Fs::remove(TEST_BINARY_FILE);
  }
  EXPECT_EQ(FileUtils::OpenBinaryFile(TEST_BINARY_FILE), nullptr);
  auto *fp = FileUtils::OpenBinaryFile(TEST_BINARY_FILE, true);
  EXPECT_NE(fp, nullptr);
  EXPECT_TRUE(FileUtils::CloseBinaryFile(fp));
  EXPECT_TRUE(Fs::exists(TEST_BINARY_FILE));
  cleanup(TEST_BINARY_FILE);
}

// Tests
// - WriteHeader
// - GetHeader
TEST(FileUtils, testWriteHeader) {
  EXPECT_FALSE(Fs::exists(TEST_BINARY_FILE));
  TestBinaryFileHeader hdr;
  EXPECT_TRUE(FileUtils::WriteHeader(TEST_BINARY_FILE, hdr));
  EXPECT_TRUE(Fs::exists(TEST_BINARY_FILE));
  TestBinaryFileHeader tmp;
  EXPECT_TRUE(FileUtils::GetHeader(TEST_BINARY_FILE, tmp));
  EXPECT_EQ(hdr.version, tmp.version);
  cleanup(TEST_BINARY_FILE);
}

TEST(FileUtils, testWriteNotExistentFile) {
  EXPECT_FALSE(Fs::exists(TEST_BINARY_FILE));
  TestBinaryEntryContainer container;
  auto r = FileUtils::WriteData<TestBinaryFileHeader, TestBinaryEntryContainer>(TEST_BINARY_FILE, container);
  EXPECT_FALSE(r);
}

TEST(FileUtils, testOpenInNonExistentDirectoryNoCreate) {
  EXPECT_FALSE(Fs::exists(TEST_BINARY_FILE));
  auto r = FileUtils::OpenBinaryFile(TEST_BINARY_FILE_IN_NON_EXISTENT_DIRECTORY, false);
  EXPECT_EQ(r, nullptr);
}

TEST(FileUtils, testOpenInNonExistentDirectoryCreate) {
  EXPECT_FALSE(Fs::exists(TEST_BINARY_FILE));
  auto r = FileUtils::OpenBinaryFile(TEST_BINARY_FILE_IN_NON_EXISTENT_DIRECTORY, true);
  EXPECT_NE(r, nullptr);
  auto c = FileUtils::CloseBinaryFile(r);
  EXPECT_TRUE(c);
  c = FileUtils::DeleteDirectory(TEST_BINARY_FILE_IN_NON_EXISTENT_DIRECTORY);
  EXPECT_TRUE(c);
}

TEST(FileUtils, testDeleteDirectory) {
  auto r = FileUtils::DeleteDirectory("/tmp/ThisDirectoryProbablyDoesNotExistAndIHopeItDoesNot");
  EXPECT_TRUE(r);
}

TEST(FileUtils, testExistentDirectory) {
  auto r = FileUtils::CreateDirectory("/tmp/");
  EXPECT_TRUE(r);
}

TEST(FileUtils, testDeleteNotExistentFile) {
  auto r = FileUtils::DeleteFile("/tmp/ThisFileProbablyDoesNotExistAndIHopeItDoesNot");
  EXPECT_FALSE(r);
}

// Tests
// - WriteHeader
// - WriteData
// - ReadDataAt
void createReadCompareTwo(const std::string& FilePath) {
  EXPECT_FALSE(Fs::exists(FilePath));
  TestBinaryFileHeader hdr;
  EXPECT_TRUE(FileUtils::WriteHeader(FilePath, hdr));
  EXPECT_TRUE(Fs::exists(FilePath));
  TestBinaryEntryContainer c1(TestBinaryEntry{3, 4, 5.32});
  TestBinaryEntryContainer c2(TestBinaryEntry{6, -2, 9.1});
  EXPECT_TRUE(FileUtils::WriteData<TestBinaryFileHeader>(FilePath, c1, 0));
  EXPECT_TRUE(FileUtils::WriteData<TestBinaryFileHeader>(FilePath, c2, 1));
  TestBinaryEntryContainer r2;
  FileUtils::ReadDataAt<TestBinaryFileHeader, TestBinaryEntryContainer>(FilePath, 1, r2);
  EXPECT_TRUE(r2.isEntryValid());
  EXPECT_EQ(c2.entry.iNumber, r2.entry.iNumber);
  EXPECT_EQ(c2.entry.fNumber, r2.entry.fNumber);
}

TEST(FileUtils, testWriteData) {
  createReadCompareTwo(TEST_BINARY_FILE);
  cleanup(TEST_BINARY_FILE);
}

// Tests
// - GetFileSize
TEST(FileUtils, testGetFileSize) {
  createReadCompareTwo(TEST_BINARY_FILE);
  auto fs1 = FileUtils::GetFileSize<TestBinaryFileHeader, TestBinaryEntryContainer>(2);
  auto fs2 = FileUtils::GetFileSize(TEST_BINARY_FILE);
  EXPECT_EQ(fs1, fs2);
  cleanup(TEST_BINARY_FILE);
}

// Tests
// - RemoveAt
TEST(FileUtils, testRemoveAt) {
  createReadCompareTwo(TEST_BINARY_FILE);
  auto r = FileUtils::RemoveAt<TestBinaryFileHeader, TestBinaryEntryContainer>(TEST_BINARY_FILE, 1);
  EXPECT_TRUE(r);
  auto fs1 = FileUtils::GetFileSize(TEST_BINARY_FILE);
  auto fs2 = FileUtils::GetFileSize<TestBinaryFileHeader, TestBinaryEntryContainer>(1);
  EXPECT_EQ(fs1, fs2);
  cleanup(TEST_BINARY_FILE);
}