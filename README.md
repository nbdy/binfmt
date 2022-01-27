# binfmt

[![CodeFactor](https://www.codefactor.io/repository/github/nbdy/binfmt/badge)](https://www.codefactor.io/repository/github/nbdy/binfmt)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/b485b4bdb1a546f8aac332245013bb81)](https://www.codacy.com/gh/nbdy/binfmt/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=nbdy/binfmt&amp;utm_campaign=Badge_Grade)
<br>
`A header only framework for binary file formats`

## [benchmarks](doc/Benchmarks.md)

### single insert (append EntryType)

| Number | Time (s) |
|--------|----------|
| 1k     | 1.35     |
| 10k    | 12.82    |
| 100k   | 128.65   |

### vector insert (append std::vector<EntryType>)

| Number | Time (ms) |
|--------|-----------|
| 1k     | 1         |
| 10k    | 1         |
| 100k   | 6         |
| 1M     | 40        |

### read

| Number | Time (s) |
|--------|----------|
| 1k     | 0ms      |
| 10k    | 0ms      |
| 100k   | 0ms      |
| 1M     | 8ms      |

## Minimal example

```c++
#include <iostream>
#include <binfmt.h>

struct LogHeader : public BinaryFileHeaderBase {
  LogHeader(): BinaryFileHeaderBase(1, 88448844) {}
};

struct LogEntry {
  char message[128];
};

using LogEntryContainer = BinaryEntryContainer<LogEntry>;
using LogFile = BinaryFile<LogHeader, LogEntry, LogEntryContainer, 1000>;

int main() {
  LogFile log("mylog.bin");

  log.append(LogEntry{"This is a log message"});
  log.append(LogEntry{"Another message"});

  std::vector<LogEntryContainer> entries;
  if (log.getAllEntries(entries)) {
    for (auto e : entries) {
      if(e.isEntryValid()) {
        std::cout << e.entry.message << std::endl;
      }
    }
  }

  return 0;
}
```

```shell
$ /home/nbdy/CLionProjects/binfmt/cmake-build-debug/binfmt_ex_min
This is a log message
Another message
```