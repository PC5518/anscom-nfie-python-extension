# Anscom: Enterprise-Grade Recursive File Scanner

**Anscom** is a high-performance, native C extension for Python designed for systems engineers, data analysts, and developers. It recursively scans directories, categorizes files by type, and generates detailed statistical reports with zero memory overhead.

Unlike standard Python scanners (like `os.walk`), Anscom is built in pure **C89**, allowing it to process massive filesystems instantly without flooding your terminal or exhausting system memory.

---

## 🚀 Key Features

*   **⚡ Native C Speed:** Written in pure C for maximum I/O throughput.
*   **🛡️ Smart Exclusion System:** Automatically hard-ignores "junk" directories (`node_modules`, `.venv`, `.git`, `__pycache__`, `build`, etc.) to prevent analysis pollution.
*   **📉 Zero-Memory Streaming:** Processes files one by one in a streaming fashion. Will never crash with `MemoryError`, even on drives with millions of files.
*   **🌳 Visual Tree Map:** Optional diagrammatic tree view of your folder structure.
*   **📊 Detailed Analytics:**
    *   **Category Summary:** Group files by Code, Images, Video, Audio, Archives, Executables, etc.
    *   **Extension Breakdown:** Exact counts for every file extension (`.py`, `.c`, `.png`, etc.).
*   **⏱️ Live Progress:** In-place progress counter to track scanning status without log spam.
*   **🛑 Safety Circuit Breaker:** Configurable recursion depth limit (Default: 6) to prevent stack overflows on deep system paths.

---

## 📦 Installation

Anscom is available on PyPI.

### Windows / Linux / macOS
```bash
pip install anscom
Note: Windows users require the "Desktop development with C++" workload (Visual Studio Build Tools) if compiling from source.
💻 Usage Guide & Examples
Here are the different ways you can use Anscom to scan your data.
1. The Standard Scan (Default)
Best for quick statistics. It uses Summary Only mode and a safety depth of 6.
Ignores .venv, node_modules, .git automatically.
Shows a live "Scanned files..." counter.
code
Python
import anscom

# Scan the current directory
anscom.scan(".")
2. The Visual Tree Scan
Best for understanding folder structure. Enables the diagrammatic view.
code
Python
import anscom

# Enable the tree view to see folder hierarchy
anscom.scan(".", show_tree=True)
Output Example:
code
Text
.
  |-- [src]
  |     |-- [main.c]
  |     |-- [utils.c]
  |-- [docs]
  |     |-- [readme.txt]
3. Deep System Scan
Best for massive nested directories. Increase max_depth to go deeper than the default (6).
code
Python
import anscom

# Go 20 levels deep and show the tree
anscom.scan("C:/Users/Admin/Projects", max_depth=20, show_tree=True)
4. Scanning an Entire Drive
Anscom is efficient enough to scan root drives.
code
Python
import anscom

# Scan the A: drive, keep tree off for speed, limit depth to 5
anscom.scan("A:/", max_depth=5, show_tree=False)
⚙️ API Reference
anscom.scan(path, max_depth=6, show_tree=False)
Parameter	Type	Default	Description
path	str	Required	The target directory path. Can be relative (.) or absolute (C:\Data).
max_depth	int	6	How many sub-folders deep the scanner will go. Prevents getting lost in deep system folders.
show_tree	bool	False	If True, prints the folder structure line-by-line. If False, shows a live progress counter.
Returns:
int: The total number of valid files scanned.
🛡️ The "Smart Filter" (Ignored Directories)
To ensure the statistics represent valuable data (and not library bloat), Anscom HARD EXCLUDES the following directories by default. They will not appear in the tree, and their files are not counted.
Development: .git, .svn, .hg, .idea, .vscode
Dependencies: node_modules, bower_components, site-packages, .venv, venv, env
Build Artifacts: build, dist, target, __pycache__
System/Temp: temp, tmp, .cache, .pytest_cache, .mypy_cache
Why? A single node_modules folder can contain 20,000+ tiny files that skew your statistics and slow down scanning by 90%. Anscom skips them to focus on your code and data.
📊 Understanding the Report
After scanning, Anscom prints two tables to the terminal:
1. Summary Report
High-level overview of file composition.
code
Text
=== SUMMARY REPORT ================================
+-----------------+--------------+----------+
| Category        | Count        | Percent  |
+-----------------+--------------+----------+
| Code/Source     |          150 |   60.00% |
| Images          |           50 |   20.00% |
| Documents       |           50 |   20.00% |
+-----------------+--------------+----------+
| TOTAL FILES     |          250 |  100.00% |
+-----------------+--------------+----------+
2. Detailed Breakdown
Exact counts of specific file extensions found.
code
Text
=== DETAILED BREAKDOWN ============================
+-----------------+--------------+
| Extension Type  | Count        |
+-----------------+--------------+
| .py             |          100 |
| .c              |           50 |
| .png            |           50 |
| .md             |           50 |
+-----------------+--------------+
🏆 Performance Architecture
Anscom is engineered differently from standard Python scripts:
Direct Syscall Interface: Uses FindFirstFileW (Windows) and readdir (POSIX) directly, bypassing Python's slow os module overhead.
Binary Search Categorization: Uses a compiled binary search algorithm to identify file extensions in O(log n) time.
I/O Throttling: The terminal output is buffered. In "Summary Mode", the progress bar updates only once per 100 files, preventing terminal I/O blocking.
License
MIT License. Open Source and free to use for personal and enterprise projects.
