//
// Created by nbdy on 14.01.22.
//

#include <gtest/gtest.h>
#include "test_common.h"
#include "FileUtils.h"

// Tests
// - Creation of EntryContainer
// - Checksum generation of contained entry
// - Validity check function
// NOLINTNEXTLINE(cert-err58-cpp)
TEST(Checksum, testGenerate) {
  TestBinaryEntryContainer c(TestBinaryEntry{2});
  EXPECT_GT(c.checksum, 0);
  EXPECT_TRUE(c.isEntryValid());
}

// Tests
// - CreateDirectory
// - DeleteDirectory
// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testCreateAndDeleteDirectory) {
  EXPECT_FALSE(std::filesystem::exists(TEST_DIRECTORY));
  EXPECT_TRUE(binfmt::FileUtils::CreateDirectory(TEST_DIRECTORY));
  EXPECT_TRUE(std::filesystem::exists(TEST_DIRECTORY));
  EXPECT_TRUE(binfmt::FileUtils::DeleteDirectory(TEST_DIRECTORY));
  EXPECT_FALSE(std::filesystem::exists(TEST_DIRECTORY));
}

// Tests
// - OpenBinaryFile
// - CloseBinaryFile
// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testOpenAndCloseBinaryFile) {
  if (std::filesystem::exists(TEST_BINARY_FILE)) {
    std::filesystem::remove(TEST_BINARY_FILE);
  }
  EXPECT_EQ(binfmt::FileUtils::OpenBinaryFile(TEST_BINARY_FILE), nullptr);
  auto *fp = binfmt::FileUtils::OpenBinaryFile(TEST_BINARY_FILE, true);
  EXPECT_NE(fp, nullptr);
  EXPECT_TRUE(binfmt::FileUtils::CloseBinaryFile(fp));
  EXPECT_TRUE(std::filesystem::exists(TEST_BINARY_FILE));
  binfmt::FileUtils::DeleteFile(TEST_BINARY_FILE);
}

// Tests
// - WriteHeader
// - GetHeader
// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testWriteHeader) {
  EXPECT_FALSE(std::filesystem::exists(TEST_BINARY_FILE));
  TestBinaryHeader hdr;
  EXPECT_TRUE(binfmt::FileUtils::WriteHeader(TEST_BINARY_FILE, hdr));
  EXPECT_TRUE(std::filesystem::exists(TEST_BINARY_FILE));
  TestBinaryHeader tmp;
  EXPECT_TRUE(binfmt::FileUtils::GetHeader(TEST_BINARY_FILE, tmp));
  EXPECT_EQ(hdr.version, tmp.version);
  binfmt::FileUtils::DeleteFile(TEST_BINARY_FILE);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testWriteNotExistentFile) {
  EXPECT_FALSE(std::filesystem::exists(TEST_BINARY_FILE));
  TestBinaryEntryContainer container;
  auto r = binfmt::FileUtils::WriteData<TestBinaryHeader,
                                        TestBinaryEntryContainer>(
      TEST_BINARY_FILE, container);
  EXPECT_FALSE(r);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testOpenInNonExistentDirectoryNoCreate) {
  EXPECT_FALSE(std::filesystem::exists(TEST_BINARY_FILE));
  auto r = binfmt::FileUtils::OpenBinaryFile(
      TEST_BINARY_FILE_IN_NON_EXISTENT_DIRECTORY, false);
  EXPECT_EQ(r, nullptr);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testOpenInNonExistentDirectoryCreate) {
  EXPECT_FALSE(std::filesystem::exists(TEST_BINARY_FILE));
  auto r = binfmt::FileUtils::OpenBinaryFile(
      TEST_BINARY_FILE_IN_NON_EXISTENT_DIRECTORY, true);
  EXPECT_NE(r, nullptr);
  auto c = binfmt::FileUtils::CloseBinaryFile(r);
  EXPECT_TRUE(c);
  c = binfmt::FileUtils::DeleteDirectory(
      TEST_BINARY_FILE_IN_NON_EXISTENT_DIRECTORY);
  EXPECT_TRUE(c);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testDeleteDirectory) {
  auto r = binfmt::FileUtils::DeleteDirectory(
      "/tmp/ThisDirectoryProbablyDoesNotExistAndIHopeItDoesNot");
  EXPECT_TRUE(r);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testExistentDirectory) {
  auto r = binfmt::FileUtils::CreateDirectory("/tmp/");
  EXPECT_TRUE(r);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testDeleteNotExistentFile) {
  auto r = binfmt::FileUtils::DeleteFile(
      "/tmp/ThisFileProbablyDoesNotExistAndIHopeItDoesNot");
  EXPECT_FALSE(r);
}

// Tests
// - WriteHeader
// - WriteData
// - ReadDataAt
void createReadCompareTwo(const std::string &FilePath) {
  EXPECT_FALSE(std::filesystem::exists(FilePath));
  TestBinaryHeader hdr;
  EXPECT_TRUE(binfmt::FileUtils::WriteHeader(FilePath, hdr));
  EXPECT_TRUE(std::filesystem::exists(FilePath));
  TestBinaryEntryContainer c1(TestBinaryEntry{4});
  TestBinaryEntryContainer c2(TestBinaryEntry{32});
  EXPECT_TRUE(
      binfmt::FileUtils::WriteData<TestBinaryHeader>(FilePath, c1, 0));
  EXPECT_TRUE(
      binfmt::FileUtils::WriteData<TestBinaryHeader>(FilePath, c2, 1));
  TestBinaryEntryContainer r2;
  binfmt::FileUtils::ReadDataAt<TestBinaryHeader, TestBinaryEntryContainer>(
      FilePath, 1, r2);
  EXPECT_TRUE(r2.isEntryValid());
  EXPECT_EQ(c2.entry.m_u32Number, r2.entry.m_u32Number);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testWriteData) {
  createReadCompareTwo(TEST_BINARY_FILE);
  binfmt::FileUtils::DeleteFile(TEST_BINARY_FILE);
}

// Tests
// - GetFileSize
// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testGetFileSize) {
  createReadCompareTwo(TEST_BINARY_FILE);
  auto fs1 = binfmt::FileUtils::GetFileSize<TestBinaryHeader,
                                            TestBinaryEntryContainer>(2);
  auto fs2 = binfmt::FileUtils::GetFileSize(TEST_BINARY_FILE);
  EXPECT_EQ(fs1, fs2);
  binfmt::FileUtils::DeleteFile(TEST_BINARY_FILE);
}

// Tests
// - RemoveAt
// NOLINTNEXTLINE(cert-err58-cpp)
TEST(FileUtils, testRemoveAt) {
  createReadCompareTwo(TEST_BINARY_FILE);
  auto r = binfmt::FileUtils::RemoveAt<TestBinaryHeader,
                                       TestBinaryEntryContainer>(
      TEST_BINARY_FILE, 1);
  EXPECT_TRUE(r);
  auto fs1 = binfmt::FileUtils::GetFileSize(TEST_BINARY_FILE);
  auto fs2 = binfmt::FileUtils::GetFileSize<TestBinaryHeader,
                                            TestBinaryEntryContainer>(1);
  EXPECT_EQ(fs1, fs2);
  binfmt::FileUtils::DeleteFile(TEST_BINARY_FILE);
}