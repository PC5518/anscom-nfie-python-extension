# anscom — a filesystem scanner for Python (C extension) (update coming soon for windows devices to be used without vs studio dev tools )

### My self-directed opensource project by learning C and Python internals.

*Built for fun and for learning. Published on PyPI under the MIT licence so anyone can read the source, try it, or take it apart.*

<table>
<tr>
<td valign="middle" width="70%">

[![PyPI version](https://badge.fury.io/py/anscom.svg)](https://pypi.org/project/anscom/)
![Current Release](https://img.shields.io/badge/release-1.5.3-blue)
![Python](https://img.shields.io/badge/python-%3E%3D3.6-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)
[![Homepage](https://img.shields.io/badge/homepage-anscom-yellow)](https://anscomqs.github.io/anscom/)
[![PyPI Downloads](https://static.pepy.tech/personalized-badge/anscom?period=total&units=NONE&left_color=BLACK&right_color=BRIGHTGREEN&left_text=Downloaded+by)](https://pepy.tech/projects/anscom)

</td>
<td valign="middle" align="center" width="30%">
<img src="https://upload.wikimedia.org/wikipedia/commons/3/35/Tux.svg" height="65" alt="Linux" />&nbsp;&nbsp;<img src="https://upload.wikimedia.org/wikipedia/commons/8/87/Windows_logo_-_2021.svg" height="55" alt="Windows" />&nbsp;&nbsp;<img src="https://cdn3d.iconscout.com/3d/free/thumb/free-3d-mac-os-finder-3d-icon-png-download-2476743.png?f=webp" height="45" alt="macOS" />&nbsp;&nbsp;<img src="https://upload.wikimedia.org/wikipedia/commons/thumb/6/64/PyPI_logo.svg/1280px-PyPI_logo.svg.png" height="45" alt="PyPI" />
</td>
</tr>
</table>

---

## About this project

anscom is a self-directed educational project. I started it to learn two things: how operating systems organise files on disk, and how Python can be extended with native C code. It reads a directory that you point it at and reports what is inside — how many files, of what types, how large — and returns the result as a plain Python dictionary.

It only reads. It does not modify, move, delete, or transmit anything, and it works on directories you already have access to on your own machine.

---

<img width="1474" height="877" alt="image" src="https://github.com/user-attachments/assets/fefc90dc-5051-468b-8722-3ccea10436f3" />

<table>
<tr>
<td width="80px" valign="middle" align="center">
<img src="https://avatars.githubusercontent.com/u/252811141?s=400&u=1ab402f781d9a27985950db4447723e90a1c3703&v=4" height="70" width="70" style="border-radius:12px;" alt="Anscom" />
</td>
<td valign="middle">

Most of us lose track of what is actually stored in a project folder once it grows past a few thousand files. anscom answers that question directly — what is there, how much of it, of what type, and how long it took to find out, down to the exact file extension — returned as a structured Python dict that plugs straight into a script, notebook, or report.
</td>
</tr>
</table>

---

**anscom** is a native C99 extension for Python, useful for students, developers, data analysts, and researchers who want to understand the shape of a directory tree. It walks directories recursively, groups files by type, and produces summary reports using constant-memory traversal.

Unlike standard Python scanners (like `os.walk`), anscom is built in native C, allowing it to process large directory trees quickly without flooding your terminal or exhausting system memory.

---

## Key Features

* **Native C Speed:** Written in native C for maximum I/O throughput.
* **Smart Exclusion System:** Automatically skips junk directories (`node_modules`, `.venv`, `.git`, `__pycache__`, `build`, etc.).
* **Zero-Memory Streaming:** Processes files one by one in streaming fashion. Will never crash with `MemoryError`.
* **Visual Tree Map:** Optional diagrammatic tree view.
* **Detailed Reports:**
  * Category Summary: Groups files by Code, Images, Video, Audio, Archives, Executables, etc.
  * Extension Breakdown: Exact counts for every file extension (`.py`, `.c`, `.png`, etc.).
* **Live Progress:** In-place progress counter.
* **Safety Depth Limit:** Configurable recursion depth limit (Default: 6).
* **Multi-Threaded Worker Pool:** Spawns N worker threads (auto-detected from CPU core count) pulling directories from a shared work queue.
* **Extension Whitelisting:** Pass a list of extensions to count only those file types.
* **Silent Mode:** Suppress all terminal output — useful when calling it from another script.
* **Minimum Size Filter:** Skip files below a byte threshold.
* **Cross-Platform:** Three separate OS-optimized backends: `FindFirstFileW` (Windows), `getdents64` direct syscall (Linux), POSIX `readdir` fallback (macOS/BSD).
* **JSON Export:** Export full scan results as a formatted `.json` file — zero external dependencies, fully native.
* **Tree Export:** Save the complete DFS directory tree to a `.txt` file simultaneously alongside stdout.
* **CSV Export** *(new in v1.5.0)*: Per-file inventory as a UTF-8 CSV — `path`, `size`, `ext`, `category`, `mtime`. Zero dependencies, RFC 4180-compliant quoting.
* **Per-File Return** *(new in v1.5.0)*: Opt-in `return_files=True` adds a complete file list to the returned dict for downstream Python processing.
* **Top-N Largest Files** *(new in v1.5.0)*: `largest_n=N` reports the largest files via per-thread min-heap — O(log N) per file, no extra pass.
* **Duplicate Detection** *(new in v1.5.0)*: Two-phase size-bucket + CRC32 checksum comparison — zero I/O for unique-size files.
* **Regex Path Filter** *(new in v1.5.0)*: `regex_filter` restricts the scan to paths matching a pattern. POSIX `regexec` on Linux/macOS for zero GIL overhead.
* **Current Directory Helper** *(new in v1.5.1)*: `anscom.pwd()` returns the working directory as an absolute string (the Unix `pwd`), `anscom.scan()` with no path scans it, and every result dict now includes `scanned_path` — the absolute path that was actually walked.
* **Mascot** *(new in v1.5.2)*: `anscom.about()` prints **Chuhi**, the anscom mascot (the house mouse of a Kolkata apartment), as ASCII art with a short credit line.
* **Byte Totals** *(new in v1.5.3)*: `with_size=True` adds `total_size`, `category_sizes` and `extension_sizes` to the result — bytes alongside the counts, so you can see *where the disk actually went*, not just how many files there are. Opt-in, because on Linux it costs one extra `stat()` per file.

---

## Installation

### Windows / Linux / macOS

```bash
pip install anscom
```

> **Note for Windows users:** Compiling from source requires the "Desktop development with C++" workload from Visual Studio Build Tools.

> **Native build (optional, Linux/macOS):** the default source build is portable. To compile with `-march=native` tuned to your CPU (best on a machine you both build and run on), set `ANSCOM_NATIVE=1` before installing.

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

### Current Directory (pwd) *(new in v1.5.1)*
```python
import anscom

print(anscom.pwd())              # absolute path of the current working directory
result = anscom.scan()           # no argument → scans the current directory
print(result["scanned_path"])    # absolute path that was actually walked
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

### Export to CSV *(new in v1.5.0)*
```python
import anscom
anscom.scan(".", export_csv="inventory.csv")
```

Per-file UTF-8 CSV with columns `path,size,ext,category,mtime`. RFC 4180-compliant quoting. Zero external dependencies.

### Return Per-File List *(new in v1.5.0)*
```python
import anscom
result = anscom.scan(".", return_files=True, silent=True)
for f in result["files"]:
    print(f["path"], f["size"], f["category"])
```

### Top-N Largest Files *(new in v1.5.0)*
```python
import anscom
result = anscom.scan("/mnt/storage", largest_n=20, silent=True)
for f in result["largest_files"]:
    print(f"{f['size']/1024**3:.2f} GB  {f['path']}")
```

### Find Duplicates *(new in v1.5.0)*
```python
import anscom
result = anscom.scan("/media-library", find_duplicates=True, silent=True)
print(f"Duplicate groups: {len(result['duplicates'])}")
for group in result["duplicates"]:
    print(f"\nGroup ({len(group)} files):")
    for path in group:
        print(f"  {path}")
```

### Regex Path Filter *(new in v1.5.0)*
```python
import anscom
result = anscom.scan("/codebase", regex_filter=r"/tests/.*\.py$", silent=True)
print(f"Matched test files: {result['total_files']}")
```

### Byte Totals *(new in v1.5.3)*
```python
import anscom
result = anscom.scan("/mnt/storage", with_size=True, silent=True)

print(result["total_size"])        # total bytes across every counted file
print(result["category_sizes"])    # {'Videos': 365..., 'Code/Source': 2..., ...}
print(result["extension_sizes"])   # {'mkv': 225..., 'mp4': 139..., 'py': 8..., ...}
```

Counts tell you *how many*; this tells you *how much*. Both describe exactly the same set of files — every filter is applied before a size is added.

### All exports in one scan pass
```python
import anscom
anscom.scan(
    ".",
    show_tree=True,
    max_depth=20,
    export_json="results.json",
    export_tree="tree.txt",
    export_csv="inventory.csv",
    return_files=True,
    largest_n=20,
    find_duplicates=True,
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

### `anscom.scan(path='.', max_depth=6, show_tree=False, workers=0, min_size=0, extensions=None, callback=None, silent=False, ignore_junk=False, export_json=None, export_tree=None, return_files=False, export_csv=None, largest_n=0, find_duplicates=False, regex_filter=None, with_size=False)`

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `path` | `str` | `'.'` (CWD) | Target directory. Optional — omit it, or pass `""` / `None`, to scan the current working directory (the Unix `pwd`). Relative (`.`) or absolute (`C:\Data`). |
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
| `return_files` *(new in v1.5.0)* | `bool` | `False` | When `True`, the result dict gains a `"files"` key containing a list of dicts (`path`, `size`, `ext`, `category`, `mtime`) — one per file. |
| `export_csv` *(new in v1.5.0)* | `str` | `None` | If a file path is provided, writes a UTF-8 CSV with columns `path,size,ext,category,mtime`. RFC 4180-compliant quoting. Zero dependencies. |
| `largest_n` *(new in v1.5.0)* | `int` | `0` | If > 0, the result dict gains a `"largest_files"` key with the top-N largest files by size. Implemented via per-thread min-heap — O(log N) per file, no extra pass. |
| `find_duplicates` *(new in v1.5.0)* | `bool` | `False` | If `True`, the result dict gains a `"duplicates"` key: a list of groups, each a list of paths sharing identical content. Two-phase: size bucket → CRC32 of first 4KB. |
| `regex_filter` *(new in v1.5.0)* | `str` | `None` | A regex pattern. Only files whose full path matches are counted, categorized, and tracked. Uses POSIX `regexec` on Linux/macOS (no GIL); Python `re` fallback on Windows. |
| `with_size` *(new in v1.5.3)* | `bool` | `False` | If `True`, the result dict gains `total_size`, `category_sizes` and `extension_sizes` — byte totals alongside the counts. Opt-in: on Linux it costs one extra `stat()` per file (free on Windows, and free on macOS/BSD). |

**Returns:** `dict`

| Key | Type | Always? | Description |
|---|---|---|---|
| `total_files` | `int` | ✓ | Files that passed all filters and were categorized |
| `scan_errors` | `int` | ✓ | Paths that could not be opened |
| `duration_seconds` | `float` | ✓ | Wall-clock elapsed time |
| `scanned_path` *(new in v1.5.1)* | `str` | ✓ | Absolute path that was actually walked (resolved from `path`) |
| `categories` | `dict[str, int]` | ✓ | All 9 categories always present |
| `extensions` | `dict[str, int]` | ✓ | Only extensions with count > 0 |
| `files` | `list[dict]` | `return_files=True` | Per-file records: `path`, `size`, `ext`, `category`, `mtime` |
| `largest_files` | `list[dict]` | `largest_n > 0` | Top-N files by size: `path`, `size` |
| `duplicates` | `list[list[str]]` | `find_duplicates=True` | Groups of paths sharing identical content (size + CRC32) |
| `total_size` *(new in v1.5.3)* | `int` | `with_size=True` | Total bytes across every file that was counted |
| `category_sizes` *(new in v1.5.3)* | `dict[str, int]` | `with_size=True` | Bytes per category — all 9 keys present, mirroring `categories` |
| `extension_sizes` *(new in v1.5.3)* | `dict[str, int]` | `with_size=True` | Bytes per extension — only non-zero entries, mirroring `extensions` |

### `anscom.pwd()` *(new in v1.5.1)*

Returns the current working directory as an absolute path string — the Unix `pwd`, as a value (never printed). Takes no arguments. Raises `OSError` if the working directory cannot be read.

```python
import anscom
root = anscom.pwd()        # e.g. '/home/user/project'
anscom.scan(root)          # feed it straight back in
```

### `anscom.about()` *(new in v1.5.2)*

Prints the anscom mascot — **Chuhi**, the house mouse of a Kolkata apartment — as ASCII art, followed by a short credit line. Takes no arguments and returns `None`. The art is about 82 columns wide, so a reasonably wide terminal shows it best.

```python
import anscom
anscom.about()             # meet Chuhi
```

---

## Export Features

### export_json — Native, Zero Dependencies

Export the full scan result as a formatted JSON file. Uses Python's built-in `json` module internally — no `pip install` required. The JSON file contains **all** keys present in the returned dict, including optional v1.5.0 keys (`files`, `largest_files`, `duplicates`) and the v1.5.3 byte-total keys (`total_size`, `category_sizes`, `extension_sizes`) when those features are enabled in the same call.

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
    "scanned_path": "/home/user/project",
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

### export_csv — Per-File Inventory *(new in v1.5.0)*

Writes a per-file UTF-8 CSV with columns `path,size,ext,category,mtime`. RFC 4180-compliant quoting (paths with embedded quotes are correctly escaped). Zero external dependencies.

```python
import anscom
anscom.scan(".", export_csv="inventory.csv")
```

Loading downstream:

```python
# With pandas
import pandas as pd
df = pd.read_csv("inventory.csv")
print(df.groupby("category")["size"].sum().sort_values(ascending=False))

# Convert to Excel via pandas
df.to_excel("report.xlsx", index=False)

# Standard library only
import csv
with open("inventory.csv", newline="", encoding="utf-8") as f:
    for row in csv.DictReader(f):
        print(row["path"], row["size"])
```

### All exports in one pass

```python
import anscom

anscom.scan(
    "/mnt/nas-storage",
    max_depth=20,
    show_tree=True,
    silent=True,
    ignore_junk=True,
    export_json="audit.json",
    export_tree="tree.txt",
    export_csv="inventory.csv",
    return_files=True,
    largest_n=50,
    find_duplicates=True,
)
```

One scan pass. Multiple output files. Full in-memory results. No re-scanning.

---

## v1.5.3 Highlights

### `with_size` — bytes, not just counts

Counts answer *how many files*. They do not answer *where the disk went*. A folder with 8,900 code files and 1,240 videos looks code-heavy by count and is overwhelmingly video by bytes:

```python
import anscom

r = anscom.scan("/mnt/nas", with_size=True, silent=True)

for cat in r["categories"]:
    n, b = r["categories"][cat], r["category_sizes"][cat]
    if n:
        print(f"{cat:16s} {n:>9,} files  {b/1024**3:>9.2f} GB  {b/n/1024:>9.1f} KB avg")

# Code/Source        8,912 files       2.06 GB      242.3 KB avg
# Videos             1,240 files     340.11 GB   287654.2 KB avg
#   ← 7x fewer files, 165x more disk
```

`largest_n` already shows the biggest individual files, but disk rarely fills up from ten large files — it fills up from fifty thousand small ones. Category and extension totals are what close that gap.

### Counts and bytes always describe the same files

Every filter (`extensions`, `regex_filter`, `min_size`) is applied before a size is accumulated, so `total_size` is always the true sum of exactly the `total_files` files that were counted — never a superset.

### Why it's opt-in

On Linux the scanner reads directory entries with `getdents64` and uses `d_type` to identify regular files without ever calling `stat()`. Byte totals need `st_size`, so `with_size=True` adds one `fstatat()` per file — measurable on a multi-million-file tree. Leave it off and the count-only hot path is byte-for-byte the v1.5.2 one.

| Platform | Extra cost with `with_size=True` |
|---|---|
| Linux | One `fstatat()` per file — unless another feature already needs the size |
| Windows | None — size already arrives in `WIN32_FIND_DATAW` |
| macOS / BSD | None — the backend already calls `lstat` |

If `min_size`, `largest_n`, `return_files`, `export_csv` or `find_duplicates` is already on, the size has been fetched anyway and `with_size` reuses it for free.

### Exports

The three keys flow into `export_json` automatically — no new export parameter:

```python
anscom.scan("/data", with_size=True, export_json="report.json", silent=True)
```

`export_csv` is unchanged: it already carries a per-file `size` column, and aggregate totals are deliberately kept out so the file stays a clean rectangular table `pandas.read_csv` can load without special handling. The printed report is unchanged too — byte totals are returned as data, not printed.

Without the flag, the result dict has the identical shape it had in v1.5.2 — the three keys are simply absent. Every existing script runs unchanged.

---

## v1.5.2 Highlights

### `anscom.about()` — meet Chuhi

A small, fully backward-compatible release: it adds a mascot and nothing else. No scan behavior, call signature, or output format changed.

```python
import anscom
anscom.about()             # prints Chuhi + a short credit line, returns None
```

**Chuhi** is the anscom mascot, named after the house mouse of a Kolkata apartment. The art is original and embedded byte-for-byte in the C extension.

---

## v1.5.1 Highlights

### `anscom.pwd()` and no-argument `scan()`

Both return values — nothing is printed, so results stay composable.

```python
import anscom

# pwd() — the current working directory as an absolute string
root = anscom.pwd()               # e.g. '/home/user/project'

# scan() with no path scans the current working directory
result = anscom.scan(silent=True)
print(result["scanned_path"])     # → '/home/user/project'
```

### `scanned_path` — Always-Present Resolved Root

Whatever `path` you pass (or omit), the result dict always includes `scanned_path`: the absolute path that was actually walked, resolved via `realpath` on Linux/macOS and `_fullpath` on Windows. If a path cannot be resolved (missing or unreadable), it falls back to the raw input so the key is always present.

```python
r = anscom.scan("../logs", silent=True)
print(r["scanned_path"])          # resolved absolute path, not '../logs'
```

This release also fixes Python reference leaks in the result-dict construction and no longer pre-allocates the per-thread file buffer on count-only scans, so a plain `anscom.scan()` now does zero heap work on the hot path.

---

## v1.5.0 Feature Deep Dive

### `return_files` — Per-File List in the Result Dict

When `True`, the returned dict gains a `"files"` key — a Python list of dicts, one per scanned file.

| Field | Type | Description |
|---|---|---|
| `path` | `str` | Full absolute path to the file |
| `size` | `int` | File size in bytes |
| `ext` | `str` | Lowercase extension (no dot), empty if unrecognized |
| `category` | `str` | One of the 9 category names |
| `mtime` | `int` | Unix timestamp of last modification |

```python
result = anscom.scan("/project", return_files=True, silent=True)

# Filter in Python
large_code = [
    f for f in result["files"]
    if f["category"] == "Code/Source" and f["size"] > 50_000
]

# Sort by size descending
by_size = sorted(result["files"], key=lambda f: f["size"], reverse=True)
print("Largest file:", by_size[0]["path"])
```

`len(result["files"]) == result["total_files"]` is always true.

### `largest_n` — Top-N Largest Files

Per-thread min-heap of capacity N. O(log N) cost per file, no extra pass, no full sort. Heaps are merged after all threads join.

```python
result = anscom.scan("/mnt/storage", largest_n=20, silent=True)

for f in result["largest_files"]:
    gb = f["size"] / (1024 ** 3)
    print(f"{gb:8.2f} GB  {f['path']}")
```

The printed report also gains a section:

```
=== TOP 20 LARGEST FILES ===========================
  1073741824 bytes : /data/backup/archive.tar.gz
   536870912 bytes : /data/media/4k_reel.mkv
...
===================================================
```

### `find_duplicates` — Two-Phase Duplicate Detection

1. **Size bucketing** — files sorted by size. Files with a unique size are skipped — zero I/O.
2. **CRC32 checksum** — for each same-size group ≥2 files, the first 4096 bytes of each are read and CRC32 is computed. Matching CRC32s are reported as duplicates.

```python
result = anscom.scan("/media-library", find_duplicates=True, silent=True)
print(f"Duplicate groups: {len(result['duplicates'])}")

for group in result["duplicates"]:
    for path in group:
        print(f"  {path}")
```

Combine with `return_files=True` to compute reclaimable space:

```python
result = anscom.scan(
    "/mnt/archive",
    find_duplicates=True,
    return_files=True,
    silent=True,
)

size_map = {f["path"]: f["size"] for f in result["files"]}
wasted = sum(
    sum(size_map.get(p, 0) for p in group[1:])
    for group in result["duplicates"]
)
print(f"Reclaimable: {wasted / (1024**3):.2f} GB")
```

### `regex_filter` — Path Pattern Filter

A regex pattern. When set, **only** files whose full absolute path matches are counted, categorized, and tracked.

* **Linux / macOS:** Compiled with POSIX `regcomp(REG_EXTENDED | REG_NOSUB)`, matched with `regexec` — **no GIL acquisition**, runs fully in C inside the worker threads.
* **Windows:** Falls back to Python's `re` module (GIL acquired per file). For large Windows scans, prefer the `extensions` whitelist for zero-GIL filtering.

The pattern is also compiled with Python's `re.compile` before the scan. Invalid patterns raise `ValueError` immediately — no scan is started.

```python
# Only test files
result = anscom.scan("/repo", regex_filter=r"test_.*\.py$", silent=True)

# Only files inside src/ directories
result = anscom.scan("/project", regex_filter=r"/src/", silent=True)
```

---

## Smart Filter (Auto-Ignored when `ignore_junk=True`)

`.git`, `node_modules`, `.venv`, `venv`, `build`, `dist`, `__pycache__`, `temp`, `tmp`, `.cache`, `.pytest_cache`, `.mypy_cache`, `site-packages`, etc.

---

## Report Output

After scanning, anscom prints:
- Summary Report
- Detailed Extension Breakdown
- Top-N Largest Files *(when `largest_n > 0`)*
- Duplicates Summary *(when `find_duplicates=True`)*

---

## Performance Architecture

- Direct OS syscalls (`FindFirstFileW` on Windows, `getdents64` on Linux, `readdir` on POSIX)
- FNV-1a open-addressing hash table for O(1) extension lookup
- Per-thread statistics accumulation — no shared locks during file counting
- Slab path allocator — one `malloc` per thread before scan, zero allocations during traversal
- Hardware atomic progress counter (`__sync_fetch_and_add` / `InterlockedExchangeAdd64`)
- Hybrid parallel/inline dispatch — queue at shallow depths, inline recursion at depth
- Per-thread min-heap for `largest_n` — O(log N) per file, lock-free
- Per-thread `FileInfo` array pre-allocated at 65,536 entries **only when a file-collecting feature is enabled** (`return_files` / `export_csv` / `find_duplicates`) — plain count-only scans do zero heap work on the hot path
- POSIX `regexec` for `regex_filter` on Linux/macOS — fully GIL-free regex matching inside worker threads
- Per-thread byte accumulators for `with_size` (`total_size`, `cat_sizes[9]`, `ext_sizes[170+]`) living in the same lock-free `ScanStats` struct as the counts, merged in their own serial pass after the threads join — and never touched at all when the flag is off

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

Machine learning projects often contain datasets with thousands of files spread across nested directories. anscom lets you verify a dataset's composition before kicking off a training run.

```python
anscom.scan("datasets/")
```

---

### Storage Review

Understand what's taking up space in a directory — whether it's archives piling up, temporary files accumulating, or an unexpected file type dominating.

```python
anscom.scan("/data/storage", max_depth=10)
```

---

### Large Directory Exploration

Modern development environments can contain tens of thousands of files once dependencies are included. anscom focuses only on what matters by automatically skipping known junk directories.

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

anscom can be used as a backend for tools that need directory statistics:

- Code statistics dashboards
- Project analysis and reporting tools
- Automated repository summaries
- Build pipeline checks

---

## Larger-Scale Use Cases

### Storage Breakdown by Category

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

### Pre-Migration Directory Inventory

```python
import anscom

result = anscom.scan(
    "/legacy-server/data",
    max_depth=30,
    silent=True,
    return_files=True,
    export_json="pre_migration_audit.json",
    export_csv="pre_migration_inventory.csv",
)

print(f"Inventory complete: {result['total_files']:,} files recorded.")
```

### Build Pipeline File Check

```python
import anscom, sys

result = anscom.scan("./repo", silent=True, ignore_junk=True)

if result["categories"]["Executables"] > 0:
    print(f"Check failed: {result['categories']['Executables']} executable files found in repo.")
    sys.exit(1)

print("File composition check passed.")
```

### Storage Reclamation via Duplicate Detection *(new in v1.5.0)*

```python
import anscom

result = anscom.scan(
    "/mnt/media-archive",
    find_duplicates=True,
    return_files=True,
    workers=16,
    silent=True,
)

size_map = {f["path"]: f["size"] for f in result["files"]}
wasted = sum(
    sum(size_map.get(p, 0) for p in group[1:])
    for group in result["duplicates"]
)

print(f"Duplicate groups : {len(result['duplicates'])}")
print(f"Reclaimable      : {wasted / (1024**3):.2f} GB")
```

### Where the Disk Actually Went *(new in v1.5.3)*

```python
import anscom

r = anscom.scan("/mnt/nas", with_size=True, ignore_junk=True,
                workers=16, silent=True)

def human(n):
    for unit in ("B", "KB", "MB", "GB", "TB"):
        if n < 1024 or unit == "TB":
            return f"{n:.1f} {unit}"
        n /= 1024

print(f"{r['total_files']:,} files, {human(r['total_size'])} total\n")

for cat, size in sorted(r["category_sizes"].items(),
                        key=lambda kv: kv[1], reverse=True):
    if not size:
        continue
    share = size / r["total_size"] * 100
    print(f"{cat:16s} {r['categories'][cat]:>9,} files  "
          f"{human(size):>10s}  {share:5.1f}%")

print("\nTop extensions by size:")
for ext, size in sorted(r["extension_sizes"].items(),
                        key=lambda kv: kv[1], reverse=True)[:10]:
    print(f"  .{ext:<10s} {human(size):>10s}")
```

### Full Directory Tree Capture (Very Large Volumes)

anscom imposes no internal limit on the volume it will traverse. Memory usage is bounded by the work queue capacity (131,072 directory slots) and the per-thread path slabs — neither grows with the total number of files on disk.

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
| **1.5.3** | 24 August 2026 | **Byte accounting.** Added `with_size` — an opt-in flag that adds `total_size`, `category_sizes` and `extension_sizes` to the result dict, so counts are joined by bytes. The three keys are written to `export_json` automatically; `export_csv` and the printed report are unchanged. Opt-in because byte totals need `st_size`: on Linux that is one extra `fstatat()` per file, skipped when another feature has already fetched it, and free on Windows and macOS/BSD. Purely additive — with the flag off, the result dict has the identical shape it had in v1.5.2 and every existing script runs unchanged. |
| **1.5.2** | 29 July 2026 | **Mascot.** Added `about()` — prints **Chuhi**, the anscom mascot (the house mouse of a Kolkata apartment), as original ASCII art with a short credit line, then returns `None`. Fully backward-compatible: no scan behavior, call signature, or output format changed. |
| **1.5.1** | 28 July 2026 | **Ergonomics & correctness.** Added `pwd()` (returns the current working directory as an absolute string — the Unix `pwd`, as a value), no-argument `scan()` (defaults to the current working directory), and `scanned_path` in every result dict (absolute path actually walked). Fixed Python reference leaks in result-dict construction (`PyDict_SetItemString` / `PyList_Append` do not steal references). The per-thread file buffer is no longer pre-allocated on count-only scans — a plain `anscom.scan()` now performs zero heap work on the hot path. Fully backward-compatible: every v1.5.0 script runs unchanged. |
| **1.5.0** | 09 April 2026 | **Major feature release.** Added `return_files` (per-file list in result dict), `export_csv` (UTF-8 CSV inventory, RFC 4180-compliant, zero deps), `largest_n` (top-N largest files via per-thread min-heap, O(log N) per file), `find_duplicates` (two-phase size + CRC32 detection, zero I/O for unique-size files), `regex_filter` (POSIX `regexec` on Linux/macOS for GIL-free path filtering, Python `re` fallback on Windows). Per-thread `FileInfo` array pre-allocated at 65,536 entries — zero reallocations for typical scans. All new features are strictly opt-in: default `anscom.scan(".")` runs the identical hot path as v1.3.0. Removed `export_excel` (was crashing on Windows due to `openpyxl` `Workbook.read_only` exception); use `export_csv` + `pandas.to_excel()` instead. |
| **1.3.0** | 15 March 2026 | **Export release.** Added `export_json` (native, zero dependencies), `export_tree` (DFS tree to `.txt`), and `export_excel` (structured `.xlsx` with openpyxl). MSVC Windows compiler compatibility fix for `uint64_t` / `stdint.h`. |
| **1.0.0** | 13 March 2026 | **Major release.** Large-volume directory scanning. Multi-threaded worker pool, `getdents64` direct syscall backend (Linux), FNV-1a hash table for O(1) extension lookup, per-thread statistics with zero shared locks, slab path allocator, extension whitelisting, silent mode, min_size filter, and live callback interface. |
| 0.6.0 | Jan 15, 2026 | Added features like data tree and many more |
| 0.5.0 | Jan 15, 2026 | — |
| 0.4.0 | Jan 15, 2026 | Release of the basic version with the concept of anscom |

---
<img width="1073" height="799" alt="image" src="https://github.com/user-attachments/assets/98424cad-b7a5-40aa-9e5d-d5abc19498a5" />
<img width="840" height="541" alt="image" src="https://github.com/user-attachments/assets/d0a84577-9c8d-478c-9b04-b900cc191b62" />
<img width="1235" height="758" alt="image" src="https://github.com/user-attachments/assets/a6c727d6-58f9-4c6f-bb64-88092d96b249" />

---

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you'd like to change.

---

## Totally open to suggestions.

## anscom official web

https://anscomqs.github.io/anscom/

---

## License

MIT License
