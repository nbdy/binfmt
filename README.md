# binfmt
[![CodeFactor](https://www.codefactor.io/repository/github/nbdy/binfmt/badge)](https://www.codefactor.io/repository/github/nbdy/binfmt)
<br>
`A header only framework for binary file formats`
## benchmarks
### single insert (append EntryType)
|Number|Time (ms)|
|------|---------|
|1k    |1290     |
|10k   |13260    |
|100k  |toolong  |

### vector insert (append std::vector<EntryType>)
|Number|Time (ms)|
|------|---------|
|1k    |< 0 s    |
|1M    |~ 2 s    |


### read 
|Number|Time (ms)|
|------|---------|
|1M    |< 0 s    |


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

typedef BinaryEntryContainer<LogEntry> LogEntryContainer;
typedef BinaryFile<LogHeader, LogEntry, LogEntryContainer, 1000> LogFile;

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