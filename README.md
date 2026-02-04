# AnsCom NFIE — Native Filesystem Intelligence Engine (Python C Extension)
[![PyPI version](https://badge.fury.io/py/anscom.svg)](https://pypi.org/project/anscom/)

`pip install anscom`

A globally published, native C-based Python extension for filesystem intelligence and large-scale directory analysis.

---

**AnsCom NFIE** is a high-performance, native C extension for Python designed for systems engineers, data analysts, and developers. It recursively scans directories, categorizes files by type, and generates detailed statistical reports with zero memory overhead.

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

## License

MIT License




