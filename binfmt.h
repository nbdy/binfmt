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
  OPEN_ERROR,
  MAGIC_MISMATCH,
  SEEK_ERROR,
  READ_ERROR,
  WRITE_ERROR,
  SYNC_ERROR,
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
  int32_t m_Fd = -1;
  ErrorCode m_ErrorCode = ErrorCode::OK;
  std::vector<std::string> m_Errors;

protected:
  virtual void onSysCallError() {
    const char* error = strerror(errno);
    if(error != nullptr) {
      m_Errors.emplace_back(std::string(error));
    }
  }

  bool seekTo(uint32_t offset, ErrorCode* o_pErrorCode = nullptr) {
    bool r = lseek(m_Fd, static_cast<off_t>(offset), SEEK_SET) == offset;
    if(!r) {
      onSysCallError();
    }
    if(o_pErrorCode != nullptr) {
      *o_pErrorCode = r ? ErrorCode::OK : ErrorCode::SEEK_ERROR;
    }
    return r;
  }

  bool seekToIndex(uint32_t index, ErrorCode* o_pErrorCode = nullptr) {
    return seekTo(getByteOffsetFromIndex(index), o_pErrorCode);
  }

  bool sync(ErrorCode* o_pErrorCode) {
    bool r = fsync(m_Fd) == 0;
    if(!r) {
      onSysCallError();
    }
    if(o_pErrorCode != nullptr) {
      *o_pErrorCode = r ? ErrorCode::OK : ErrorCode::SYNC_ERROR;
    }
    return r;
  }

  template<typename DataType>
  bool writeData(DataType i_Data, ErrorCode* o_pErrorCode = nullptr) {
    bool r = write(m_Fd, &i_Data, sizeof(DataType)) == sizeof(DataType);
    if(r) {
      r = sync(o_pErrorCode);
    } else {
      onSysCallError();
    }
    if(o_pErrorCode != nullptr) {
      *o_pErrorCode = r ? ErrorCode::OK : ErrorCode::WRITE_ERROR;
    }
    return r;
  }

  template <typename DataType>
  bool writeVector(std::vector<DataType> i_Data, ErrorCode* o_pErrorCode = nullptr) {
    auto expectedWriteSize = i_Data.size() * sizeof(DataType);
    bool r = write(m_Fd, &i_Data[0], expectedWriteSize) == expectedWriteSize;
    if(r) {
      r = sync(o_pErrorCode);
    } else {
      onSysCallError();
    }
    if(o_pErrorCode != nullptr) {
      *o_pErrorCode = r ? ErrorCode::OK : ErrorCode::WRITE_ERROR;
    }
    return r;
  }

  bool truncate(uint32_t i_u32Size, ErrorCode* o_pErrorCode = nullptr) {
    bool r = ftruncate(m_Fd, i_u32Size) == 0;
    if(r) {
      r = sync(o_pErrorCode);
    } else {
      onSysCallError();
    }
    if(o_pErrorCode != nullptr) {
      *o_pErrorCode = r ? ErrorCode::OK : ErrorCode::WRITE_ERROR;
    }
    return r;
  }

  template<typename DataType>
  bool read(DataType &o_Data, ErrorCode* o_pErrorCode = nullptr) {
    bool r = ::read(m_Fd, (void*) &o_Data, sizeof(DataType)) == sizeof(DataType);
    if(!r) {
      onSysCallError();
    }
    if(o_pErrorCode != nullptr) {
      *o_pErrorCode = r ? ErrorCode:: OK : ErrorCode::READ_ERROR;
    }
    return r;
  }

  template<typename DataType>
  bool readVector(std::vector<DataType> &o_Data, ErrorCode* o_pErrorCode = nullptr) {
    auto expectedReadSize = o_Data.size() * sizeof(DataType);
    bool r = ::read(m_Fd, &o_Data[0], expectedReadSize) == expectedReadSize;
    if(!r) {
      onSysCallError();
      if(o_pErrorCode != nullptr) {
        *o_pErrorCode = ErrorCode::READ_ERROR;
      }
    }
    return r;
  }

  uint32_t getByteOffsetFromIndex(uint32_t index) {
    return m_u32HeaderSize + (index * m_u32ContainerSize);
  }

  uint32_t getCurrentByteOffset() {
    return m_u32HeaderSize + (m_u32ContainerSize * m_CurrentHeader.offset);
  }

  bool readHeader(ErrorCode* o_pErrorCode = nullptr) {
    bool r = false;
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif
    
    if(seekTo(0, o_pErrorCode)) {
      r = read<HeaderType>(m_CurrentHeader, o_pErrorCode);
    }

    return r;
  }

  virtual void onVersionOlder(){};
  virtual void onVersionNewer(){};

  virtual void onMaxEntriesChanged(){};

  bool checkHeader(ErrorCode* o_pErrorCode = nullptr) {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif

    if (m_CurrentHeader.magic != m_ExpectedHeader.magic) {
      *o_pErrorCode = ErrorCode::MAGIC_MISMATCH;
      return false;
    }

    if (m_CurrentHeader.version < m_ExpectedHeader.version) {
      onVersionOlder();
    } else if (m_CurrentHeader.version > m_ExpectedHeader.version) {
      onVersionNewer();
    }

    if (m_CurrentHeader.maxEntries != m_ExpectedHeader.maxEntries) {
      onMaxEntriesChanged();
    }

    return true;
  }

  bool writeHeader(ErrorCode* o_pErrorCode = nullptr) {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif
    
    if(seekTo(0, o_pErrorCode)) {
      if(writeData<HeaderType>(m_ExpectedHeader, o_pErrorCode)) {
        return truncate(m_u32HeaderSize, o_pErrorCode);
      }
    }

    return false;
  }

  bool fixHeader(ErrorCode* o_pErrorCode = nullptr) {
    if (writeHeader(o_pErrorCode)) {
      if(readHeader(o_pErrorCode)) {
        return checkHeader(o_pErrorCode);
      }
    }
    
    return false;
  }

  void initialize() {
    m_Fd = open(m_Path.c_str(), O_RDWR | O_CREAT, 0644); // NOLINT(hicpp-signed-bitwise)
    if (m_Fd < 0) {
      m_ErrorCode = ErrorCode::OPEN_ERROR;
      m_Errors.emplace_back(strerror(errno));
    }

    if(m_ErrorCode == ErrorCode::OK) {
      if(!readHeader(&m_ErrorCode) || !checkHeader(&m_ErrorCode)) {
        (void)fixHeader(&m_ErrorCode);
      }
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

  virtual void beforeAppend(const std::vector<ContainerType>& i_Containers) {};

  virtual void onAppendSuccess(ContainerType /*i_Container*/) {};
  virtual void onAppendSuccess(const std::vector<ContainerType> &/*i_Containers*/) {};

  virtual void onAppendFailure(ContainerType i_Container){};
  virtual void onAppendFailure(const std::vector<ContainerType> &i_Containers){};

  ErrorCode append(ContainerType i_Container) {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif

    beforeAppend(i_Container);

    ErrorCode r = ErrorCode::OK;

    if(seekTo(getCurrentByteOffset(), &r)) {
      if(writeData<ContainerType>(i_Container, &r)) {
        (void)sync(&r);
        m_CurrentHeader.offset++;
        m_CurrentHeader.count++;
        onAppendSuccess(i_Container);
      } else {
        onAppendFailure(i_Container);
      }
    }
    
    return r;
  }

  bool _append(std::vector<ContainerType> &i_Containers, ErrorCode* o_pErrorCode = nullptr) {
    bool r = false;
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif

    if(m_CurrentHeader.offset + i_Containers.size() > m_CurrentHeader.maxEntries && m_CurrentHeader.maxEntries != 0) {
      auto writeableEntries = m_CurrentHeader.maxEntries - m_CurrentHeader.offset;
      auto entriesToWrite = std::vector<ContainerType>(i_Containers.begin(), i_Containers.begin() + writeableEntries);
      seekToIndex(m_CurrentHeader.offset);
      if(writeVector(entriesToWrite, o_pErrorCode)) {
        m_CurrentHeader.offset = 0;
        m_CurrentHeader.count += writeableEntries;
        i_Containers = std::vector<ContainerType>(i_Containers.begin() + writeableEntries, i_Containers.end());
        r = _append(i_Containers, o_pErrorCode);
      }
    } else {
      r = seekToIndex(m_CurrentHeader.offset, o_pErrorCode) && writeVector(i_Containers, o_pErrorCode);
      if(r) {
        if(i_Containers.size() == m_CurrentHeader.maxEntries) {
          m_CurrentHeader.offset = 0;
        } else {
          m_CurrentHeader.offset += i_Containers.size();
        }
        m_CurrentHeader.count += i_Containers.size();
      }
    }

    return r;
  }

  ErrorCode append(std::vector<ContainerType>& i_Containers) {
    ErrorCode r = ErrorCode::OK;
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif

    beforeAppend(i_Containers);

    if(_append(i_Containers, &r)) {
      onAppendSuccess(i_Containers);
    } else {
      onAppendFailure(i_Containers);
    }

    return r;
  }

  ErrorCode append(EntryType i_Entry) {
    ContainerType container(i_Entry);
    return append(container);
  }

  ErrorCode append(const std::vector<EntryType>& i_Entries) {
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

  bool getEntriesFromTo(std::vector<ContainerType>& o_Containers, uint32_t i_u32Start, uint32_t i_u32End, ErrorCode* o_pErrorCode = nullptr) {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif
    o_Containers.resize(i_u32End - i_u32Start);
    if(seekToIndex(i_u32Start, o_pErrorCode)) {
      return readVector(o_Containers, o_pErrorCode);
    }
    return false;
  }

  bool getEntry(uint32_t i_u32Index, ContainerType &o_Container, ErrorCode* o_pErrorCode = nullptr) {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif
    return seekToIndex(i_u32Index, o_pErrorCode) && read(o_Container, o_pErrorCode);
  }

  bool getEntriesFrom(std::vector<ContainerType> &o_Containers,
                      uint32_t i_u32Index, uint32_t i_u32Count, ErrorCode *o_pErrorCode = nullptr) {
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif
    o_Containers.resize(i_u32Count);
    return seekToIndex(i_u32Index, o_pErrorCode) && readVector(o_Containers, o_pErrorCode);
  }

  bool
  getEntriesChunked(std::function<void(std::vector<ContainerType>)> i_Callback,
                    uint32_t i_u32Begin = 0, uint32_t i_u32End = 0,
                    uint32_t i_u32ChunkSize = 100000,
                    ErrorCode *o_pErrorCode = nullptr) {
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
      if(getEntriesFromTo(tmp, i_u32Begin, i_u32Begin + rdCnt, o_pErrorCode)) {
        i_u32Begin += rdCnt;
        i_Callback(tmp);
      } else {
        return false;
      }
    }

    return true;
  }

  bool removeEntryAtEnd() {
    m_CurrentHeader.count--;
    m_CurrentHeader.offset--;
    return truncate(m_CurrentHeader.offset * m_u32ContainerSize + m_u32HeaderSize);
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
    return std::filesystem::file_size(m_Path);
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
#ifndef LOCK_FREE
    LG(m_Mutex);
#endif
    return truncate(m_u32HeaderSize);
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
