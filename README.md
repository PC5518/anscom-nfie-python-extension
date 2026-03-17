# AnsCom NFIE — Native Filesystem Intelligence Engine (Python C Extension)

[![PyPI version](https://badge.fury.io/py/anscom.svg)](https://pypi.org/project/anscom/)
![Current Release](https://img.shields.io/badge/release-1.3.0-blue)
![Python](https://img.shields.io/badge/python-%3E%3D3.6-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)
[![Homepage](https://img.shields.io/badge/homepage-anscom-yellow)](https://anscomqs.github.io/anscom/)

> Every large organization, research team, and development environment has storage they cannot see clearly. Anscom is the tool that makes it visible — what is on disk, how much, of what type, in how long, down to the exact file extension — returned as a structured Python dict that plugs directly into any pipeline, dashboard, or audit system.

`pip install anscom`

A globally published, native C-based Python extension for filesystem intelligence and large-scale directory analysis.

---

**AnsCom NFIE** is a high-performance, native C99 extension for Python designed for systems engineers, developers, data analysts, and researchers. It recursively scans directories, categorizes files by type, and generates detailed statistical reports with constant-memory streaming traversal.
Unlike standard Python scanners (like `os.walk`), AnsCom is built in native C, allowing it to process massive filesystems instantly without flooding your terminal or exhausting system memory.

---

## Key Features

* **Native C Speed:** Written in native C for maximum I/O throughput.
* **Smart Exclusion System:** Automatically hard-ignores junk directories (`node_modules`, `.venv`, `.git`, `__pycache__`, `build`, etc.).
* **Zero-Memory Streaming:** Processes files one by one in streaming fashion. Will never crash with `MemoryError`.
* **Visual Tree Map:** Optional diagrammatic tree view.
* **Detailed Analytics:**
  * Category Summary: Groups files by Code, Images, Video, Audio, Archives, Executables, etc.
  * Extension Breakdown: Exact counts for every file extension (`.py`, `.c`, `.png`, etc.).
* **Live Progress:** In-place progress counter.
* **Safety Circuit Breaker:** Configurable recursion depth limit (Default: 6).
* **Multi-Threaded Worker Pool:** Spawns N worker threads (auto-detected from CPU core count) pulling directories from a shared work queue.
* **Extension Whitelisting:** Pass a list of extensions to count only those file types.
* **Silent Mode:** Suppress all terminal output — ideal for embedding inside pipelines or CI systems.
* **Minimum Size Filter:** Skip files below a byte threshold.
* **Cross-Platform:** Three separate OS-optimized backends: `FindFirstFileW` (Windows), `getdents64` direct syscall (Linux), POSIX `readdir` fallback (macOS/BSD).
* **JSON Export:** Export full scan results as a formatted `.json` file — zero external dependencies, fully native.
* **Tree Export:** Save the complete DFS directory tree to a `.txt` file simultaneously alongside stdout.
* **Excel Export:** Export scan results to a structured `.xlsx` file with Categories, Extensions, and Summary sheets (requires `openpyxl`).

---

## Installation

### Windows / Linux / macOS

```bash
pip install anscom
```

> **Note for Windows users:** Compiling from source requires the "Desktop development with C++" workload from Visual Studio Build Tools.

### Optional: Excel export support

```bash
pip install anscom[excel]
```

> ⚠️ **Windows users:** If you encounter a `SystemError` with `export_excel`, use the Python wrapper approach shown below instead.

### Verify installation

```python
import anscom
result = anscom.scan(".", silent=True)
print(f"Anscom is working. Files found: {result['total_files']}")
```

---

## Usage Guide

### Standard Scan
```python
import anscom
anscom.scan(".")
```

### Visual Tree Mode
```python
import anscom
anscom.scan(".", show_tree=True)
```

### Deep Scan
```python
anscom.scan("C:/Users/Admin/Projects", max_depth=20, show_tree=True)
```

### Full Drive Scan
```python
anscom.scan("A:/", max_depth=5, show_tree=False)
```

### Export to JSON (Native — no dependencies)
```python
import anscom
anscom.scan(".", export_json="results.json")
```

### Export Tree to File
```python
import anscom
anscom.scan(".", show_tree=True, max_depth=20, export_tree="tree.txt")
```

### Export to Excel
<img width="12" height="13" alt="image" src="https://github.com/user-attachments/assets/aa68f237-deff-48ad-a7bf-679d1ff57b71" />

```python
import anscom
anscom.scan(".", export_excel="results.xlsx")
```

### All exports in one scan pass
```python
import anscom
anscom.scan(
    ".",
    show_tree=True,
    max_depth=20,
    export_json="results.json",
    export_tree="tree.txt",
    export_excel="results.xlsx"
)
```

---

## Demo

A real scan of a local project directory — 4,262 files across code, documents, audio, images and more, completed in just over 1 second.

**Tree output (excerpt):**
```
  |     |     |-- [factor_models]
  |     |     |     |-- fama_french_5factor_regression.py
  |     |     |     |-- momentum_alpha_backtest.ipynb
  |     |     |     |-- risk_adjusted_returns_analysis.pdf
  |     |     |     |-- rolling_sharpe_ratio_estimation.py
  |     |     |     |-- covariance_matrix_shrinkage.py
  |     |     |-- [market_microstructure]
  |     |     |     |-- order_flow_imbalance_model.py
  |     |     |     |-- bid_ask_spread_decomposition.pdf
  |     |     |     |-- tick_data_preprocessing.py
  |     |     |-- [research_papers]
  |     |     |     |-- gold_etf_fair_value_engine_v3.pdf
  |     |     |     |-- yield_curve_arbitrage_strategy.pdf
  |     |     |     |-- [figures]
  |     |     |     |     |-- cumulative_returns_plot.png
  |     |     |     |     |-- drawdown_analysis_chart.png
```

**Summary Report:**
```
=== SUMMARY REPORT ================================
+-----------------+--------------+----------+
| Category        | Count        | Percent  |
+-----------------+--------------+----------+
| Code/Source     |         1941 |   45.54% |
| Documents       |          585 |   13.73% |
| Images          |          369 |    8.66% |
| Videos          |           72 |    1.69% |
| Audio           |          748 |   17.55% |
| Archives        |           11 |    0.26% |
| Executables     |           50 |    1.17% |
| System/Config   |           22 |    0.52% |
| Other/Unknown   |          464 |   10.89% |
+-----------------+--------------+----------+
| TOTAL FILES     |         4262 |  100.00% |
+-----------------+--------------+----------+
```

**Detailed Extension Breakdown:**
```
=== DETAILED EXTENSION BREAKDOWN ==================
+-----------------+--------------+
| Extension       | Count        |
+-----------------+--------------+
| .js             |          816 |
| .ts             |          512 |
| .mp3            |          730 |
| .py             |          270 |
| .pdf            |          269 |
| .json           |          208 |
| .md             |          189 |
| .jpg            |          247 |
| .png            |           98 |
| .mp4            |           55 |
| .xlsx           |           23 |
| .docx           |           32 |
| .csv            |           29 |
+-----------------+--------------+

Time     : 1.0315 seconds
Errors   : 0 (permission denied / inaccessible)
===================================================
```

4,262 files. 1.03 seconds. Zero errors. Zero configuration.

---

## API Reference

### `anscom.scan(path, max_depth=6, show_tree=False, workers=0, min_size=0, extensions=None, callback=None, silent=False, ignore_junk=False, export_json=None, export_tree=None, export_excel=None)`

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `path` | `str` | Required | Target directory. Relative (`.`) or absolute (`C:\Data`). Empty string treated as `.`. |
| `max_depth` | `int` | `6` | Recursion depth limit. Clamped to `[0, 64]`. |
| `show_tree` | `bool` | `False` | If `True`, prints a DFS-ordered directory tree to stdout. Forces `workers=1`. |
| `workers` | `int` | `0` | Thread count. `0` = read from OS (core count). |
| `min_size` | `int` | `0` | Skip files smaller than this byte count. `0` = no filter. |
| `extensions` | `list[str]` | `None` | Whitelist. When set, only matching extensions are counted. |
| `callback` | `callable` | `None` | Called with current scanned file count every ~1 second. GIL is acquired before each call. |
| `silent` | `bool` | `False` | Suppresses summary report and progress counter. |
| `ignore_junk` | `bool` | `False` | When `True`, skips directories in the hardcoded exclusion list entirely. |
| `export_json` | `str` | `None` | If a file path is provided, exports full scan results as a formatted `.json` file. Zero external dependencies — fully native. |
| `export_tree` | `str` | `None` | If a file path is provided and `show_tree=True`, mirrors the full tree output to a `.txt` file simultaneously alongside stdout. |
| `export_excel` | `str` | `None` | If a file path is provided, exports scan results to an `.xlsx` file with three sheets: Categories, Extensions, Summary. Requires `openpyxl`. |

**Returns:** `dict`

| Key | Type | Description |
|---|---|---|
| `total_files` | `int` | Files that passed all filters and were categorized |
| `scan_errors` | `int` | Paths that could not be opened |
| `duration_seconds` | `float` | Wall-clock elapsed time |
| `categories` | `dict[str, int]` | All 9 categories always present |
| `extensions` | `dict[str, int]` | Only extensions with count > 0 |

---

## Export Features

### export_json — Native, Zero Dependencies

<img width="12" height="12" alt="image" src="https://github.com/user-attachments/assets/4bc11c50-37f5-4a31-883c-9d1e2c222b41" />

Export the full scan result as a formatted JSON file. Uses Python's built-in `json` module internally — no `pip install` required.

```python
import anscom

anscom.scan(".", export_json="results.json")
```

Example output (`results.json`):
```json
{
    "total_files": 4262,
    "scan_errors": 0,
    "duration_seconds": 1.0315,
    "categories": {
        "Code/Source": 1941,
        "Documents": 585,
        "Images": 369,
        "Videos": 72,
        "Audio": 748,
        "Archives": 11,
        "Executables": 50,
        "System/Config": 22,
        "Other/Unknown": 464
    },
    "extensions": {
        "py": 270,
        "js": 816,
        "mp3": 730
    }
}
```

### export_tree — Save DFS Tree to File

Only active when `show_tree=True`. Both stdout and the file receive every line simultaneously — no buffering, no memory accumulation.

```python
import anscom

anscom.scan(
    ".",
    show_tree=True,
    max_depth=50,
    export_tree="filesystem_tree.txt"
)
```

### export_excel — Structured XLSX Report

Requires `openpyxl` (`pip install anscom[excel]`):

```python
import anscom

anscom.scan(".", export_excel="results.xlsx")
```

| Sheet | Columns | Content |
|---|---|---|
| `Categories` | Category, Count, Percentage | All 9 categories |
| `Extensions` | Extension, Count | All non-zero extensions |
| `Summary` | Metric, Value | Total files, errors, duration |

> ⚠️ **Windows users:** If `export_excel` raises a `SystemError`, use this Python wrapper instead:

```python
import anscom
import openpyxl

def scan_to_excel(path, excel_path="results.xlsx", **kwargs):
    kwargs.pop("export_excel", None)
    result = anscom.scan(path, **kwargs)

    wb = openpyxl.Workbook()

    ws1 = wb.active
    ws1.title = "Categories"
    ws1.append(["Category", "Count", "Percentage"])
    for cat, count in result["categories"].items():
        pct = round(count / result["total_files"] * 100, 2) if result["total_files"] else 0
        ws1.append([cat, count, pct])

    ws2 = wb.create_sheet("Extensions")
    ws2.append(["Extension", "Count"])
    for ext, count in result["extensions"].items():
        ws2.append([ext, count])

    ws3 = wb.create_sheet("Summary")
    ws3.append(["Metric", "Value"])
    ws3.append(["Total Files", result["total_files"]])
    ws3.append(["Errors", result["scan_errors"]])
    ws3.append(["Duration (s)", round(result["duration_seconds"], 4)])

    wb.save(excel_path)
    return result

scan_to_excel(".", excel_path="results.xlsx")
```

### All exports in one pass

```python
import anscom

anscom.scan(
    "/mnt/enterprise-storage",
    max_depth=20,
    show_tree=True,
    silent=True,
    ignore_junk=True,
    export_json="audit.json",
    export_tree="tree.txt",
    export_excel="report.xlsx"
)
```

One scan pass. Three output files. No re-scanning.

---

## Smart Filter (Auto-Ignored when `ignore_junk=True`)

`.git`, `node_modules`, `.venv`, `venv`, `build`, `dist`, `__pycache__`, `temp`, `tmp`, `.cache`, `.pytest_cache`, `.mypy_cache`, `site-packages`, etc.

---

## Report Output

After scanning, AnsCom prints:
- Summary Report
- Detailed Extension Breakdown

---

## Performance Architecture

- Direct OS syscalls (`FindFirstFileW` on Windows, `getdents64` on Linux, `readdir` on POSIX)
- FNV-1a open-addressing hash table for O(1) extension lookup
- Per-thread statistics accumulation — no shared locks during file counting
- Slab path allocator — one `malloc` per thread before scan, zero allocations during traversal
- Hardware atomic progress counter (`__sync_fetch_and_add` / `InterlockedExchangeAdd64`)
- Hybrid parallel/inline dispatch — queue at shallow depths, inline recursion at depth

---

## Demo
<img width="280" height="879" alt="image" src="https://github.com/user-attachments/assets/c23cc5ec-3ae9-43e5-8580-5f397bd413a2" />
<img width="649" height="146" alt="image" src="https://github.com/user-attachments/assets/96bcf704-224b-47ac-bb32-e8976a914776" />

---

## Use Cases

### Codebase Analysis

Quickly understand the structure of a large repository before diving in.

```python
import anscom
anscom.scan(".")
```

Get answers like:
- How many source files exist in the project?
- What programming languages are present, and in what proportion?
- How are file types distributed across the repository?

---

### Dataset Inspection

Machine learning projects often contain datasets with thousands of files spread across nested directories. Anscom lets you verify a dataset's composition before kicking off a training run.

```python
anscom.scan("datasets/")
```

---

### Storage Auditing

Understand what's consuming space in a directory — whether it's archives piling up, temporary files accumulating, or an unexpected file type dominating.

```python
anscom.scan("/data/storage", max_depth=10)
```

---

### Large Filesystem Exploration

Modern development environments can contain tens of thousands of files once dependencies are included. Anscom focuses only on what matters by automatically skipping known junk directories.

| Ignored Directory | Reason |
|---|---|
| `.git` | Version control internals |
| `node_modules` | JavaScript dependencies |
| `.venv` | Python virtual environments |
| `__pycache__` | Python bytecode cache |
| `build` | Compiled output |
| `dist` | Distribution artifacts |

---

### Development Tool Integration

Anscom is designed to be embedded as a backend engine for tools that need filesystem analytics:

- Code statistics dashboards
- Project analysis and reporting tools
- Automated repository audits
- DevOps pipeline checks and gates

---

## Enterprise Use Cases

### Storage Intelligence & Cost Allocation

```python
import anscom

result = anscom.scan(
    "/mnt/nas-storage",
    max_depth=20,
    silent=True,
    ignore_junk=True,
    workers=16
)

total = result["total_files"]
media = result["categories"]["Videos"] + result["categories"]["Images"] + result["categories"]["Audio"]
code  = result["categories"]["Code/Source"]

print(f"Media files:  {media:,}  ({media/total*100:.1f}%)")
print(f"Source code:  {code:,}  ({code/total*100:.1f}%)")
print(f"Scan time:    {result['duration_seconds']:.3f}s")
```

### Pre-Migration Filesystem Audit

```python
import anscom

result = anscom.scan(
    "/legacy-server/data",
    max_depth=30,
    silent=True,
    export_json="pre_migration_audit.json"
)

print(f"Audit complete: {result['total_files']:,} files recorded.")
```

### CI/CD Pipeline File Validation

```python
import anscom, sys

result = anscom.scan("./repo", silent=True, ignore_junk=True)

if result["categories"]["Executables"] > 0:
    print(f"POLICY VIOLATION: {result['categories']['Executables']} executables found in repo.")
    sys.exit(1)

print("File composition check passed.")
```

### Full Filesystem Tree Capture (Terabyte-Scale)

Anscom imposes no internal limit on the volume of filesystem it will traverse. Memory usage is bounded by the work queue capacity (131,072 directory slots) and the per-thread path slabs — neither grows with the total number of files on disk.

```python
import anscom

anscom.scan(
    "/mnt/large-volume",
    max_depth=64,
    show_tree=True,
    silent=True,
    ignore_junk=True,
    export_tree="filesystem_tree.txt"
)
```

---

## Release History

| Version | Date | Notes |
|---|---|---|
| **1.3.0** | 15 March 2026 | **Export release.** Added `export_json` (native, zero dependencies), `export_tree` (DFS tree to `.txt`), and `export_excel` (structured `.xlsx` with openpyxl). MSVC Windows compiler compatibility fix for `uint64_t` / `stdint.h`. |
| **1.0.0** | 13 March 2026 | **Major release.** Terabyte-scale filesystem scanning. Multi-threaded worker pool, `getdents64` direct syscall backend (Linux), FNV-1a hash table for O(1) extension lookup, per-thread statistics with zero shared locks, slab path allocator, extension whitelisting, silent mode, min_size filter, and live callback interface. |
| 0.6.0 | Jan 15, 2026 | Added features like data tree and many more |
| 0.5.0 | Jan 15, 2026 | — |
| 0.4.0 | Jan 15, 2026 | Release of the basic version with the concept of anscom |

---

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you'd like to change.

---

## anscom official web

https://anscomqs.github.io/anscom/

---

## License

MIT License
