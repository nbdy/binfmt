//
// Created by nbdy on 25.09.21.
//

#ifndef BINFMT__TEST_COMMON_H_
#define BINFMT__TEST_COMMON_H_

#include <cstdint>
#include <ctime>

#include "gtest/gtest.h"

#include "binfmt.h"

#define TEST_DIRECTORY "/tmp/xxxxxxxxxx__xxxxxxxxxxx__xx"
#define TEST_BINARY_FILE "/tmp/xxxxxxx__xxxxxxxxx.bin"
#define TEST_MAX_ENTRIES 1000000000
#define TEST_BINARY_FILE_IN_NON_EXISTENT_DIRECTORY "/tmp/xxxxxxxxxxxxxxxxxxx/xxxxxxx.bin"

#define TIMESTAMP std::chrono::high_resolution_clock::to_time_t(std::chrono::high_resolution_clock::now())

const char* TIMEIT_NAME = "TIMEIT";
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

struct TestBinaryFileHeader : public binfmt::BinaryFileHeaderBase {
  TestBinaryFileHeader(): BinaryFileHeaderBase(0x7357, 0x8888) {}
};

struct TestBinaryEntry {
  int32_t  iNumber;
  float    fNumber;
};

using TestBinaryEntryContainer = binfmt::BinaryEntryContainer<TestBinaryEntry>;
using TestBinaryFile = binfmt::BinaryFile<TestBinaryFileHeader, TestBinaryEntry, TestBinaryEntryContainer, TEST_MAX_ENTRIES>;


int generateRandomInteger() {
  return std::rand() % 10000000; // NOLINT(cert-msc50-cpp)
}

float generateRandomFloat() {
  return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); // NOLINT(cert-msc50-cpp)
}

#endif //BINFMT__TEST_COMMON_H_
