//
// Created by nbdy on 20.08.21.
//

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