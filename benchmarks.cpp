//
// Created by nbdy on 02.09.21.
//

#include "FunctionTimer/FunctionTimer.h"
#include "test_common.h"


template<typename FileType>
void benchmark_read(FileType* i_pFile, uint32_t i_u32Count, std::vector<TestBinaryEntryContainer> entries) {
  auto t = *i_pFile;
  int ec = 0;
  std::vector<TestBinaryEntryContainer> allContainers;
  FunctionTimer ft1(
      [&t, &allContainers]() { EXPECT_TRUE(t.getAllEntries(allContainers)); });
  std::cout << i_u32Count << " getAllEntries: " << ft1.getExecutionTimeMs() << "ms"
            << std::endl;

  for (const auto &container : allContainers) {
    EXPECT_EQ(container.checksum, TestBinaryEntryContainer(entries[ec]).checksum);
    ec++;
  }
}

void testSingleInsert(uint32_t i_u32Count) {
  auto t = getRandomTestFile();
  std::vector<TestBinaryEntryContainer> entries;

  FunctionTimer ft([&t, &entries, i_u32Count]() {
    for (uint32_t i = 0; i < i_u32Count; i++) {
      auto container = generateRandomTestEntryContainer();
      entries.emplace_back(container);
      EXPECT_EQ(t.append(container.entry),
                binfmt::ErrorCode::OK);
    }
  });

  std::cout << "Single insert of " << i_u32Count << " items took " << ft.getExecutionTimeMs() << "ms" << std::endl;
  EXPECT_EQ(t.getFileSize(), i_u32Count * sizeof(TestBinaryEntryContainer) + sizeof(TestBinaryHeader));
  std::cout << "Size: " << t.getFileSize() << std::endl;

  benchmark_read(&t, i_u32Count, entries);

  cleanupTestFile(t);
}

void testVectorInsert(uint32_t i_u32Count) {
  auto t = getRandomTestFile();
  std::vector<TestBinaryEntryContainer> entries;
  for (uint32_t i = 0; i < i_u32Count; i++) {
    entries.push_back(generateRandomTestEntryContainer());
  }

  FunctionTimer ft([&t, &entries]() {
      EXPECT_EQ(t.append(entries), binfmt::ErrorCode::OK);
  });

  std::cout << "Vector insert of " << i_u32Count << " items took " << ft.getExecutionTimeMs() << "ms" << std::endl;
  EXPECT_EQ(t.getFileSize(), i_u32Count * sizeof(TestBinaryEntryContainer) + sizeof(TestBinaryHeader));
  std::cout << "Size: " << t.getFileSize() << std::endl;

  benchmark_read(&t, i_u32Count, entries);

  cleanupTestFile(t);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, test1kSingleInsert) {
    testSingleInsert(1000);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, test10kSingleInsert) {
    testSingleInsert(10000);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, test100kSingleInsert) {
  testSingleInsert(100000);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, test1kVectorInsert) {
    testVectorInsert(1000);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, test10kVectorInsert) {
    testVectorInsert(10000);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, test100kVectorInsert) {
    testVectorInsert(100000);
}

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(BinaryFile, test1MVectorInsert) {
  testVectorInsert(1000000);
}