# AnsCom NFIE — Native Filesystem Intelligence Engine (Python C Extension)

[![PyPI version](https://badge.fury.io/py/anscom.svg)](https://pypi.org/project/anscom/)
![Current Release](https://img.shields.io/badge/release-1.0.0-blue)
![Python](https://img.shields.io/badge/python-%3E%3D3.6-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)

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

---

## Installation

### Windows / Linux / macOS

```bash
pip install anscom
```

> **Note for Windows users:** Compiling from source requires the "Desktop development with C++" workload from Visual Studio Build Tools.

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

### `anscom.scan(path, max_depth=6, show_tree=False, workers=0, min_size=0, extensions=None, callback=None, silent=False, ignore_junk=False)`

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

**Returns:** `dict`

| Key | Type | Description |
|---|---|---|
| `total_files` | `int` | Files that passed all filters and were categorized |
| `scan_errors` | `int` | Paths that could not be opened |
| `duration_seconds` | `float` | Wall-clock elapsed time |
| `categories` | `dict[str, int]` | All 9 categories always present |
| `extensions` | `dict[str, int]` | Only extensions with count > 0 |

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
import anscom, json, datetime

result = anscom.scan("/legacy-server/data", max_depth=30, silent=True)
result["audit_timestamp"] = str(datetime.datetime.utcnow())
result["hostname"] = "legacy-srv-04"

with open("pre_migration_audit.json", "w") as f:
    json.dump(result, f, indent=2)
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

Anscom imposes no internal limit on the volume of filesystem it will traverse. Memory usage is bounded by the work queue capacity (131,072 directory slots) and the per-thread path slabs — neither grows with the total number of files on disk. Scanning 50 million files does not consume meaningfully more memory than scanning 50,000.

```python
import anscom, sys

class FileWriter:
    def __init__(self, path):
        self.f = open(path, "w", encoding="utf-8", buffering=1024 * 1024)
    def write(self, s):
        self.f.write(s)
    def flush(self):
        self.f.flush()

writer = FileWriter("filesystem_tree.txt")
sys.stdout = writer

anscom.scan(
    "/mnt/large-volume",
    max_depth=64,
    show_tree=True,
    silent=True,
    ignore_junk=True
)

sys.stdout = sys.__stdout__
writer.f.close()
```

---

## Release History

| Version | Date | Notes |
|---|---|---|
| **1.0.0** | March 2026 | **Major release.** Terabyte-scale filesystem scanning — previous versions were limited in the volume they could reliably traverse. This release introduces a multi-threaded worker pool, `getdents64` direct syscall backend (Linux), FNV-1a hash table for O(1) extension lookup, per-thread statistics with zero shared locks during scan, slab path allocator (zero heap allocation during traversal), extension whitelisting, silent mode, min_size filter, and live callback interface. Memory usage is now bounded regardless of filesystem size. |
| 0.6.0 | Jan 15, 2026 | added features like data tree and many more |
| 0.5.0 | Jan 15, 2026 | — |
| 0.4.0 | Jan 15, 2026 | Release of the basic version with the concept of anscom|

---

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you'd like to change.

---

## License

MIT License

