# AnsCom NFIE — Native Filesystem Intelligence Engine (Python C Extension)
[![PyPI version](https://badge.fury.io/py/anscom.svg)](https://pypi.org/project/anscom/)

`pip install anscom`

A globally published, native C-based Python extension for filesystem intelligence and large-scale directory analysis.

---

**AnsCom NFIE** is a high-performance, native C extension for Python designed for systems engineers, data analysts, and developers. It recursively scans directories, categorizes files by type, and generates detailed statistical reports with Constant-memory streaming traversal.
Unlike standard Python scanners (like `os.walk`), AnsCom is built in pure **C89**, allowing it to process massive filesystems instantly without flooding your terminal or exhausting system memory.

---

##  Key Features

* ** Native C Speed:** Written in pure C for maximum I/O throughput.
* ** Smart Exclusion System:** Automatically hard-ignores junk directories (`node_modules`, `.venv`, `.git`, `__pycache__`, `build`, etc.).
* ** Zero-Memory Streaming:** Processes files one by one in streaming fashion. Will never crash with `MemoryError`.
* ** Visual Tree Map:** Optional diagrammatic tree view.
* ** Detailed Analytics:**
  * Category Summary: Groups files by Code, Images, Video, Audio, Archives, Executables, etc.
  * Extension Breakdown: Exact counts for every file extension (`.py`, `.c`, `.png`, etc.).
* ** Live Progress:** In-place progress counter.
* ** Safety Circuit Breaker:** Configurable recursion depth limit (Default: 6).

---

##  Installation
```bash
pip install anscom
```

---

##  Usage Guide

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

##  API Reference

### `anscom.scan(path, max_depth=6, show_tree=False)`

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `path` | str | Required | Target directory |
| `max_depth` | int | 6 | Recursion depth |
| `show_tree` | bool | False | Enable tree view |

**Returns:** `int` - total files scanned.

---

##  Smart Filter (Auto-Ignored)

`.git`, `node_modules`, `.venv`, `venv`, `build`, `dist`, `__pycache__`, `temp`, `tmp`, `.cache`, `.pytest_cache`, `.mypy_cache`, `site-packages`, etc.

---

##  Report Output

After scanning, AnsCom prints:
- Summary Report
- Detailed Extension Breakdown

---

##  Performance Architecture

- Direct OS syscalls (`FindFirstFileW` / `readdir`)
- Binary search categorization
- Buffered terminal output.

---
# Demo
<img width="280" height="879" alt="image" src="https://github.com/user-attachments/assets/c23cc5ec-3ae9-43e5-8580-5f397bd413a2" />
<img width="649" height="146" alt="image" src="https://github.com/user-attachments/assets/96bcf704-224b-47ac-bb32-e8976a914776" />


# Anscom

**Fast, native filesystem analysis for developers, data scientists, and system administrators.**

Anscom is a high-performance directory scanner written in C with a Python interface. It instantly profiles any directory — counting files, categorizing types, and surfacing meaningful project structure — while automatically filtering out dependency noise like `node_modules` and `__pycache__`.

---

## Features

- **Instant file-type breakdown** — source code, images, documents, archives, and more
- **Language distribution** — understand what your codebase is made of at a glance
- **Noise filtering** — automatically skips `.git`, `node_modules`, `.venv`, `build`, `dist`, and other dependency/cache directories
- **Configurable depth** — control how deep the scan recurses
- **Native C performance** — significantly faster than pure Python filesystem tools

---

## Installation

```bash
pip install anscom
```

---

## Quick Start

```python
import anscom

anscom.scan(".")
```

**Example output:**

```
=== SUMMARY REPORT ===
Code Files:    120
Images:         15
Documents:       8
Total Files:   143
```

---

## Use Cases

### 🗂️ Codebase Analysis

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

### 🧠 Dataset Inspection

Machine learning projects often contain datasets with thousands of files spread across nested directories. Anscom lets you verify a dataset's composition before kicking off a training run.

```python
anscom.scan("datasets/")
```

Surfaces:
- Count of images, videos, or audio files
- Dataset folder organization
- File distribution across subdirectories

---

### 💾 Storage Auditing

Understand what's consuming space in a directory — whether it's archives piling up, temporary files accumulating, or an unexpected file type dominating.

```python
anscom.scan("/data/storage", max_depth=10)
```

Useful for answering:
- What file types dominate this directory?
- Are there too many archives or temp files?
- How is storage distributed across categories?

---

### 🌲 Large Filesystem Exploration

Modern development environments can contain tens of thousands of files once dependencies are included. Anscom focuses only on what matters by automatically skipping:

| Ignored Directory | Reason |
|---|---|
| `.git` | Version control internals |
| `node_modules` | JavaScript dependencies |
| `.venv` | Python virtual environments |
| `__pycache__` | Python bytecode cache |
| `build` | Compiled output |
| `dist` | Distribution artifacts |

This keeps results focused on your actual project files.

---

### 🔧 Development Tool Integration

Anscom is designed to be embedded as a backend engine for tools that need filesystem analytics:

- Code statistics dashboards
- Project analysis and reporting tools
- Automated repository audits
- DevOps pipeline checks and gates

Because Anscom runs in native C, it provides significantly faster scanning performance compared to pure Python implementations — making it suitable even in performance-sensitive pipelines.

---

## API Reference

### `anscom.scan(path, max_depth=None)`

| Parameter | Type | Default | Description |
|---|---|---|---|
| `path` | `str` | — | Directory path to scan |
| `max_depth` | `int` | `None` | Maximum recursion depth. `None` means unlimited. |

---

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you'd like to change.

---

## License

[MIT](LICENSE)
## License

MIT License






