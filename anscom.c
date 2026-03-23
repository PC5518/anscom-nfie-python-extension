/*
 * anscom.c
 *
 * Version: v1.3.0 (Tree Structure & DFS Fix) flagship version
 * Description: High-performance, multi-threaded recursive file scanner.
 *              Fixed Deep-Tree generation and added file tracking.
 * Compilation: python setup.py build_ext --inplace
 * update fix_2/21/2026: includes logic to ignore junk or corrupted files  (new version available on PyPl)
 * update: new  version released:-> 13 March 2026 : added features which can analyze directories at terabyte scale under seconds without any lag! via n.log(n). making the most powerful version than ever before !
 * update v1.3.0: added export_json, export_excel, and export_tree features
 */


#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <sys/stat.h>

/* Cross-platform & OS-specific headers */
#ifdef _WIN32
    #include <windows.h>
    #define PATH_SEP '\\'
    #define PATH_MAX 4096
    typedef HANDLE os_thread_t;
    typedef CRITICAL_SECTION os_mutex_t;
    typedef CONDITION_VARIABLE os_cond_t;
    static void mutex_init(os_mutex_t *m) { InitializeCriticalSection(m); }
    static void mutex_lock(os_mutex_t *m) { EnterCriticalSection(m); }
    static void mutex_unlock(os_mutex_t *m) { LeaveCriticalSection(m); }
    static void cond_init(os_cond_t *c) { InitializeConditionVariable(c); }
    static void cond_wait(os_cond_t *c, os_mutex_t *m) { SleepConditionVariableCS(c, m, INFINITE); }
    static void cond_broadcast(os_cond_t *c) { WakeAllConditionVariable(c); }
    static void cond_signal(os_cond_t *c) { WakeConditionVariable(c); }
    #define ATOMIC_ADD(ptr, val) InterlockedExchangeAdd64((LONG64 volatile *)(ptr), (val))
    #define ATOMIC_READ(ptr) InterlockedOr64((LONG64 volatile *)(ptr), 0)
#else
    #include <pthread.h>
    #include <dirent.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <time.h>
    #define PATH_SEP '/'
    #if defined(__linux__) && defined(__NR_getdents64)
        #include <sys/syscall.h>
        #define USE_GETDENTS64 1
        struct linux_dirent64 {
            uint64_t       d_ino;
            int64_t        d_off;
            unsigned short d_reclen;
            unsigned char  d_type;
            char           d_name[];
        };
    #endif
    typedef pthread_t os_thread_t;
    typedef pthread_mutex_t os_mutex_t;
    typedef pthread_cond_t os_cond_t;
    static void mutex_init(os_mutex_t *m) { pthread_mutex_init(m, NULL); }
    static void mutex_lock(os_mutex_t *m) { pthread_mutex_lock(m); }
    static void mutex_unlock(os_mutex_t *m) { pthread_mutex_unlock(m); }
    static void cond_init(os_cond_t *c) { pthread_cond_init(c, NULL); }
    static void cond_wait(os_cond_t *c, os_mutex_t *m) { pthread_cond_wait(c, m); }
    static void cond_broadcast(os_cond_t *c) { pthread_cond_broadcast(c); }
    static void cond_signal(os_cond_t *c) { pthread_cond_signal(c); }
    #define ATOMIC_ADD(ptr, val) __sync_fetch_and_add((ptr), (val))
    #define ATOMIC_READ(ptr) __sync_fetch_and_add((ptr), 0)
#endif

/* -------------------------------------------------------------------------
   Configuration & Constants
   ------------------------------------------------------------------------- */

static const char* IGNORE_DIRS[] = {
    ".venv", "venv", "env", "site-packages", "node_modules", "bower_components",
    ".git", ".svn", ".hg", "__pycache__", ".idea", ".vscode", "build", "dist",
    "target", "temp", "tmp", ".cache", ".pytest_cache", ".mypy_cache"
};
static const int IGNORE_COUNT = sizeof(IGNORE_DIRS) / sizeof(char*);

typedef enum {
    CAT_CODE = 0, CAT_DOCUMENT, CAT_IMAGE, CAT_VIDEO, CAT_AUDIO,
    CAT_ARCHIVE, CAT_EXECUTABLE, CAT_SYSTEM, CAT_UNKNOWN, CAT_COUNT
} FileCategory;

static const char* CAT_NAMES[CAT_COUNT] = {
    "Code/Source", "Documents", "Images", "Videos", "Audio",
    "Archives", "Executables", "System/Config", "Other/Unknown"
};

typedef struct {
    char ext[16];
    FileCategory category;
} ExtDef;

static const ExtDef EXTENSION_TABLE[] = {
    {"3g2", CAT_VIDEO}, {"3gp", CAT_VIDEO}, {"7z", CAT_ARCHIVE}, {"aac", CAT_AUDIO},
    {"accdb", CAT_DOCUMENT}, {"ai", CAT_IMAGE}, {"aif", CAT_AUDIO}, {"apk", CAT_ARCHIVE},
    {"app", CAT_EXECUTABLE}, {"asf", CAT_VIDEO}, {"asm", CAT_CODE}, {"asp", CAT_CODE},
    {"aspx", CAT_CODE}, {"avi", CAT_VIDEO}, {"avif", CAT_IMAGE}, {"awk", CAT_CODE},
    {"bak", CAT_SYSTEM}, {"bas", CAT_CODE}, {"bat", CAT_CODE}, {"bin", CAT_EXECUTABLE},
    {"bmp", CAT_IMAGE}, {"bz2", CAT_ARCHIVE}, {"c", CAT_CODE}, {"cab", CAT_ARCHIVE},
    {"cbr", CAT_ARCHIVE}, {"cc", CAT_CODE}, {"cfg", CAT_SYSTEM}, {"class", CAT_EXECUTABLE},
    {"cmd", CAT_CODE}, {"cnf", CAT_SYSTEM}, {"com", CAT_EXECUTABLE}, {"conf", CAT_SYSTEM},
    {"cpp", CAT_CODE}, {"cr2", CAT_IMAGE}, {"crt", CAT_SYSTEM}, {"cs", CAT_CODE},
    {"css", CAT_CODE}, {"csv", CAT_DOCUMENT}, {"cue", CAT_AUDIO}, {"cur", CAT_IMAGE},
    {"dat", CAT_SYSTEM}, {"db", CAT_SYSTEM}, {"dbf", CAT_DOCUMENT}, {"deb", CAT_ARCHIVE},
    {"dll", CAT_EXECUTABLE}, {"dmg", CAT_ARCHIVE}, {"doc", CAT_DOCUMENT}, {"docx", CAT_DOCUMENT},
    {"dot", CAT_DOCUMENT}, {"dotx", CAT_DOCUMENT}, {"drw", CAT_IMAGE}, {"dxf", CAT_IMAGE},
    {"ebook", CAT_DOCUMENT}, {"elf", CAT_EXECUTABLE}, {"eml", CAT_DOCUMENT}, {"env", CAT_SYSTEM},
    {"eps", CAT_IMAGE}, {"epub", CAT_DOCUMENT}, {"exe", CAT_EXECUTABLE}, {"flac", CAT_AUDIO},
    {"flv", CAT_VIDEO}, {"fnt", CAT_SYSTEM}, {"fon", CAT_SYSTEM}, {"fth", CAT_CODE},
    {"gif", CAT_IMAGE}, {"git", CAT_SYSTEM}, {"gitignore", CAT_SYSTEM}, {"go", CAT_CODE},
    {"gpg", CAT_SYSTEM}, {"gradle", CAT_CODE}, {"groovy", CAT_CODE}, {"gz", CAT_ARCHIVE},
    {"h", CAT_CODE}, {"heic", CAT_IMAGE}, {"heif", CAT_IMAGE}, {"hpp", CAT_CODE},
    {"htm", CAT_CODE}, {"html", CAT_CODE}, {"hwp", CAT_DOCUMENT}, {"ico", CAT_IMAGE},
    {"ics", CAT_DOCUMENT}, {"iff", CAT_IMAGE}, {"img", CAT_ARCHIVE}, {"indd", CAT_DOCUMENT},
    {"ini", CAT_SYSTEM}, {"iso", CAT_ARCHIVE}, {"jar", CAT_ARCHIVE}, {"java", CAT_CODE},
    {"jpeg", CAT_IMAGE}, {"jpg", CAT_IMAGE}, {"js", CAT_CODE}, {"json", CAT_CODE},
    {"jsp", CAT_CODE}, {"jsx", CAT_CODE}, {"key", CAT_DOCUMENT}, {"kt", CAT_CODE},
    {"kts", CAT_CODE}, {"less", CAT_CODE}, {"log", CAT_SYSTEM}, {"lua", CAT_CODE},
    {"m", CAT_CODE}, {"m3u", CAT_AUDIO}, {"m4a", CAT_AUDIO}, {"m4v", CAT_VIDEO},
    {"mak", CAT_CODE}, {"md", CAT_DOCUMENT}, {"mdb", CAT_DOCUMENT}, {"mid", CAT_AUDIO},
    {"midi", CAT_AUDIO}, {"mkv", CAT_VIDEO}, {"mm", CAT_CODE}, {"mobi", CAT_DOCUMENT},
    {"mov", CAT_VIDEO}, {"mp3", CAT_AUDIO}, {"mp4", CAT_VIDEO}, {"mpeg", CAT_VIDEO},
    {"mpg", CAT_VIDEO}, {"msi", CAT_EXECUTABLE}, {"nef", CAT_IMAGE}, {"numbers", CAT_DOCUMENT},
    {"obj", CAT_SYSTEM}, {"odp", CAT_DOCUMENT}, {"ods", CAT_DOCUMENT}, {"odt", CAT_DOCUMENT},
    {"ogg", CAT_AUDIO}, {"ogv", CAT_VIDEO}, {"orf", CAT_IMAGE}, {"otf", CAT_SYSTEM},
    {"pages", CAT_DOCUMENT}, {"pak", CAT_ARCHIVE}, {"pas", CAT_CODE}, {"pdf", CAT_DOCUMENT},
    {"pem", CAT_SYSTEM}, {"php", CAT_CODE}, {"pkg", CAT_ARCHIVE}, {"pl", CAT_CODE},
    {"pm", CAT_CODE}, {"png", CAT_IMAGE}, {"ppt", CAT_DOCUMENT}, {"pptx", CAT_DOCUMENT},
    {"ps", CAT_IMAGE}, {"ps1", CAT_CODE}, {"psd", CAT_IMAGE}, {"pub", CAT_DOCUMENT},
    {"py", CAT_CODE}, {"pyc", CAT_SYSTEM}, {"pyd", CAT_EXECUTABLE}, {"pyw", CAT_CODE},
    {"r", CAT_CODE}, {"rar", CAT_ARCHIVE}, {"raw", CAT_IMAGE}, {"rb", CAT_CODE},
    {"reg", CAT_SYSTEM}, {"rm", CAT_VIDEO}, {"rpm", CAT_ARCHIVE}, {"rs", CAT_CODE},
    {"rst", CAT_DOCUMENT}, {"rtf", CAT_DOCUMENT}, {"sass", CAT_CODE}, {"scala", CAT_CODE},
    {"scss", CAT_CODE}, {"sh", CAT_CODE}, {"sln", CAT_CODE}, {"so", CAT_EXECUTABLE},
    {"sql", CAT_CODE}, {"srt", CAT_VIDEO}, {"svg", CAT_IMAGE}, {"swf", CAT_VIDEO},
    {"swift", CAT_CODE}, {"sys", CAT_SYSTEM}, {"tar", CAT_ARCHIVE}, {"tga", CAT_IMAGE},
    {"tgz", CAT_ARCHIVE}, {"tif", CAT_IMAGE}, {"tiff", CAT_IMAGE}, {"tmp", CAT_SYSTEM},
    {"ts", CAT_CODE}, {"tsv", CAT_DOCUMENT}, {"ttf", CAT_SYSTEM}, {"txt", CAT_DOCUMENT},
    {"vb", CAT_CODE}, {"vbox", CAT_SYSTEM}, {"vcd", CAT_ARCHIVE}, {"vcf", CAT_DOCUMENT},
    {"vcxproj", CAT_CODE}, {"vob", CAT_VIDEO}, {"vue", CAT_CODE}, {"wav", CAT_AUDIO},
    {"webm", CAT_VIDEO}, {"webp", CAT_IMAGE}, {"wma", CAT_AUDIO}, {"wmv", CAT_VIDEO},
    {"woff", CAT_SYSTEM}, {"woff2", CAT_SYSTEM}, {"wpd", CAT_DOCUMENT}, {"wps", CAT_DOCUMENT},
    {"wsf", CAT_CODE}, {"xcodeproj", CAT_CODE}, {"xls", CAT_DOCUMENT}, {"xlsm", CAT_DOCUMENT},
    {"xlsx", CAT_DOCUMENT}, {"xml", CAT_CODE}, {"yaml", CAT_CODE}, {"yml", CAT_CODE},
    {"zip", CAT_ARCHIVE}
};
static const int EXTENSION_COUNT = sizeof(EXTENSION_TABLE) / sizeof(ExtDef);

#define HASH_SIZE 512
static int EXT_HASH_TABLE[HASH_SIZE];

/* -------------------------------------------------------------------------
   Thread-Safe Data Structures
   ------------------------------------------------------------------------- */

typedef struct {
    uint64_t total_files;
    uint64_t scan_errors;
    uint64_t cat_counts[CAT_COUNT];
    uint64_t ext_counts[sizeof(EXTENSION_TABLE) / sizeof(ExtDef)];
} ScanStats;

typedef struct {
    int max_depth;
    int show_tree;
    int workers;
    uint64_t min_size;
    PyObject *callback;
    uint8_t *allowed_exts;
    int ignore_junk;
    /* === NEW FEATURE: export_tree === */
    FILE *tree_file;
} ScanConfig;

static volatile uint64_t g_atomic_scanned = 0;

#define MAX_QUEUE 131072
typedef struct {
    char *path;
    int depth;
} WorkItem;

typedef struct {
    WorkItem items[MAX_QUEUE];
    int head, tail, count;
    int active_workers;
    int shutdown;
    os_mutex_t lock;
    os_cond_t cond;
} WorkQueue;

typedef struct {
    ScanStats stats;
    ScanConfig *config;
    WorkQueue *queue;
    char *path_slab;
} ThreadState;

/* -------------------------------------------------------------------------
   Helper Functions
   ------------------------------------------------------------------------- */

static uint32_t hash_ext(const char *str) {
    uint32_t hash = 2166136261u;
    while (*str) {
        hash ^= (uint8_t)tolower(*str++);
        hash *= 16777619u;
    }
    return hash % HASH_SIZE;
}

static int str_iequal(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

static int is_ignored(const char *dirname) {
    for (int i = 0; i < IGNORE_COUNT; i++) {
        if (str_iequal(dirname, IGNORE_DIRS[i])) return 1;
    }
    return 0;
}

static void identify_and_count(const char *filename, ThreadState *ts) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        ts->stats.cat_counts[CAT_UNKNOWN]++;
        ts->stats.total_files++;
        ATOMIC_ADD(&g_atomic_scanned, 1);
        return;
    }
    char ext_lower[32];
    const char *ext_ptr = dot + 1;
    size_t i = 0;
    while (ext_ptr[i] && i < 31) {
        ext_lower[i] = (char)tolower((unsigned char)ext_ptr[i]);
        i++;
    }
    ext_lower[i] = '\0';

    uint32_t idx = hash_ext(ext_lower);
    uint32_t start_idx = idx;
    while (EXT_HASH_TABLE[idx] != -1) {
        int table_idx = EXT_HASH_TABLE[idx];
        if (strcmp(EXTENSION_TABLE[table_idx].ext, ext_lower) == 0) {
            if (ts->config->allowed_exts && !ts->config->allowed_exts[table_idx]) return;
            ts->stats.ext_counts[table_idx]++;
            ts->stats.cat_counts[EXTENSION_TABLE[table_idx].category]++;
            ts->stats.total_files++;
            ATOMIC_ADD(&g_atomic_scanned, 1);
            return;
        }
        idx = (idx + 1) % HASH_SIZE;
        if (idx == start_idx) break;
    }
    if (!ts->config->allowed_exts) {
        ts->stats.cat_counts[CAT_UNKNOWN]++;
        ts->stats.total_files++;
        ATOMIC_ADD(&g_atomic_scanned, 1);
    }
}

static void print_report(const ScanStats *s, double elapsed) {
    int i;
    double pct;

    PySys_WriteStdout("\n=== SUMMARY REPORT ================================\n");
    PySys_WriteStdout("+-----------------+--------------+----------+\n");
    PySys_WriteStdout("| %-15s | %-12s | %-8s |\n", "Category", "Count", "Percent");
    PySys_WriteStdout("+-----------------+--------------+----------+\n");

    if (s->total_files == 0) {
        PySys_WriteStdout("| %-38s |\n", "No files found.");
    } else {
        for (i = 0; i < CAT_COUNT; i++) {
            if (s->cat_counts[i] == 0) continue;
            pct = (double)s->cat_counts[i] / (double)s->total_files * 100.0;
            PySys_WriteStdout("| %-15s | %12" PRIu64 " | %7.2f%% |\n",
                CAT_NAMES[i], s->cat_counts[i], pct);
        }
    }

    PySys_WriteStdout("+-----------------+--------------+----------+\n");
    PySys_WriteStdout("| %-15s | %12" PRIu64 " | %-8s |\n",
        "TOTAL FILES", s->total_files, "100.00%");
    PySys_WriteStdout("+-----------------+--------------+----------+\n");

    if (s->total_files > 0) {
        PySys_WriteStdout("\n=== DETAILED EXTENSION BREAKDOWN ==================\n");
        PySys_WriteStdout("+-----------------+--------------+\n");
        PySys_WriteStdout("| %-15s | %-12s |\n", "Extension", "Count");
        PySys_WriteStdout("+-----------------+--------------+\n");
        for (i = 0; i < EXTENSION_COUNT; i++) {
            if (s->ext_counts[i] > 0) {
                PySys_WriteStdout("| .%-14s | %12" PRIu64 " |\n",
                    EXTENSION_TABLE[i].ext, s->ext_counts[i]);
            }
        }
        PySys_WriteStdout("+-----------------+--------------+\n");
    }

    PySys_WriteStdout("\nTime     : %.4f seconds\n", elapsed);
    PySys_WriteStdout("Errors   : %" PRIu64 " (permission denied / inaccessible)\n",
        s->scan_errors);
    PySys_WriteStdout("===================================================\n\n");
}

/* -------------------------------------------------------------------------
   Queue Operations
   ------------------------------------------------------------------------- */

static int queue_push(WorkQueue *q, const char *path, int depth) {
    mutex_lock(&q->lock);
    if (q->count >= MAX_QUEUE) {
        mutex_unlock(&q->lock);
        return 0;
    }
    size_t len = strlen(path) + 1;
    q->items[q->tail].path = (char*)malloc(len);
    if (q->items[q->tail].path) memcpy(q->items[q->tail].path, path, len);
    q->items[q->tail].depth = depth;
    q->tail = (q->tail + 1) % MAX_QUEUE;
    q->count++;
    cond_signal(&q->cond);
    mutex_unlock(&q->lock);
    return 1;
}

static int queue_pop(WorkQueue *q, char **out_path, int *out_depth) {
    mutex_lock(&q->lock);
    while (q->count == 0 && !q->shutdown) {
        cond_wait(&q->cond, &q->lock);
    }
    if (q->count == 0 && q->shutdown) {
        mutex_unlock(&q->lock);
        return 0;
    }
    *out_path = q->items[q->head].path;
    *out_depth = q->items[q->head].depth;
    q->head = (q->head + 1) % MAX_QUEUE;
    q->count--;
    q->active_workers++;
    mutex_unlock(&q->lock);
    return 1;
}

static void queue_task_done(WorkQueue *q) {
    mutex_lock(&q->lock);
    q->active_workers--;
    if (q->count == 0 && q->active_workers == 0) {
        cond_broadcast(&q->cond);
    }
    mutex_unlock(&q->lock);
}

/* -------------------------------------------------------------------------
   Core Scanning Logic (Fixed for DFS Tree Output & File Printing)
   ------------------------------------------------------------------------- */

static void process_dir_recursive(const char *base_path, int depth, ThreadState *ts);

#ifdef _WIN32
static void process_dir_recursive(const char *base_path, int depth, ThreadState *ts) {
    if (depth > ts->config->max_depth) return;
    WIN32_FIND_DATAW findData;
    HANDLE hFind;
    wchar_t searchPath[PATH_MAX];
    char filenameUtf8[PATH_MAX];
    char *current_slab = ts->path_slab + (depth * PATH_MAX);
    MultiByteToWideChar(CP_UTF8, 0, base_path, -1, searchPath, PATH_MAX);
    if (wcslen(searchPath) + 3 >= PATH_MAX) return;
    wcscat(searchPath, L"\\*");
    hFind = FindFirstFileW(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) { ts->stats.scan_errors++; return; }
    do {
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) continue;
        WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, -1, filenameUtf8, PATH_MAX, NULL, NULL);
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
            
            /* Apply junk filter only if ignore_junk is enabled */
            if (ts->config->ignore_junk && is_ignored(filenameUtf8)) continue;
            
            snprintf(current_slab, PATH_MAX, "%s\\%s", base_path, filenameUtf8);
            
            if (ts->config->show_tree) {
                mutex_lock(&ts->queue->lock);
                PyGILState_STATE gstate = PyGILState_Ensure();
                for (int i = 0; i < depth; i++) PySys_WriteStdout("  |   ");
                PySys_WriteStdout("  |-- [%s]\n", filenameUtf8); /* Bracket indicates folder */
                PyGILState_Release(gstate);

                /* === NEW FEATURE: export_tree === */
                if (ts->config->tree_file) {
                    for (int i = 0; i < depth; i++) fprintf(ts->config->tree_file, "  |   ");
                    fprintf(ts->config->tree_file, "  |-- [%s]\n", filenameUtf8);
                    fflush(ts->config->tree_file);
                }

                mutex_unlock(&ts->queue->lock);
                
                /* Immediate recursive call enforces strict Depth-First Search for the tree */
                process_dir_recursive(current_slab, depth + 1, ts);
            } else {
                if (depth < 3 && !queue_push(ts->queue, current_slab, depth + 1))
                    process_dir_recursive(current_slab, depth + 1, ts);
                else if (depth >= 3)
                    process_dir_recursive(current_slab, depth + 1, ts);
            }
        } else {
            if (ts->config->min_size > 0) {
                LARGE_INTEGER size;
                size.HighPart = findData.nFileSizeHigh;
                size.LowPart = findData.nFileSizeLow;
                if ((uint64_t)size.QuadPart < ts->config->min_size) continue;
            }
            
            if (ts->config->show_tree) {
                mutex_lock(&ts->queue->lock);
                PyGILState_STATE gstate = PyGILState_Ensure();
                for (int i = 0; i < depth; i++) PySys_WriteStdout("  |   ");
                PySys_WriteStdout("  |-- %s\n", filenameUtf8); /* No bracket indicates file */
                PyGILState_Release(gstate);

                /* === NEW FEATURE: export_tree === */
                if (ts->config->tree_file) {
                    for (int i = 0; i < depth; i++) fprintf(ts->config->tree_file, "  |   ");
                    fprintf(ts->config->tree_file, "  |-- %s\n", filenameUtf8);
                    fflush(ts->config->tree_file);
                }

                mutex_unlock(&ts->queue->lock);
            }
            identify_and_count(filenameUtf8, ts);
        }
    } while (FindNextFileW(hFind, &findData) != 0);
    FindClose(hFind);
}

#elif defined(USE_GETDENTS64)
static void process_dir_recursive(const char *base_path, int depth, ThreadState *ts) {
    if (depth > ts->config->max_depth) return;
    int dirfd = open(base_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (dirfd < 0) { ts->stats.scan_errors++; return; }
    char buf[131072];
    char *current_slab = ts->path_slab + (depth * PATH_MAX);
    while (1) {
        long nread = syscall(SYS_getdents64, dirfd, buf, sizeof(buf));
        if (nread == -1) { ts->stats.scan_errors++; break; }
        if (nread == 0) break;
        for (long bpos = 0; bpos < nread;) {
            struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + bpos);
            bpos += d->d_reclen;
            if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0) continue;
            unsigned char type = d->d_type;
            struct stat st;
            uint64_t file_size = 0;
            if (type == DT_UNKNOWN || ts->config->min_size > 0) {
                if (fstatat(dirfd, d->d_name, &st, AT_SYMLINK_NOFOLLOW) == 0) {
                    if (S_ISDIR(st.st_mode)) type = DT_DIR;
                    else if (S_ISREG(st.st_mode)) type = DT_REG;
                    file_size = st.st_size;
                }
            }
            if (type == DT_DIR) {
                if (ts->config->ignore_junk && is_ignored(d->d_name)) continue;
                
                snprintf(current_slab, PATH_MAX, "%s/%s", base_path, d->d_name);
                
                if (ts->config->show_tree) {
                    mutex_lock(&ts->queue->lock);
                    PyGILState_STATE gstate = PyGILState_Ensure();
                    for (int i = 0; i < depth; i++) PySys_WriteStdout("  |   ");
                    PySys_WriteStdout("  |--[%s]\n", d->d_name);
                    PyGILState_Release(gstate);

                    /* === NEW FEATURE: export_tree === */
                    if (ts->config->tree_file) {
                        for (int i = 0; i < depth; i++) fprintf(ts->config->tree_file, "  |   ");
                        fprintf(ts->config->tree_file, "  |-- [%s]\n", d->d_name);
                        fflush(ts->config->tree_file);
                    }

                    mutex_unlock(&ts->queue->lock);
                    
                    process_dir_recursive(current_slab, depth + 1, ts);
                } else {
                    if (depth < 3 && !queue_push(ts->queue, current_slab, depth + 1))
                        process_dir_recursive(current_slab, depth + 1, ts);
                    else if (depth >= 3)
                        process_dir_recursive(current_slab, depth + 1, ts);
                }
            } else if (type == DT_REG) {
                if (ts->config->min_size > 0 && file_size < ts->config->min_size) continue;
                if (ts->config->show_tree) {
                    mutex_lock(&ts->queue->lock);
                    PyGILState_STATE gstate = PyGILState_Ensure();
                    for (int i = 0; i < depth; i++) PySys_WriteStdout("  |   ");
                    PySys_WriteStdout("  |-- %s\n", d->d_name);
                    PyGILState_Release(gstate);

                    /* === NEW FEATURE: export_tree === */
                    if (ts->config->tree_file) {
                        for (int i = 0; i < depth; i++) fprintf(ts->config->tree_file, "  |   ");
                        fprintf(ts->config->tree_file, "  |-- %s\n", d->d_name);
                        fflush(ts->config->tree_file);
                    }

                    mutex_unlock(&ts->queue->lock);
                }
                identify_and_count(d->d_name, ts);
            }
        }
    }
    close(dirfd);
}
#else
static void process_dir_recursive(const char *base_path, int depth, ThreadState *ts) {
    if (depth > ts->config->max_depth) return;
    DIR *dir = opendir(base_path);
    if (!dir) { ts->stats.scan_errors++; return; }
    struct dirent *entry;
    struct stat st;
    char *current_slab = ts->path_slab + (depth * PATH_MAX);
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        snprintf(current_slab, PATH_MAX, "%s/%s", base_path, entry->d_name);
        if (lstat(current_slab, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            if (ts->config->ignore_junk && is_ignored(entry->d_name)) continue;

            if (ts->config->show_tree) {
                mutex_lock(&ts->queue->lock);
                PyGILState_STATE gstate = PyGILState_Ensure();
                for (int i = 0; i < depth; i++) PySys_WriteStdout("  |   ");
                PySys_WriteStdout("  |-- [%s]\n", entry->d_name);
                PyGILState_Release(gstate);

                /* === NEW FEATURE: export_tree === */
                if (ts->config->tree_file) {
                    for (int i = 0; i < depth; i++) fprintf(ts->config->tree_file, "  |   ");
                    fprintf(ts->config->tree_file, "  |-- [%s]\n", entry->d_name);
                    fflush(ts->config->tree_file);
                }

                mutex_unlock(&ts->queue->lock);
                
                process_dir_recursive(current_slab, depth + 1, ts);
            } else {
                if (depth < 3 && !queue_push(ts->queue, current_slab, depth + 1))
                    process_dir_recursive(current_slab, depth + 1, ts);
                else if (depth >= 3)
                    process_dir_recursive(current_slab, depth + 1, ts);
            }
        } else if (S_ISREG(st.st_mode)) {
            if (ts->config->min_size > 0 && (uint64_t)st.st_size < ts->config->min_size) continue;
            if (ts->config->show_tree) {
                mutex_lock(&ts->queue->lock);
                PyGILState_STATE gstate = PyGILState_Ensure();
                for (int i = 0; i < depth; i++) PySys_WriteStdout("  |   ");
                PySys_WriteStdout("  |-- %s\n", entry->d_name);
                PyGILState_Release(gstate);

                /* === NEW FEATURE: export_tree === */
                if (ts->config->tree_file) {
                    for (int i = 0; i < depth; i++) fprintf(ts->config->tree_file, "  |   ");
                    fprintf(ts->config->tree_file, "  |-- %s\n", entry->d_name);
                    fflush(ts->config->tree_file);
                }

                mutex_unlock(&ts->queue->lock);
            }
            identify_and_count(entry->d_name, ts);
        }
    }
    closedir(dir);
}
#endif

/* -------------------------------------------------------------------------
   Thread Entry Points
   ------------------------------------------------------------------------- */

#ifdef _WIN32
static DWORD WINAPI worker_thread_func(LPVOID arg) {
#else
static void* worker_thread_func(void *arg) {
#endif
    ThreadState *ts = (ThreadState *)arg;
    char *path;
    int depth;
    ts->path_slab = (char *)malloc((ts->config->max_depth + 2) * PATH_MAX);
    if (!ts->path_slab) goto exit;
    while (queue_pop(ts->queue, &path, &depth)) {
        process_dir_recursive(path, depth, ts);
        if (path) free(path);
        queue_task_done(ts->queue);
    }
    free(ts->path_slab);
exit:
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

#ifdef _WIN32
static DWORD WINAPI progress_thread_func(LPVOID arg) {
#else
static void* progress_thread_func(void *arg) {
#endif
    ThreadState *master = (ThreadState *)arg;
    int loops = 0;
    while (!master->queue->shutdown) {
#ifdef _WIN32
        Sleep(250);
#else
        usleep(250000);
#endif
        if (master->queue->shutdown) break;
        uint64_t current = ATOMIC_READ(&g_atomic_scanned);
        PyGILState_STATE gstate = PyGILState_Ensure();
        if (!master->config->show_tree) {
            PySys_WriteStdout("Scanned files: %" PRIu64 " ...\r", current);
        }
        if (master->config->callback != Py_None && loops % 4 == 0) {
            PyObject *arglist = Py_BuildValue("(K)", current);
            PyObject *result = PyObject_CallObject(master->config->callback, arglist);
            Py_DECREF(arglist);
            if (result) Py_DECREF(result);
            else PyErr_Clear();
        }
        PyGILState_Release(gstate);
        loops++;
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

/* -------------------------------------------------------------------------
   Python Interface
   ------------------------------------------------------------------------- */

static PyObject* anscom_scan(PyObject *self, PyObject *args, PyObject *keywds) {
    const char *input_path;
    int max_depth = 6;
    int show_tree = 0;
    int workers = 0;
    int silent = 0; 
    int ignore_junk = 0;  /* Default 0: Never miss anything */
    unsigned long long min_size = 0;
    PyObject *extensions_list = Py_None;
    PyObject *callback = Py_None;

    /* === NEW FEATURE: export_json, export_excel, export_tree === */
    const char *export_json = NULL;
    const char *export_excel = NULL;
    const char *export_tree = NULL;

    double elapsed = 0.0;
#ifdef _WIN32
    LARGE_INTEGER start_time, end_time, freq;
#else
    struct timespec start_time, end_time;
#endif

    static char *kwlist[] = {
        "path", "max_depth", "show_tree", "workers",
        "min_size", "extensions", "callback", "silent", "ignore_junk",
        "export_json", "export_excel", "export_tree", NULL
    };

    /* Formatter string updated to support new optional string parameters */
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|ipiKOOppzzz", kwlist,
                                     &input_path, &max_depth, &show_tree, &workers,
                                     &min_size, &extensions_list, &callback, &silent, &ignore_junk,
                                     &export_json, &export_excel, &export_tree)) {
        return NULL;
    }

    if (input_path[0] == '\0') input_path = ".";

    if (max_depth < 0) max_depth = 0;
    if (max_depth > 64) max_depth = 64;

    if (workers <= 0) {
#ifdef _WIN32
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        workers = sysinfo.dwNumberOfProcessors;
#else
        workers = sysconf(_SC_NPROCESSORS_ONLN);
#endif
        if (workers <= 0) workers = 4;
    }
    if (show_tree) workers = 1;

    ScanConfig config;
    memset(&config, 0, sizeof(config));
    config.max_depth = max_depth;
    config.show_tree = show_tree;
    config.workers = workers;
    config.min_size = min_size;
    config.callback = callback;
    config.ignore_junk = ignore_junk;

    if (extensions_list != Py_None && PyList_Check(extensions_list)) {
        config.allowed_exts = calloc(EXTENSION_COUNT, sizeof(uint8_t));
        Py_ssize_t size = PyList_Size(extensions_list);
        for (Py_ssize_t i = 0; i < size; i++) {
            PyObject *item = PyList_GetItem(extensions_list, i);
            if (PyUnicode_Check(item)) {
                const char *ext = PyUnicode_AsUTF8(item);
                uint32_t idx = hash_ext(ext);
                uint32_t start_idx = idx;
                while (EXT_HASH_TABLE[idx] != -1) {
                    int table_idx = EXT_HASH_TABLE[idx];
                    if (str_iequal(EXTENSION_TABLE[table_idx].ext, ext)) {
                        config.allowed_exts[table_idx] = 1;
                        break;
                    }
                    idx = (idx + 1) % HASH_SIZE;
                    if (idx == start_idx) break;
                }
            }
        }
    }

    /* === NEW FEATURE: export_tree === */
    if (show_tree && export_tree != NULL) {
        config.tree_file = fopen(export_tree, "w");
        if (!config.tree_file) {
            PyErr_SetFromErrnoWithFilename(PyExc_IOError, export_tree);
            if (config.allowed_exts) free(config.allowed_exts);
            return NULL;
        }
    } else {
        config.tree_file = NULL;
    }

    WorkQueue *queue = (WorkQueue *)calloc(1, sizeof(WorkQueue));
    if (!queue) {
        PyErr_NoMemory();
        if (config.allowed_exts) free(config.allowed_exts);
        if (config.tree_file) fclose(config.tree_file);
        return NULL;
    }
    mutex_init(&queue->lock);
    cond_init(&queue->cond);

    ThreadState *threads = calloc(workers, sizeof(ThreadState));
    os_thread_t *thread_handles = calloc(workers, sizeof(os_thread_t));
    os_thread_t progress_thread;

    for (int i = 0; i < workers; i++) {
        threads[i].config = &config;
        threads[i].queue = queue;
    }

    g_atomic_scanned = 0;

    PySys_WriteStdout("\nAnscom Enterprise v1.3.0 (Threads: %d)\n", workers);
    PySys_WriteStdout("Target: %s\n", input_path);

#ifdef _WIN32
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start_time);
#else
    clock_gettime(CLOCK_MONOTONIC, &start_time);
#endif

    Py_BEGIN_ALLOW_THREADS

    for (int i = 0; i < workers; i++) {
#ifdef _WIN32
        thread_handles[i] = CreateThread(NULL, 2 * 1024 * 1024, worker_thread_func, &threads[i], 0, NULL);
#else
        pthread_create(&thread_handles[i], NULL, worker_thread_func, &threads[i]);
#endif
    }
#ifdef _WIN32
    progress_thread = CreateThread(NULL, 0, progress_thread_func, &threads[0], 0, NULL);
#else
    pthread_create(&progress_thread, NULL, progress_thread_func, &threads[0]);
#endif

    queue_push(queue, input_path, 0);

    mutex_lock(&queue->lock);
    while (queue->count > 0 || queue->active_workers > 0) {
        cond_wait(&queue->cond, &queue->lock);
    }
    queue->shutdown = 1;
    cond_broadcast(&queue->cond);
    mutex_unlock(&queue->lock);

    for (int i = 0; i < workers; i++) {
#ifdef _WIN32
        WaitForSingleObject(thread_handles[i], INFINITE);
        CloseHandle(thread_handles[i]);
#else
        pthread_join(thread_handles[i], NULL);
#endif
    }
#ifdef _WIN32
    WaitForSingleObject(progress_thread, INFINITE);
    CloseHandle(progress_thread);
#else
    pthread_join(progress_thread, NULL);
#endif

    /* === NEW FEATURE: export_tree cleanup === */
    if (config.tree_file) {
        fclose(config.tree_file);
        config.tree_file = NULL;
    }

#ifdef _WIN32
    QueryPerformanceCounter(&end_time);
    elapsed = (double)(end_time.QuadPart - start_time.QuadPart) / freq.QuadPart;
#else
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    elapsed = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
#endif

    Py_END_ALLOW_THREADS

    if (!show_tree) PySys_WriteStdout("                                         \r");

    /* Merge stats from all threads */
    ScanStats final_stats;
    memset(&final_stats, 0, sizeof(final_stats));
    for (int i = 0; i < workers; i++) {
        final_stats.total_files += threads[i].stats.total_files;
        final_stats.scan_errors += threads[i].stats.scan_errors;
        for (int c = 0; c < CAT_COUNT; c++)
            final_stats.cat_counts[c] += threads[i].stats.cat_counts[c];
        for (int e = 0; e < EXTENSION_COUNT; e++)
            final_stats.ext_counts[e] += threads[i].stats.ext_counts[e];
    }

    free(threads);
    free(thread_handles);
    free(queue);
    if (config.allowed_exts) free(config.allowed_exts);

    if (!silent) {
        print_report(&final_stats, elapsed);
    }

    PyObject *res_dict = PyDict_New();
    PyDict_SetItemString(res_dict, "total_files",      PyLong_FromUnsignedLongLong(final_stats.total_files));
    PyDict_SetItemString(res_dict, "scan_errors",      PyLong_FromUnsignedLongLong(final_stats.scan_errors));
    PyDict_SetItemString(res_dict, "duration_seconds", PyFloat_FromDouble(elapsed));

    PyObject *cat_dict = PyDict_New();
    for (int i = 0; i < CAT_COUNT; i++)
        PyDict_SetItemString(cat_dict, CAT_NAMES[i], PyLong_FromUnsignedLongLong(final_stats.cat_counts[i]));
    PyDict_SetItemString(res_dict, "categories", cat_dict);
    Py_DECREF(cat_dict);

    PyObject *ext_dict = PyDict_New();
    for (int i = 0; i < EXTENSION_COUNT; i++) {
        if (final_stats.ext_counts[i] > 0)
            PyDict_SetItemString(ext_dict, EXTENSION_TABLE[i].ext, PyLong_FromUnsignedLongLong(final_stats.ext_counts[i]));
    }
    PyDict_SetItemString(res_dict, "extensions", ext_dict);
    Py_DECREF(ext_dict);

    /* === NEW FEATURE: export_json === */
    if (export_json != NULL) {
        PyObject *json_mod = PyImport_ImportModule("json");
        if (json_mod) {
            PyObject *dumps_func = PyObject_GetAttrString(json_mod, "dumps");
            if (dumps_func) {
                PyObject *kwargs = PyDict_New();
                PyObject *indent_val = PyLong_FromLong(4);
                PyDict_SetItemString(kwargs, "indent", indent_val);
                Py_DECREF(indent_val);

                PyObject *args_tuple = PyTuple_Pack(1, res_dict);
                PyObject *json_str_obj = PyObject_Call(dumps_func, args_tuple, kwargs);
                
                if (json_str_obj) {
                    const char *json_str = PyUnicode_AsUTF8(json_str_obj);
                    if (json_str) {
                        FILE *f = fopen(export_json, "w");
                        if (f) {
                            fputs(json_str, f);
                            fclose(f);
                        } else {
                            PyErr_SetFromErrnoWithFilename(PyExc_IOError, export_json);
                        }
                    }
                    Py_DECREF(json_str_obj);
                }
                
                Py_DECREF(args_tuple);
                Py_DECREF(kwargs);
                Py_DECREF(dumps_func);
            }
            Py_DECREF(json_mod);
        }
        if (PyErr_Occurred()) {
            Py_DECREF(res_dict);
            return NULL;
        }
    }

    /* === NEW FEATURE: export_excel === */
    if (export_excel != NULL) {
        PyObject *openpyxl = PyImport_ImportModule("openpyxl");
        if (!openpyxl) {
            PyErr_Clear();
            PyErr_SetString(PyExc_ImportError, "openpyxl is required for export_excel but not installed. Please install it via 'pip install openpyxl'.");
            Py_DECREF(res_dict);
            return NULL;
        }
        
        PyObject *wb = PyObject_CallMethod(openpyxl, "Workbook", NULL);
        if (wb) {
            PyObject *ws_cat = PyObject_GetAttrString(wb, "active");
            if (ws_cat) {
                PyObject *title_str = PyUnicode_FromString("Categories");
                PyObject_SetAttrString(ws_cat, "title", title_str);
                Py_DECREF(title_str);

                PyObject *row_tuple = Py_BuildValue("(sss)", "Category", "Count", "Percentage");
                PyObject *res = PyObject_CallMethod(ws_cat, "append", "O", row_tuple);
                Py_DECREF(row_tuple);
                Py_XDECREF(res);
                
                for (int i = 0; i < CAT_COUNT; i++) {
                    double pct = final_stats.total_files ? ((double)final_stats.cat_counts[i] / final_stats.total_files * 100.0) : 0.0;
                    row_tuple = Py_BuildValue("(sKd)", CAT_NAMES[i], final_stats.cat_counts[i], pct);
                    res = PyObject_CallMethod(ws_cat, "append", "O", row_tuple);
                    Py_DECREF(row_tuple);
                    Py_XDECREF(res);
                }
                Py_DECREF(ws_cat);
            }

            PyObject *ws_ext = PyObject_CallMethod(wb, "create_sheet", "s", "Extensions");
            if (ws_ext) {
                PyObject *row_tuple = Py_BuildValue("(ss)", "Extension", "Count");
                Py_DECREF(row_tuple);
                PyObject *res = PyObject_CallMethod(ws_ext, "append", "O", row_tuple);
                Py_XDECREF(res);
                
                for (int i = 0; i < EXTENSION_COUNT; i++) {
                    if (final_stats.ext_counts[i] > 0) {
                        row_tuple = Py_BuildValue("(sK)", EXTENSION_TABLE[i].ext, final_stats.ext_counts[i]);
                        res = PyObject_CallMethod(ws_ext, "append", "O", row_tuple);
                        Py_DECREF(row_tuple);
                        Py_XDECREF(res);
                    }
                }
                Py_DECREF(ws_ext);
            }

            PyObject *ws_sum = PyObject_CallMethod(wb, "create_sheet", "s", "Summary");
            if (ws_sum) {
                PyObject *row_tuple = Py_BuildValue("(ss)", "Metric", "Value");
                PyObject *res = PyObject_CallMethod(ws_sum, "append", "O", row_tuple);
                Py_DECREF(row_tuple);
                Py_XDECREF(res);
                
                row_tuple = Py_BuildValue("(sK)", "Total Files", final_stats.total_files);
                res = PyObject_CallMethod(ws_sum, "append", "O", row_tuple);
                Py_DECREF(row_tuple);
                Py_XDECREF(res);

                row_tuple = Py_BuildValue("(sK)", "Errors", final_stats.scan_errors);
                res = PyObject_CallMethod(ws_sum, "append", "O", row_tuple);
                Py_DECREF(row_tuple);
                Py_XDECREF(res);

                row_tuple = Py_BuildValue("(sd)", "Duration (s)", elapsed);
                res = PyObject_CallMethod(ws_sum, "append", "O", row_tuple);
                Py_DECREF(row_tuple);
                Py_XDECREF(res);

                Py_DECREF(ws_sum);
            }

            PyObject *res_save = PyObject_CallMethod(wb, "save", "s", export_excel);
            Py_XDECREF(res_save);

            Py_DECREF(wb);
        }
        Py_DECREF(openpyxl);
        
        if (PyErr_Occurred()) {
            Py_DECREF(res_dict);
            return NULL;
        }
    }

    return res_dict;
}

/* -------------------------------------------------------------------------
   Module Registration
   ------------------------------------------------------------------------- */

static PyMethodDef AnscomMethods[] = {
    {"scan", (PyCFunction)(void(*)(void))anscom_scan, METH_VARARGS | METH_KEYWORDS,
     "scan(path, max_depth=6, show_tree=False, workers=0, min_size=0, "
     "extensions=None, callback=None, silent=False, ignore_junk=False, "
     "export_json=None, export_excel=None, export_tree=None) -> dict"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef anscommodule = {
    PyModuleDef_HEAD_INIT,
    "anscom",
    "Analyst grade recursive file scanner (v1.3.0 Tree Structure Edition).",
    -1,
    AnscomMethods
};

PyMODINIT_FUNC PyInit_anscom(void) {
    for (int i = 0; i < HASH_SIZE; i++) EXT_HASH_TABLE[i] = -1;
    for (int i = 0; i < EXTENSION_COUNT; i++) {
        if (i > 0 && strcmp(EXTENSION_TABLE[i-1].ext, EXTENSION_TABLE[i].ext) >= 0) {
            PyErr_Format(PyExc_RuntimeError,
                "Anscom Init Error: EXTENSION_TABLE not sorted at index %d ('%s')",
                i, EXTENSION_TABLE[i].ext);
            return NULL;
        }
        uint32_t idx = hash_ext(EXTENSION_TABLE[i].ext);
        while (EXT_HASH_TABLE[idx] != -1) idx = (idx + 1) % HASH_SIZE;
        EXT_HASH_TABLE[idx] = i;
    }
    return PyModule_Create(&anscommodule);
}
