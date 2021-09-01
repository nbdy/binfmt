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

#include <string>
#include <vector>
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
  explicit BinaryFileHeaderBase(uint32_t version, uint32_t magic): version(version), magic(magic) {}
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

struct FileUtils {
  /*!
   * Create a directory if it does not exist
   * @param Path
   * @return false if the directory could not be created
   */
  static bool CreateDirectory(const std::string& Path) {
    if (!Fs::exists(Path)) {
      return Fs::create_directory(Path);
    }
    return true;
  }

  /*!
   * Delete a directory if it exists
   * @param Path
   * @return true if Fs::remove was successful or the directory does not exist
   */
  static bool DeleteDirectory(const std::string& Path) {
    if (Fs::exists(Path)) {
      return Fs::remove(Path);
    }
    return true;
  }

  /*!
   * Use FCLOSE macro to close it and sync the fs
   * @param FilePath
   * @param Create
   * @return
   */
  static FILE* OpenBinaryFile(const std::string& FilePath, bool Create = false, uint32_t offset = 0) {
    FILE* r = nullptr;
    auto filePath = Fs::path(FilePath);
    if(!Fs::exists(filePath.parent_path())) {
      Fs::create_directories(filePath.parent_path());
    }
    if (Fs::exists(FilePath) || Create) {
      r = std::fopen(FilePath.c_str(), Create ? "wbe" : "r+be");
      if (fseek(r, offset, SEEK_SET) != 0) {
        (void) CloseBinaryFile(r);
        r = nullptr;
      }
    }
    return r;
  }

  /*!
   * Calls syncfs on the fp and fcloses it afterwards
   * @param fp
   * @return
   */
  static bool CloseBinaryFile(FILE* fp) {
    auto r = syncfs(fileno(fp));
    return fclose(fp) == 0 && r == 0;
  }

  /*!
   * Calculate the offset of an entry
   * @tparam HeaderType
   * @tparam EntryType
   * @param offset
   * @return
   */
  template<typename HeaderType, typename EntryType>
  static uint32_t CalculateEntryOffset(uint32_t offset) {
    return sizeof(HeaderType) + offset * sizeof(EntryType);
  }

  /*!
   * Creates file if does not exists,
   * writes the provided struct to the beginning of it
   * and closes the file again
   * @tparam HeaderType
   * @param FilePath
   * @param header
   * @return false if cannot seek beginning or write header data
   */
  template<typename HeaderType>
  static bool WriteHeader(const std::string& FilePath, HeaderType header) {
    auto* fp = OpenBinaryFile(FilePath, true);
    if (fp == nullptr) {
      return false;
    }
    auto r = fwrite(&header, sizeof(HeaderType), 1, fp);
    (void) CloseBinaryFile(fp);
    return r == 1;
  }

  /*!
   * Gets the first sizeof(HeaderType) bytes from a file and returns them as HeaderType.
   * @tparam HeaderType
   * @param FilePath
   * @return false if file does not exist, seek to start / reading the entry / closing the file failed
   */
  template<typename HeaderType>
  static bool GetHeader(const std::string& FilePath, HeaderType& r) {
    auto* fp = OpenBinaryFile(FilePath);
    if (fp == nullptr) {
      return false;
    }
    auto ret = fread(&r, sizeof(HeaderType), 1, fp);
    (void) CloseBinaryFile(fp);
    return ret == 1;
  }

  /*!
   * Write data entry to file. Skips size of HeaderType in bytes.
   * @tparam EntryType
   * @param FilePath
   * @param entry
   * @return false if file does not exist, seek to offset failed or we can't write the object
   */
  template<typename HeaderType, typename EntryType>
  static bool WriteData(const std::string& FilePath, EntryType entry, uint32_t offset = 0) {
    uint32_t o = sizeof(HeaderType) + offset * sizeof(EntryType);
    FILE* fp = OpenBinaryFile(FilePath, false, o);
    if (fp == nullptr) {
      return false;
    }
    auto r = fwrite(&entry, sizeof(EntryType), 1, fp);
    (void) CloseBinaryFile(fp);
    return r == 1;
  }

  /*!
   *
   * @tparam HeaderType
   * @tparam EntryType
   * @tparam EntryCount
   * @param FilePath
   * @return
   */
  template<typename HeaderType, typename EntryType>
  static bool ReadData(std::vector<EntryType>& Output, const std::string& FilePath, uint32_t offset = 0, uint32_t count = 0) {
    auto EntrySize = sizeof(EntryType);
    auto EntryCount = (GetFileSize(FilePath) - sizeof(HeaderType)) / EntrySize;
    uint32_t o = sizeof(HeaderType) + offset * EntrySize;
    FILE *fp = OpenBinaryFile(FilePath, false, o);

    if(count > 0) {
      EntryCount = count;
    }

    Output.resize(EntryCount);

    auto c = fread(&Output[0], EntrySize, EntryCount, fp);
    fclose(fp);

    return c == EntryCount;
  }

  /*!
   * Read file by object, so it should only take sizeof(EntryType) bytes of ram.
   * @tparam HeaderType
   * @tparam EntryType
   * @param FilePath
   * @param callback
   * @return false if file does not exist, seek to offset failed or file close at exit failed
   */
  template<typename HeaderType, typename EntryType>
  static bool ReadDataAt(const std::string& FilePath, uint32_t offset, EntryType& output) {
    auto EntrySize = sizeof(EntryType);
    uint32_t o = sizeof(HeaderType) + offset * EntrySize;
    FILE* fp = OpenBinaryFile(FilePath, false, o);
    if (fp == nullptr) {
      return false;
    }
    (void) fread(&output, EntrySize, 1, fp);
    (void) CloseBinaryFile(fp);
    return true;
  }

  /*!
   * Truncate a file to a certain offset
   * @tparam HeaderType
   * @param FilePath
   * @return false if the file does not exist, seek to start of data section begins / truncation / file closing failed
   */
  template <typename HeaderType, typename EntryType>
  static bool ClearFile(const std::string &FilePath, uint32_t offset = 0)
  {
    auto *fp = OpenBinaryFile(FilePath);
    if(fp == nullptr) {
      return false;
    }
    auto r = ftruncate(fileno(fp), GetFileSize<HeaderType, EntryType>(offset));
    (void) CloseBinaryFile(fp);
    return r == 0;
  }

  /*!
   * Gets the size of a file
   * @param FilePath
   * @param r
   * @return file size or 0 if file does not exist
   */
  static uintmax_t GetFileSize(const std::string& FilePath) {  // NOLINT(misc-definitions-in-headers)
    if (!Fs::exists(FilePath)) {
      return 0;
    }
    return Fs::file_size(FilePath);
  }

  /*!
   * Calculate the size of a file by its header and entry type size aswell as entry count
   * @tparam HeaderType
   * @tparam EntryType
   * @param EntryCount
   * @return
   */
  template<typename HeaderType, typename EntryType>
  static uint32_t GetFileSize(uint32_t EntryCount) {
    return sizeof(HeaderType) + sizeof(EntryType) * EntryCount;
  }

  /*!
   * Get the entry count based on the file and HeaderType / EntryType sizes
   * @tparam HeaderType
   * @tparam EntryType
   * @param FilePath
   * @return
   */
  template<typename HeaderType, typename EntryType>
  static uint32_t GetEntryCount(const std::string& FilePath) {
    auto fs = GetFileSize(FilePath);
    if (fs == 0) {
      return 0;
    }
    return (fs - sizeof(HeaderType)) / sizeof(EntryType);
  }

  /*!
   * Remove an entry at the specified position,
   * move all entries to the beginning and truncate the file by sizeof(EntryType)
   * @tparam HeaderType
   * @tparam EntryType
   * @param FilePath
   * @param position
   * @return
   */
  template<typename HeaderType, typename EntryType>
  static bool RemoveAt(const std::string& FilePath, uint32_t position) {
    auto ec = GetEntryCount<HeaderType, EntryType>(FilePath);
    if (ec == 0) {
      return false;
    }
    auto* fp = OpenBinaryFile(FilePath, false, CalculateEntryOffset<HeaderType, EntryType>(position));
    if (fp == nullptr) {
      return false;
    }
    bool ok = true;
    if (position < ec) {
      EntryType tmp;
      auto entriesLeft = ec - position;
      for (int c = 0; c < entriesLeft; c++) {
        if (ReadDataAt<HeaderType>(FilePath, position + 1 + c, tmp)) {
          if (!WriteData<HeaderType>(FilePath, tmp, position + c)) {
            ok = false;
            break;
          }
        } else {
          ok = false;
          break;
        }
      }
    }
    if (ok) {
      ok = ftruncate(fileno(fp), GetFileSize<HeaderType, EntryType>(GetEntryCount<HeaderType, EntryType>(FilePath) - 1))
          == 0;
    }
    CloseBinaryFile(fp);
    return ok;
  }

  /*!
   * Delete a file if it exists
   * @param FilePath
   * @return false if it does not exist, else the return value of std::filesystem::remove
   */
  static bool DeleteFile(const std::string &FilePath)
  {
    if (!Fs::exists(FilePath))
    {
      return false;
    }
    return Fs::remove(FilePath);
  }
};

struct AppendResult {
  bool rewind{};
  bool ok{};
  uint32_t offset{};
};

template<typename HeaderType, typename EntryType, typename ContainerType, uint32_t EntryCount>
class BinaryFile {
 public:
  enum ErrorCode {
    HEADER_OK = 0,
    HEADER_MISMATCH,
    NO_HEADER
  };

  [[maybe_unused]] explicit BinaryFile(std::string FilePath, HeaderType ExpectedHeader)
      : m_sFilePath(std::move(FilePath)) {
    initialize(ExpectedHeader);
  }

  [[maybe_unused]] explicit BinaryFile(std::string FilePath) : m_sFilePath(std::move(FilePath)) {
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
    if(FileUtils::GetHeader(m_sFilePath, tmp)) {
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

    auto ok = FileUtils::WriteData<HeaderType, ContainerType>(m_sFilePath, ContainerType(entry), m_uCurrentAppendOffset);
    if (ok) {
      m_uCurrentAppendOffset += 1;
    }

    return AppendResult{rewind, ok, m_uCurrentAppendOffset};
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
    if(FileUtils::ReadDataAt<HeaderType, ContainerType>(m_sFilePath, position, tmp)) {
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

 private:
  const uint32_t m_uHeaderSize = sizeof(HeaderType);
  const uint32_t m_uContainerSize = sizeof(ContainerType);
  const uint32_t m_uEntrySize = sizeof(EntryType);
  const uint32_t m_uMaxFileSize = m_uHeaderSize + m_uContainerSize * EntryCount;

  std::string m_sFilePath{};
  HeaderType m_Header;

  ErrorCode m_ErrorCode = ErrorCode::HEADER_OK;

  uint32_t m_uCurrentAppendOffset = 0;
};

#endif //BINFMT__BINFMT_H_
