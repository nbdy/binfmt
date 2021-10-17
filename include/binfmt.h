/*
    MIT License

    Copyright (c) 2021 Pascal Eberlein

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

/*
 * @author Pascal Eberlein
 * @date 20.08.2021
 */

#ifndef BINFMT__BINFMT_H_
#define BINFMT__BINFMT_H_

#include <iostream>
#include <string>
#include <vector>
#include <error.h>
#include <cstring>
#include <unistd.h>

#if __GNUC__ >= 8
#include <filesystem>
namespace Fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace Fs = std::experimental::filesystem;
#endif

#if defined(_WIN32)
#define FILE_NAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__) || defined(__MACH__) || \
defined(__linux__) || defined(__ANDROID__)
#define FILE_NAME \
(strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)  // NOLINT(cppcoreguidelines-macro-usage)
#endif

using SystemClock = std::chrono::system_clock;

enum SizeType {
  Byte = 1,
  KiloByte = SizeType::Byte * 1024,
  MegaByte = SizeType::KiloByte * 1024,
  GigaByte = SizeType::MegaByte * 1024
};

struct TimeUtils {
  static uint32_t GetSecondsSinceEpoch() {
    return SystemClock::to_time_t(SystemClock::now());
  }
};

struct Checksum {
  /*!
   * Generate a basic Checksum
   * @param data
   * @param length
   * @return
   */
  static uint32_t Generate(const char* data, uint32_t length) {
    uint32_t r = 0;
    do {
      r += *data++;
    } while (--length);
    return -r;
  }

  static uint32_t Generate(const std::string& data) {
    return Checksum::Generate(data.c_str(), data.size());
  }
};

struct BinaryFileHeaderBase {
  uint32_t version;
  uint32_t magic;

  BinaryFileHeaderBase() = default;
  explicit BinaryFileHeaderBase(uint32_t version, uint32_t magic) : version(version), magic(magic) {}
};

template<typename Entry>
struct BinaryEntryContainer {
  uint32_t checksum = 0;
  Entry entry;

  BinaryEntryContainer() = default;
  explicit BinaryEntryContainer(const Entry& entry) : checksum(Checksum::Generate((const char*) &entry, sizeof(Entry))),
                                                      entry(entry) {}
  bool isEntryValid() {
    return Checksum::Generate((const char*) &entry, sizeof(Entry)) == checksum;
  }
};


template<typename HeaderType, typename EntryType, typename ContainerType, uint32_t EntryCount>
class BF {
  const uint32_t m_uHeaderSize = sizeof(HeaderType);
  const uint32_t m_uContainerSize = sizeof(ContainerType);
  const uint32_t m_uEntrySize = sizeof(EntryType);
  const uint32_t m_uMaxFileSize = m_uHeaderSize + m_uContainerSize * EntryCount;

  std::string m_sFilePath{};
  HeaderType m_Header;

  ErrorCode m_ErrorCode = ErrorCode::HEADER_OK;

  uint32_t m_uCurrentAppendOffset = 0;

 public:
  [[maybe_unused]] explicit BF(std::string FilePath, HeaderType ExpectedHeader)
      : m_sFilePath(std::move(FilePath)) {
    initialize(ExpectedHeader);
  }

  [[maybe_unused]] explicit BF(std::string FilePath) : m_sFilePath(std::move(FilePath)) {
    initialize(HeaderType{});
  }

  void initialize(HeaderType ExpectedHeader) {
    if (FileUtils::GetHeader<HeaderType>(m_sFilePath, m_Header)) {
      if (m_Header.magic != ExpectedHeader.magic) {
        m_ErrorCode = ErrorCode::HEADER_MISMATCH;
      }
    } else {
      FileUtils::WriteHeader(m_sFilePath, ExpectedHeader);
    }
  }

  ErrorCode getErrorCode() { return m_ErrorCode; }

  [[nodiscard]] uint32_t getHeaderSize() const { return m_uHeaderSize; }

  [[nodiscard]] uint32_t getEntrySize() const { return m_uEntrySize; }

  std::string getFilePath() { return m_sFilePath; }

  HeaderType getHeader() { return m_Header; }

  ErrorCode checkHeader() {
    HeaderType tmp;
    if (FileUtils::GetHeader(m_sFilePath, tmp)) {
      m_ErrorCode = tmp.magic == m_Header.magic ? ErrorCode::HEADER_OK : ErrorCode::HEADER_MISMATCH;
    } else {
      m_ErrorCode = ErrorCode::NO_HEADER;
    }
    return m_ErrorCode;
  }

  AppendResult append(ContainerType container) {
    bool rewind = false;

    if (getFileSize() + sizeof(ContainerType) > m_uMaxFileSize) {
      m_uCurrentAppendOffset = 0;
      rewind = true;
    }

    auto ok = FileUtils::WriteData<HeaderType, ContainerType>(m_sFilePath, container, m_uCurrentAppendOffset);
    if (ok) {
      m_uCurrentAppendOffset += 1;
    }

    return AppendResult{rewind, ok, m_uCurrentAppendOffset};
  }

  AppendResult append(EntryType entry) {
    bool rewind = false;

    if (getFileSize() + sizeof(ContainerType) > m_uMaxFileSize) {
      m_uCurrentAppendOffset = 0;
      rewind = true;
    }

    auto
        ok = FileUtils::WriteData<HeaderType, ContainerType>(m_sFilePath, ContainerType(entry), m_uCurrentAppendOffset);
    if (ok) {
      m_uCurrentAppendOffset += 1;
    }

    return AppendResult{rewind, ok, m_uCurrentAppendOffset};
  }

  AppendResult append(std::vector<EntryType> entries) {
    AppendResult r{
        false, true, 0
    };
    bool rewind = false;
    if (getFileSize() + sizeof(ContainerType) * entries.size() > m_uMaxFileSize) {
      r.rewind = true;
      auto sizeLeft = m_uMaxFileSize - getFileSize();
      if (sizeLeft == 0) {
        m_uCurrentAppendOffset = 0;
      } else {
        uint32_t entriesLeft = sizeLeft / sizeof(ContainerType) - 1;
        std::vector<EntryType> tmpEntries(entries.begin(), entries.begin() + entriesLeft);
        entries = std::vector<EntryType>(entries.begin() + entriesLeft + 1, entries.end());
        r = append(tmpEntries);
      }
    }
    std::vector<ContainerType> containers;
    for (const auto& entry: entries) {
      containers.push_back(ContainerType(entry));
    }
    r.ok = FileUtils::WriteDataVector<HeaderType, ContainerType>(m_sFilePath, containers, m_uCurrentAppendOffset);
    if (r.ok) {
      m_uCurrentAppendOffset += containers.size();
    }
    return r;
  }

  bool setEntryAt(EntryType entry, uint32_t position) {
    return FileUtils::WriteData<HeaderType, ContainerType>(m_sFilePath, ContainerType(entry), position);
  }

  bool setEntryAt(ContainerType container, uint32_t position) {
    return FileUtils::WriteData<HeaderType, ContainerType>(m_sFilePath, container, position);
  }

  bool removeEntryAt(uint32_t position) {
    return FileUtils::RemoveAt<HeaderType, ContainerType>(m_sFilePath, position);
  }

  bool getEntryAt(ContainerType& entry, uint32_t position) {
    return FileUtils::ReadDataAt<HeaderType, ContainerType>(m_sFilePath, position, entry);
  }

  bool getEntryAt(EntryType& entry, uint32_t position) {
    ContainerType tmp;
    if (FileUtils::ReadDataAt<HeaderType, ContainerType>(m_sFilePath, position, tmp)) {
      entry = tmp.entry;
      return true;
    }
    return false;
  }

  bool getEntriesFrom(std::vector<ContainerType>& Output, uint32_t position, uint32_t count) {
    return FileUtils::ReadData<HeaderType, ContainerType>(Output, m_sFilePath, position, count);
  }

  bool getAllEntries(std::vector<ContainerType>& Output) {
    return FileUtils::ReadData<HeaderType, ContainerType>(Output, m_sFilePath);
  }

  uint32_t getEntryCount() {
    return FileUtils::GetEntryCount<HeaderType, ContainerType>(m_sFilePath);
  }

  uint32_t getFileSize() { return FileUtils::GetFileSize(m_sFilePath); }

  bool clear() { return FileUtils::ClearFile<HeaderType, ContainerType>(m_sFilePath); }

  bool isEmpty() {
    return getEntryCount() == 0;
  }

  bool deleteFile() {
    return FileUtils::DeleteFile(m_sFilePath);
  }

  uint32_t getContainerSize() {
    return m_uContainerSize;
  }
};

#endif //BINFMT__BINFMT_H_
