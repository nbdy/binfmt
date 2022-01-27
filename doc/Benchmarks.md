# Ryzen 9 3900xt

```
[==========] Running 7 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 7 tests from BinaryFile
[ RUN      ] BinaryFile.test1kSingleInsert
Single insert of 1000 items took 1354ms
Size: 8020
1000 getAllEntries: 0ms
[       OK ] BinaryFile.test1kSingleInsert (1366 ms)
[ RUN      ] BinaryFile.test10kSingleInsert
Single insert of 10000 items took 12817ms
Size: 80020
10000 getAllEntries: 0ms
[       OK ] BinaryFile.test10kSingleInsert (12820 ms)
[ RUN      ] BinaryFile.test100kSingleInsert
Single insert of 100000 items took 128647ms
Size: 800020
100000 getAllEntries: 1ms
[       OK ] BinaryFile.test100kSingleInsert (128656 ms)
[ RUN      ] BinaryFile.test1kVectorInsert
Vector insert of 1000 items took 1ms
Size: 8020
1000 getAllEntries: 0ms
[       OK ] BinaryFile.test1kVectorInsert (5 ms)
[ RUN      ] BinaryFile.test10kVectorInsert
Vector insert of 10000 items took 1ms
Size: 80020
10000 getAllEntries: 0ms
[       OK ] BinaryFile.test10kVectorInsert (5 ms)
[ RUN      ] BinaryFile.test100kVectorInsert
Vector insert of 100000 items took 6ms
Size: 800020
100000 getAllEntries: 0ms
[       OK ] BinaryFile.test100kVectorInsert (19 ms)
[ RUN      ] BinaryFile.test1MVectorInsert
Vector insert of 1000000 items took 40ms
Size: 8000020
1000000 getAllEntries: 8ms
[       OK ] BinaryFile.test1MVectorInsert (153 ms)
[----------] 7 tests from BinaryFile (143024 ms total)

[----------] Global test environment tear-down
[==========] 7 tests from 1 test suite ran. (143024 ms total)
[  PASSED  ] 7 tests.

```