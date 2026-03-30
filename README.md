# Memory-Efficient Versioned File Indexer

CS253 Course - Programming Assignment'26

Language: C++17

By: Joel Bansal (230509)

---

## What This Project Does

This program builds a word-level frequency index over one or more large text log files and answers three types of analytical queries against that index. Files are never loaded into memory all at once — instead, they are read incrementally using a fixed-size buffer. Each file is associated with a user-provided version name, and multiple versions can coexist in the same index, enabling cross-version comparison.

A word-level index maps every unique word (contiguous alphanumeric characters, lowercased) to how many times it appears in that version's file.

---

## How to Compile

Make sure you have GCC installed. On Windows with MinGW, run:

    g++ -O2 -o analyzer main.cpp -static-libgcc -static-libstdc++

The `-static-libgcc -static-libstdc++` flags bundle the runtime libraries into the executable so it runs without MinGW DLLs in PATH.

On Linux/macOS:

    g++ -O2 -o analyzer main.cpp

---

## How to Run

There are three query types: `word`, `top`, and `diff`.

### word — frequency of a specific word in a version

    ./analyzer --file test_logs.txt --version v1 --buffer 256 --query word --word error

### top — top K most frequent words in a version

    ./analyzer --file test_logs.txt --version v1 --buffer 256 --query top --top 10

### diff — difference in word frequency between two versions

    ./analyzer --file1 test_logs.txt --version1 v1 --file2 verbose_logs.txt --version2 v2 --buffer 256 --query diff --word error

---

## Command Line Arguments

| Argument        | Applicable Queries | Description                                        |
|-----------------|--------------------|----------------------------------------------------|
| `--file <path>` | word, top          | Path to the input file                             |
| `--file1 <path>`| diff               | Path to the first input file                       |
| `--file2 <path>`| diff               | Path to the second input file                      |
| `--version <n>` | word, top          | Version identifier for the file                    |
| `--version1 <n>`| diff               | Version identifier for the first file              |
| `--version2 <n>`| diff               | Version identifier for the second file             |
| `--query <type>`| all                | Query type: `word`, `top`, or `diff`               |
| `--word <token>`| word, diff         | The word to look up (case-insensitive)             |
| `--top <k>`     | top                | Number of top results to return (must be > 0)      |
| `--buffer <kb>` | all                | Buffer size in KB, 256–1024 (default: 512)         |

---

## Output Format

Every run prints to stdout. All formats are shown below.

**word query:**

    Version: <version>
    Count: <frequency>
    Buffer Size (KB): <size>
    Execution Time (s): <seconds>

**top query:**

    Version: <version>
    Top <k> words in Version: <version>:
    <word1>: <count1>
    <word2>: <count2>
    ...
    Buffer Size (KB): <size>
    Execution Time (s): <seconds>

**diff query:**

    Version 1: <version>
    Count 1: <frequency>
    Version 2: <version>
    Count 2: <frequency>
    Difference (<version2> - <version1>): <delta>
    Buffer Size (KB): <size>
    Execution Time (s): <seconds>

Note: word matching is case-insensitive (`DEVOPs`, `devops`, and `DEVOPS` all return the same count). Informational log messages go to stderr and do not appear in the query output.

---

## Buffer and Memory Design

- **Buffer Size**: Configurable 256-1024 KB via `--buffer`; allocated once using `std::unique_ptr<char[]>`
- **Word Boundary Handling**: Partial words at chunk boundaries are saved and prepended to next chunk to prevent token loss
- **Memory Efficiency**: Only frequency map grows with input; raw content never accumulated; memory bounded by distinct word count, not file size

---
