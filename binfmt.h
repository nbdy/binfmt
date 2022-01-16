/*
  MIT License

  Copyright (c) 2021 Pascal Eberlein

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to
  deal in the Software without restriction, including without limitation the
  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
  sell copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.
*/

#ifndef BINFMT__BINFMT_H_
#define BINFMT__BINFMT_H_

#define LOCK_FREE

#include <fcntl.h>
#include <unistd.h>

#include <filesystem>
#include <mutex>
#include <utility>

namespace binfmt {
using Path = std::filesystem::path;
using LockGuard = std::lock_guard<std::mutex>;

enum SizeType {
  Byte = 1,
  KiloByte = SizeType::Byte * 1000,
  MegaByte = SizeType::KiloByte * 1000,
  GigaByte = SizeType::MegaByte * 1000
};

#ifndef LOCK_FREE
#define LG(mtx) LockGuard ___lg(mtx)
#endif

struct Checksum {
  /*!
   * Generate a basic Checksum
   * @param data
   * @param length
   * @return
   */
  static uint32_t Generate(const char *data, uint32_t length) {
    uint32_t r = 0;
    do // NOLINT(altera-unroll-loops)
    {
      r += *data++;
    } while (--length != 0U);
    return -r;
  }

  static uint32_t Generate(const std::string &data) {
    return Checksum::Generate(data.c_str(), data.size());
  }
};

struct BinaryFileHeaderBase {
  uint32_t magic{0xBEEF};
  uint32_t version{0x0001};
  uint32_t maxEntries{0};
  uint32_t count = 0;
  uint32_t offset = 0;

  BinaryFileHeaderBase() = default;

  explicit BinaryFileHeaderBase(uint32_t magic, uint32_t version,
                                uint32_t maxEntries)
      : magic(magic), version(version), maxEntries(maxEntries) {}

  BinaryFileHeaderBase(uint32_t magic, uint32_t version, uint32_t count,
                       uint32_t offset, uint32_t maxEntries)
      : magic(magic), version(version), maxEntries(maxEntries), count(count),
        offset(offset) {}

  explicit BinaryFileHeaderBase(char *data) {
    auto *base = reinterpret_cast<BinaryFileHeaderBase *>(data);
    magic = base->magic;
    version = base->version;
    count = base->count;
    offset = base->offset;
    maxEntries = base->maxEntries;
  }

  BinaryFileHeaderBase &
  operator=(char *data) // NOLINT(fuchsia-overloaded-operator)
  {
    auto *other = reinterpret_cast<BinaryFileHeaderBase *>(data);
    magic = other->magic;
    version = other->version;
    count = other->count;
    offset = other->offset;
    maxEntries = other->maxEntries;
    return *this;
  }
};

template <typename EntryType> struct BinaryEntryContainer {
  uint32_t checksum = 0;
  EntryType entry;

  BinaryEntryContainer() = default;

  explicit BinaryEntryContainer(const EntryType &entry)
      : checksum(Checksum::Generate(
            (const char *)&entry,
            sizeof(EntryType))), // NOLINT(google-readability-casting)
        entry(entry) {}

  bool isEntryValid() {
    return Checksum::Generate(
               (const char *)&entry,
               sizeof(EntryType)) == // NOLINT(google-readability-casting)
           checksum;
  }
};

enum class ErrorCode {
  OK = 0,
  NO_HEADER,
  HEADER_MALFORMED,
  MAGIC_MISMATCH,
  FILE_ERROR,
  HEADER_WRITE_ERROR
};

template <typename HeaderType, typename EntryType, typename ContainerType>
class BinaryFile {
  const uint32_t m_u32HeaderSize = sizeof(HeaderType);
  const uint32_t m_u32EntrySize = sizeof(EntryType);
  const uint32_t m_u32ContainerSize = sizeof(ContainerType);

  Path m_Path;
  HeaderType m_ExpectedHeader;
  HeaderType m_CurrentHeader;
#ifndef LOCK_FREE
  std::mutex m_Mutex;
#endif
  int m_Fd = -1;
  ErrorCode m_ErrorCode = ErrorCode::OK;

  bool seekToIndex(uint32_t index) {
    return seekTo(getByteOffsetFromIndex(index));
  }

  bool seekTo(uint32_t offset) {
    bool r = true;
    if (lseek(m_Fd, static_cast<off_t>(offset), SEEK_SET) == -1) {
      m_ErrorCode = ErrorCode::FILE_ERROR;
      r = false;
    }
    return r;
  }

protected:
  uint32_t getByteOffsetFromIndex(uint32_t index) {
    return m_u32HeaderSize + (index * m_u32ContainerSize);
  }

  uint32_t getCurrentByteOffset() {
    return m_u32HeaderSize + (m_u32ContainerSize * m_CurrentHeader.offset);
  }

  void readHeader() {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif

    seekTo(0);

    char header[m_u32HeaderSize];
    int32_t readSize = read(m_Fd, &header, m_u32HeaderSize);

    if (readSize != static_cast<int32_t>(m_u32HeaderSize)) {
      m_ErrorCode =
          (readSize == -1) ? ErrorCode::NO_HEADER : ErrorCode::HEADER_MALFORMED;
    } else {
      m_ErrorCode = ErrorCode::OK;
      m_CurrentHeader = HeaderType(header);
    }
  }

  virtual void onVersionOlder(){};
  virtual void onVersionNewer(){};

  virtual void onMaxEntriesChanged(){};

  void checkHeader() {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif

    if (m_CurrentHeader.magic != m_ExpectedHeader.magic) {
      m_ErrorCode = ErrorCode::MAGIC_MISMATCH;
    }

    if (m_CurrentHeader.version < m_ExpectedHeader.version) {
      onVersionOlder();
    } else if (m_CurrentHeader.version > m_ExpectedHeader.version) {
      onVersionNewer();
    }

    if (m_CurrentHeader.maxEntries != m_ExpectedHeader.maxEntries) {
      onMaxEntriesChanged();
    }
  }

  void writeHeader() {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif

    seekTo(0);

    if (write(m_Fd, &m_ExpectedHeader, m_u32HeaderSize) !=
        static_cast<int32_t>(m_u32HeaderSize)) {
      m_ErrorCode = ErrorCode::HEADER_WRITE_ERROR;
    }

    ftruncate(m_Fd, m_u32HeaderSize);
    fsync(m_Fd);
  }

  void fixHeader() {
    writeHeader();
    readHeader();
    checkHeader();

    if (m_ErrorCode != ErrorCode::OK) {
      std::cout << "Error: " << static_cast<uint32_t>(m_ErrorCode) << std::endl;
      throw std::runtime_error("Could not fix header");
    }
  }

  void initialize() {
    m_Fd = open(m_Path.c_str(), O_RDWR | O_CREAT,
                0644); // NOLINT(hicpp-signed-bitwise)
    if (m_Fd < 0) {
      m_ErrorCode = ErrorCode::NO_HEADER;
      throw std::runtime_error("Could not open file");
    }

    readHeader();
    checkHeader();

    if (m_ErrorCode != ErrorCode::OK) {
      fixHeader();
    }
  }

public:
  BinaryFile(Path i_Path, HeaderType i_Header)
      : m_Path(std::move(i_Path)), m_ExpectedHeader(i_Header) {
    initialize();
  };

  ~BinaryFile() {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif
    close(m_Fd);
  };

  [[nodiscard]] ErrorCode getErrorCode() const { return m_ErrorCode; }

  virtual void beforeAppend(ContainerType /*i_Container*/) {
    if (m_CurrentHeader.maxEntries == 0) {
      return;
    }

    if (m_CurrentHeader.offset == m_CurrentHeader.maxEntries) {
      m_CurrentHeader.offset = 0;
    }
  };

  virtual void beforeAppend(const std::vector<ContainerType>& i_Containers) {
    if (m_CurrentHeader.maxEntries == 0) {
      return;
    }

    if (m_CurrentHeader.offset + i_Containers.size() > m_CurrentHeader.maxEntries) {
      m_CurrentHeader.offset = 0;
    }
  };

  virtual void onAppendSuccess(ContainerType /*i_Container*/) {
    m_CurrentHeader.offset++;
    m_CurrentHeader.count++;
  };

  virtual void onAppendSuccess(const std::vector<ContainerType> &i_Containers) {

  };

  virtual void onAppendFailure(ContainerType i_Container){};
  virtual void onAppendFailure(const std::vector<ContainerType> &i_Containers){};

  bool append(ContainerType i_Container) {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif

    beforeAppend(i_Container);

    seekTo(getCurrentByteOffset());
    bool r = write(m_Fd, &i_Container, m_u32ContainerSize) ==
             static_cast<ssize_t>(m_u32ContainerSize);

    if (r) {
      onAppendSuccess(i_Container);
    } else {
      onAppendFailure(i_Container);
    }

    return r && fsync(m_Fd) == 0;
  }

  bool append(const std::vector<ContainerType>& i_Containers) {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif

    beforeAppend(i_Containers);
    auto expectedReadSize = i_Containers.size() * m_u32ContainerSize;
    seekTo(getCurrentByteOffset());
    bool r = write(m_Fd, &i_Containers[0], expectedReadSize) == expectedReadSize;

    if (r) {
      onAppendSuccess(i_Containers);
    } else {
      onAppendFailure(i_Containers);
    }

    return fsync(m_Fd) == 0 && r;
  }

  bool append(EntryType i_Entry) {
    ContainerType container(i_Entry);
    return append(container);
  }

  bool append(const std::vector<EntryType>& i_Entries) {
    std::vector<ContainerType> containers;
    // NOLINTNEXTLINE(altera-unroll-loops)
    for (auto &entry : i_Entries) {
      containers.template emplace_back(ContainerType(entry));
    }
    return append(containers);
  }

  uint32_t getEntryCount() {
    return (getFileSize() - m_u32HeaderSize) / m_u32ContainerSize;
  }

  std::vector<ContainerType> getEntriesFromTo(uint32_t i_u32Start,
                                              uint32_t i_u32End) {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif

    std::vector<ContainerType> r(i_u32End - i_u32Start);
    seekToIndex(i_u32Start);
    auto expectedReadSize = (i_u32End - i_u32Start) * m_u32ContainerSize;
    if (read(m_Fd, &r[0], expectedReadSize) == expectedReadSize) {
      return r;
    }
    return {};
  }

  bool getEntry(uint32_t i_u32Index, ContainerType &o_Container) {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif
    return seekToIndex(i_u32Index) &&
           read(m_Fd, &o_Container, m_u32ContainerSize) == m_u32ContainerSize;
  }

  bool getEntriesFrom(std::vector<ContainerType> &o_Containers,
                      uint32_t i_u32Index, uint32_t i_u32Count) {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif
    o_Containers.resize(i_u32Count);
    seekToIndex(i_u32Index);
    auto expectedReadCount = i_u32Count * m_u32ContainerSize;
    return read(m_Fd, &o_Containers[0], expectedReadCount) == expectedReadCount;
  }

  void
  getEntriesChunked(std::function<void(std::vector<ContainerType>)> i_Callback,
                    uint32_t i_u32Begin = 0, uint32_t i_u32End = 0,
                    uint32_t i_u32ChunkSize = 100000) {
    std::vector<ContainerType> tmp(i_u32ChunkSize);

    uint32_t entryCount = getEntryCount();

    if (i_u32End == 0) {
      i_u32End = entryCount;
    }

    uint32_t rdCnt = 0;
    // NOLINTNEXTLINE(altera-unroll-loops,altera-id-dependent-backward-branch)
    while (i_u32Begin < i_u32End) {
      if (entryCount < i_u32ChunkSize) {
        rdCnt = entryCount;
      } else {
        rdCnt = i_u32ChunkSize;
      }
      tmp = getEntriesFromTo(i_u32Begin, i_u32Begin + rdCnt);
      i_u32Begin += rdCnt;
      i_Callback(tmp);
    }
  }

  virtual bool removeEntryAtEnd() {
    // we can just remove the entry from the end of the file by truncating the
    // file
    m_CurrentHeader.count--;
    m_CurrentHeader.offset--;
    return ftruncate(m_Fd, getCurrentByteOffset()) == 0 && fsync(m_Fd) == 0;
  }

  uint32_t getOffset() {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif
    return m_CurrentHeader.offset;
  }

  virtual uint32_t getFileSize() {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif
    return m_CurrentHeader.count * m_u32ContainerSize + m_u32HeaderSize;
  }

  bool deleteFile() {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif
    close(m_Fd);
    return std::filesystem::remove(m_Path);
  }

  HeaderType getHeader() {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif
    return m_CurrentHeader;
  }

  Path getPath() {
    return m_Path;
  }

  uint32_t getHeaderSize() {
    return m_u32HeaderSize;
  }

  uint32_t getContainerSize() {
    return m_u32ContainerSize;
  }

  uint32_t getEntrySize() {
    return m_u32EntrySize;
  }

  bool clear() {
    return ftruncate(m_Fd, m_u32HeaderSize) == 0 && fsync(m_Fd) == 0;
  }

  bool isEmpty() {
    return getEntryCount() == 0;
  }

  bool getAllEntries(std::vector<ContainerType> &o_Containers) {
    return getEntriesFrom(o_Containers, 0, getEntryCount());
  }
};

} // namespace binfmt

#endif // BINFMT__BINFMT_H_
