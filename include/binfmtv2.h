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
 * @date 12.10.2021
 */

#ifndef BINFMT__BINFMTV2_H_
#define BINFMT__BINFMTV2_H_

#include <cstring>
#include <error.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#if __GNUC__ >= 8
#include <filesystem>
        namespace Fs = std::filesystem;
#else
#include <experimental/filesystem>
        namespace Fs = std::experimental::filesystem;
#endif

#if defined(_WIN32)
#define FILE_NAME                                                              \
  (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#elif defined(unix) || defined(__unix) || defined(__unix__) ||                 \
    defined(__APPLE__) || defined(__MACH__) || defined(__linux__) ||           \
    defined(__ANDROID__)
#define FILE_NAME                                                              \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1                         \
                          : __FILE__) // NOLINT(cppcoreguidelines-macro-usage)
#endif

using SystemClock = std::chrono::system_clock;

#define LG(mtx) const std::lock_guard<std::mutex> ___lock(mtx);

enum SizeType {
  Byte = 1,
  KiloByte = SizeType::Byte * 1024,
  MegaByte = SizeType::KiloByte * 1024,
  GigaByte [[maybe_unused]] = SizeType::MegaByte * 1024
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
  static uint32_t Generate(const char *data, uint32_t length) {
    uint32_t r = 0;
    do {
      r += *data++;
    } while (--length);
    return -r;
  }

  static uint32_t Generate(const std::string &data) {
    return Checksum::Generate(data.c_str(), data.size());
  }
};

struct AppendResult {
  bool rewind{};
  bool ok{};
  uint32_t offset{};
};

enum class ErrorCode { NO_ERROR, HEADER_OK = 0, HEADER_MISMATCH, NO_HEADER };

struct BinaryFileHeaderBase;

struct IBinaryHeaderBase {
  virtual bool isEqual(const BinaryFileHeaderBase& other) = 0;
};

struct BinaryFileHeaderBase : public IBinaryHeaderBase {
  uint32_t version;
  uint32_t magic;

  BinaryFileHeaderBase() = default;
  explicit BinaryFileHeaderBase(uint32_t version, uint32_t magic)
      : version(version), magic(magic) {}

  explicit BinaryFileHeaderBase(char* data) {
    auto* base = (BinaryFileHeaderBase*) data;
    version = base->version;
    magic = base->magic;
  }

  bool isEqual(const BinaryFileHeaderBase &other) override {
    return other.magic == magic && other.version == version;
  }

  BinaryFileHeaderBase& operator=(char* data) {
    auto* other = (BinaryFileHeaderBase*) data;
    version = other->version;
    magic = other->magic;
    return *this;
  }
};

template <typename Entry> struct BinaryEntryContainer {
  uint32_t checksum = 0;
  Entry entry;

  BinaryEntryContainer() = default;
  explicit BinaryEntryContainer(const Entry &entry)
      : checksum(Checksum::Generate((const char *)&entry, sizeof(Entry))),
        entry(entry) {}
  bool isEntryValid() {
    return Checksum::Generate((const char *)&entry, sizeof(Entry)) == checksum;
  }
};

template <typename HeaderType, typename EntryType, typename ContainerType,
          uint32_t EntryCount>
class BinaryFile {
  const uint32_t m_uHeaderSize = sizeof(HeaderType);
  const uint32_t m_uContainerSize = sizeof(ContainerType);
  const uint32_t m_uEntrySize = sizeof(EntryType);
  const uint32_t m_uMaxFileSize = m_uHeaderSize + m_uContainerSize * EntryCount;

  std::mutex m_mFileMutex;
  uint32_t m_uCurrentAppendOffset = 0;

  Fs::path m_Path;
  BinaryFileHeaderBase m_ExpectedHeader;
  BinaryFileHeaderBase m_CurrentHeader;

  int m_iFileDescriptor = -1;

  ErrorCode m_ErrorCode = ErrorCode::NO_ERROR;

public:
  BinaryFile(Fs::path i_Path, HeaderType i_Header): m_Path(std::move(i_Path)), m_ExpectedHeader(i_Header) {
    initialize();
  }

  ~BinaryFile() {
    LG(m_mFileMutex)
    close(m_iFileDescriptor);
  }

  void readHeader(bool lock = true) {
    LG(m_mFileMutex)

    if(lseek(m_iFileDescriptor, 0, SEEK_SET) == -1) {
      m_ErrorCode = ErrorCode::NO_HEADER;
      std::cout << " lseek " << strerror(errno) << std::endl;
      return;
    }

    char header[m_uHeaderSize];
    int32_t readSize = read(m_iFileDescriptor, &header, m_uHeaderSize);

    if(readSize != static_cast<int32_t>(m_uHeaderSize)) {
      if(readSize == -1) {
        std::cout << "read error: " << strerror(errno) << std::endl;
      } else {
        std::cout << "read warning: read " << readSize << " of " << m_uHeaderSize << std::endl;
      }
      m_ErrorCode = ErrorCode::NO_HEADER;
    } else {
      m_CurrentHeader = HeaderType(header);
    }

    if(m_ExpectedHeader.isEqual(m_CurrentHeader)) {
      m_ErrorCode = ErrorCode::HEADER_OK;
    } else {
      m_ErrorCode = ErrorCode::HEADER_MISMATCH;
    }
  }

  void writeHeader() {
    LG(m_mFileMutex)
    if(lseek(m_iFileDescriptor, 0, SEEK_SET) == -1) {
      std::cout << " lseek " << strerror(errno) << std::endl;
    }
    if(write(m_iFileDescriptor, &m_ExpectedHeader, m_uHeaderSize) != static_cast<int32_t>(m_uHeaderSize)) {
      std::cout << " write writeSize != m_uHeaderSize" << std::endl;
    } else {
      m_uCurrentAppendOffset += m_uHeaderSize;
    }
    fsync(m_iFileDescriptor);
  }

  void initialize() {
    m_iFileDescriptor = open(m_Path.c_str(), O_CREAT | O_RDWR, S_IRWXU);
    if(m_iFileDescriptor == -1) {
      std::cout << "open error: " << strerror(errno) << std::endl;
    }

    readHeader();

    if(m_ErrorCode != ErrorCode::HEADER_OK) {
      writeHeader();
    }

    readHeader();

    fsync(m_iFileDescriptor);
  }

  bool clear() {
    LG(m_mFileMutex)
    auto r = ftruncate(m_iFileDescriptor, m_uHeaderSize);
    if(r == -1) {
      std::cout << "truncate error: " << strerror(errno) << std::endl;
    }
    return r == 0;
  }

  bool deleteFile() {
    LG(m_mFileMutex)
    close(m_iFileDescriptor);
    return Fs::remove(m_Path);
  }

  AppendResult append(ContainerType container) {
    LG(m_mFileMutex)
    bool rewind = false;

    if (getFileSize() + m_uContainerSize > m_uMaxFileSize) {
      m_uCurrentAppendOffset = m_uHeaderSize;
      rewind = true;
    }

    lseek(m_iFileDescriptor, m_uCurrentAppendOffset, SEEK_SET);
    write(m_iFileDescriptor, &container, m_uContainerSize);
    m_uCurrentAppendOffset += m_uContainerSize;

    std::cout << m_uCurrentAppendOffset << std::endl;

    return AppendResult{rewind, true, m_uCurrentAppendOffset};
  }

  AppendResult append(EntryType entry) {
    LG(m_mFileMutex)
    bool rewind = false;

    if (getFileSize() + m_uContainerSize > m_uMaxFileSize) {
      m_uCurrentAppendOffset = m_uHeaderSize;
      rewind = true;
    }

    ContainerType c(entry);

    lseek(m_iFileDescriptor, m_uCurrentAppendOffset, SEEK_SET);
    write(m_iFileDescriptor, &c, m_uContainerSize);
    m_uCurrentAppendOffset += m_uContainerSize;

    return AppendResult{rewind, true, m_uCurrentAppendOffset};
  }

  std::vector<AppendResult> append(std::vector<EntryType> entries) {
    std::vector<AppendResult> r;
    for(const auto& entry : entries) {
      r.push_back(append(entry));
    }
    return r;
  }

  bool removeEntryAt(uint32_t i_u32Index) {
    auto entryCount = getEntryCount();
    auto nextEntryIndex = i_u32Index + 1;
    auto suffixEntryCount = entryCount - nextEntryIndex;
    auto suffixSize = suffixEntryCount * m_uContainerSize;
    LG(m_mFileMutex)
    lseek(m_iFileDescriptor, m_uHeaderSize + ((i_u32Index + 1) * m_uContainerSize),
          SEEK_SET);

    char end[suffixSize];
    if(read(m_iFileDescriptor, &end, suffixSize) != suffixSize) {
      std::cout << " read readItemCount != suffixSize" << std::endl;
    }
    if(lseek(m_iFileDescriptor, m_uHeaderSize + (i_u32Index * m_uContainerSize), SEEK_SET) == -1) {
      std::cout << " lseek " << strerror(errno) << std::endl;
    }
    if(write(m_iFileDescriptor, &end, suffixSize) != suffixSize) {
      std::cout << " write writeItemCount != suffixSize" << std::endl;
    }
    if(ftruncate(m_iFileDescriptor, (entryCount - 1) * m_uContainerSize + m_uHeaderSize) == -1) {
      std::cout << " truncate " << strerror(errno) << std::endl;
    }
    return true;
  }

  // -----------------------------------------------------------
  // Setters (mutexed / might block)
  // -----------------------------------------------------------

  bool setEntryAt(ContainerType i_Input, uint32_t i_u32Index) {
    LG(m_mFileMutex)
    lseek(m_iFileDescriptor, m_uHeaderSize + (i_u32Index * m_uContainerSize), SEEK_SET);
    return write(m_iFileDescriptor, &i_Input, m_uContainerSize) == m_uContainerSize;
  }

  // -----------------------------------------------------------
  // Getters (unmutexed / instantly returning)
  // -----------------------------------------------------------

  uint32_t getEntryCount() {
    return (getFileSize() - m_uHeaderSize) / m_uContainerSize;
  }

  bool isEmpty() { return getEntryCount() == 0; }

  uint32_t getContainerSize() { return m_uContainerSize; }

  uint32_t getFileSize() { return Fs::file_size(m_Path); }

  BinaryFileHeaderBase getExpectedHeader() { return m_ExpectedHeader; }

  BinaryFileHeaderBase getCurrentHeader() { return m_CurrentHeader; }

  ErrorCode getErrorCode() { return m_ErrorCode; }

  [[nodiscard]] uint32_t getHeaderSize() const { return m_uHeaderSize; }

  [[nodiscard]] uint32_t getEntrySize() const { return m_uEntrySize; }

  std::string getFilePath() { return m_Path; }

  // -----------------------------------------------------------
  // Getters (mutexed, might block)
  // -----------------------------------------------------------

  std::vector<ContainerType> getAllEntries() {
    auto s = getFileSize() - m_uHeaderSize;
    std::vector<ContainerType> r(getEntryCount());
    LG(m_mFileMutex)
    lseek(m_iFileDescriptor, m_uHeaderSize, SEEK_SET);
    auto rs = read(m_iFileDescriptor, &r[0], s);
    if(rs < s) {
      if(rs == -1) {
        std::cout << "read error: " << strerror(errno) << std::endl;
      } else {
        std::cout << "read warn: rs: " << rs << " s " << s << std::endl;
      }
    }
    return r;
  }

  bool getEntryAt(ContainerType &i_Output, uint32_t i_u32Index) {
    LG(m_mFileMutex)
    lseek(m_iFileDescriptor, m_uHeaderSize + (i_u32Index * m_uContainerSize),
          SEEK_SET);
    return read(m_iFileDescriptor, &i_Output, m_uContainerSize) == m_uContainerSize;
  }

  bool getEntryAt(EntryType &i_Output, uint32_t i_u32Index) {
    LG(m_mFileMutex)
    lseek(m_iFileDescriptor, m_uHeaderSize + (i_u32Index * m_uContainerSize), SEEK_SET);
    ContainerType c;
    if(read(m_iFileDescriptor, &c, m_uContainerSize) != m_uContainerSize){
      std::cout << __PRETTY_FUNCTION__ << " read readInputSize != m_uContainerSize" << std::endl;
    }
    i_Output = c.entry;
    return c.isEntryValid();
  }

  std::vector<ContainerType> getEntriesFrom(uint32_t i_u32Index, uint32_t i_u32Count) {
    std::vector<ContainerType> r(i_u32Count);

    LG(m_mFileMutex)
    auto sr = lseek(m_iFileDescriptor, m_uHeaderSize + (i_u32Index * m_uContainerSize), SEEK_SET);
    if(sr == -1) {
      std::cout << "lseek error: " << strerror(errno) << std::endl;
      return r;
    }

    uint32_t byteCount = i_u32Count * m_uContainerSize;
    std::cout << "going to read " << i_u32Count << " entries (" << byteCount << " bytes)" << std::endl;
    int32_t rs = read(m_iFileDescriptor, &r[0], byteCount);
    if(rs != static_cast<int32_t>(i_u32Count)) {
      if(rs == -1) {
        std::cout << "read error: " << strerror(errno) << std::endl;
      } else {
        std::cout << "read warning: rs: " << rs << " s: " << i_u32Count << std::endl;
      }
    }

    return r;
  }
};

#endif // BINFMT__BINFMTV2_H_
