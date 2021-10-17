//
// Created by nbdy on 12.10.21.
//

#ifndef BINFMT__FILEUTILS_H_
#define BINFMT__FILEUTILS_H_

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
    if (!Fs::exists(filePath.parent_path()) && Create) {
      Fs::create_directories(filePath.parent_path());
    }
    if (Fs::exists(FilePath) || Create) {
      r = std::fopen(FilePath.c_str(), Create ? "wbe" : "r+be");
      auto fn = fileno(r);
      if (lockf(fn, F_LOCK, 0) != 0) {
        (void) CloseBinaryFile(r);
        r = nullptr;
      }
      auto sr = fseek(r, offset, SEEK_SET);
      if (sr != 0) {
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
    auto fn = fileno(fp);
    (void) lockf(fn, F_ULOCK, 0);
    auto r = syncfs(fn);
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
   * Write data entry to file. Skips size of HeaderType in bytes.
   * @tparam EntryType
   * @param FilePath
   * @param entry
   * @return false if file does not exist, seek to offset failed or we can't write the object
   */
  template<typename HeaderType, typename EntryType>
  static bool WriteDataVector(const std::string& FilePath, std::vector<EntryType> entries, uint32_t offset = 0) {
    uint32_t o = sizeof(HeaderType) + offset * sizeof(EntryType);
    FILE* fp = OpenBinaryFile(FilePath, false, o);
    if (fp == nullptr) {
      std::cout << strerror(errno) << std::endl;
      return false;
    }
    auto r = fwrite(&entries[0], sizeof(EntryType), entries.size(), fp);
    (void) CloseBinaryFile(fp);
    return r == entries.size();
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
  static bool ReadData(std::vector<EntryType>& Output,
                       const std::string& FilePath,
                       uint32_t offset = 0,
                       uint32_t count = 0) {
    auto EntrySize = sizeof(EntryType);
    auto EntryCount = (GetFileSize(FilePath) - sizeof(HeaderType)) / EntrySize;
    uint32_t o = sizeof(HeaderType) + offset * EntrySize;
    FILE* fp = OpenBinaryFile(FilePath, false, o);

    if (count > 0) {
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
  template<typename HeaderType, typename EntryType>
  static bool ClearFile(const std::string& FilePath, uint32_t offset = 0) {
    auto* fp = OpenBinaryFile(FilePath);
    if (fp == nullptr) {
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
  static bool DeleteFile(const std::string& FilePath) {
    if (!Fs::exists(FilePath)) {
      return false;
    }
    return Fs::remove(FilePath);
  }
};

#endif // BINFMT__FILEUTILS_H_
