/*
 * anscom.c
 * aditya narayan singh
 * Version: v1.5.0 (Tree Structure & DFS Fix) flagship version
 * Description: High-performance, multi-threaded recursive file scanner.
 *              Fixed Deep-Tree generation and added file tracking.
 * Compilation: python setup.py build_ext --inplace
 * update fix_2/21/2026: includes logic to ignore junk or corrupted files  (new version available on PyPl)
 * update: new  version released:-> 13 March 2026 : added features which can analyze directories at terabyte scale under seconds without any lag! via n.log(n).
 * v1.5.0: Removed export_excel (was crashing with openpyxl Workbook.read_only
 *         exception). CSV export is sufficient and works perfectly.
 *         Added: return_files, export_csv, largest_n, find_duplicates, regex_filter.
 *         All other features preserved: tree, JSON, CSV, duplicates,
 *         largest_n, return_files, regex_filter, ignore_junk, callbacks.
 ###attention to users/readers: many parts of the code is generated/implemented/edited by artificial intelligence by claude code 

 Kindly Issue a request if you think think this project can be maded bigger better and stronger than ever before.
 */


#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <sys/stat.h>

/* -------------------------------------------------------------------------
   Cross-platform & OS-specific headers
   ------------------------------------------------------------------------- */
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
    #include <regex.h>
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
    {"zip", CAT_ARCHIVE},
};
static const int EXTENSION_COUNT = sizeof(EXTENSION_TABLE) / sizeof(ExtDef);

#define HASH_SIZE 512
static int EXT_HASH_TABLE[HASH_SIZE];

/* -------------------------------------------------------------------------
   CRC32 and Duplicate Data Structures
   ------------------------------------------------------------------------- */

static const uint32_t crc32_tab[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static uint32_t compute_crc32(const void *buf, size_t size) {
    const uint8_t *p = buf;
    uint32_t crc = ~0U;
    while (size--) crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    return crc ^ ~0U;
}

static uint32_t get_file_crc(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    if (n == 0) return 0;
    return compute_crc32(buf, n);
}

typedef struct {
    char *path;
    uint64_t size;
    char ext[32];
    FileCategory category;
    uint64_t mtime;
} FileInfo;

#define FILEARRAY_INIT_CAP 65536
typedef struct {
    FileInfo *data;
    size_t size;
    size_t capacity;
} FileArray;

typedef struct {
    FileInfo *data;
    int size;
    int capacity;
} TopKHeap;

/* -------------------------------------------------------------------------
   Thread-Safe Data Structures
   ------------------------------------------------------------------------- */

typedef struct {
    uint64_t total_files;
    uint64_t scan_errors;
    uint64_t cat_counts[CAT_COUNT];
    uint64_t ext_counts[sizeof(EXTENSION_TABLE) / sizeof(ExtDef)];
    int largest_n;
    FileInfo *top_files;
    int top_files_count;
    int find_duplicates_enabled;
    uint64_t duplicates_groups;
} ScanStats;

typedef struct {
    int max_depth;
    int show_tree;
    int workers;
    uint64_t min_size;
    PyObject *callback;
    uint8_t *allowed_exts;
    int ignore_junk;
    FILE *tree_file;
    int return_files;
    const char *export_csv;
    int largest_n;
    int find_duplicates;
    PyObject *regex_compiled;
#ifndef _WIN32
    regex_t regex_compiled_posix;
    int has_posix_regex;
#endif
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
    char current_file_path[PATH_MAX];
    uint64_t current_file_size;
    uint64_t current_file_mtime;
    FileArray files;
    TopKHeap top_files;
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

static void heap_push(TopKHeap *h, FileInfo fi, int max_n) {
    if (h->size < max_n) {
        FileInfo new_fi = fi;
        new_fi.path = strdup(fi.path);
        int i = h->size++;
        while (i > 0) {
            int p = (i - 1) / 2;
            if (h->data[p].size <= new_fi.size) break;
            h->data[i] = h->data[p];
            i = p;
        }
        h->data[i] = new_fi;
    } else if (fi.size > h->data[0].size) {
        free(h->data[0].path);
        FileInfo new_fi = fi;
        new_fi.path = strdup(fi.path);
        int i = 0;
        while (i * 2 + 1 < h->size) {
            int left = i * 2 + 1;
            int right = i * 2 + 2;
            int smallest = left;
            if (right < h->size && h->data[right].size < h->data[left].size) {
                smallest = right;
            }
            if (new_fi.size <= h->data[smallest].size) break;
            h->data[i] = h->data[smallest];
            i = smallest;
        }
        h->data[i] = new_fi;
    }
}

static void identify_and_count(const char *filename, ThreadState *ts) {
    /* Regex filter — uses native POSIX regex on Linux for max speed */
    if (ts->config->regex_compiled) {
#ifndef _WIN32
        if (ts->config->has_posix_regex) {
            if (regexec(&ts->config->regex_compiled_posix, ts->current_file_path, 0, NULL, 0) != 0) {
                return;
            }
        } else {
            PyGILState_STATE gstate = PyGILState_Ensure();
            PyObject *res = PyObject_CallMethod(ts->config->regex_compiled, "search", "s", ts->current_file_path);
            if (!res || res == Py_None) {
                if (!res) PyErr_Clear();
                Py_XDECREF(res);
                PyGILState_Release(gstate);
                return;
            }
            Py_DECREF(res);
            PyGILState_Release(gstate);
        }
#else
        PyGILState_STATE gstate = PyGILState_Ensure();
        PyObject *res = PyObject_CallMethod(ts->config->regex_compiled, "search", "s", ts->current_file_path);
        if (!res || res == Py_None) {
            if (!res) PyErr_Clear();
            Py_XDECREF(res);
            PyGILState_Release(gstate);
            return;
        }
        Py_DECREF(res);
        PyGILState_Release(gstate);
#endif
    }

    FileCategory found_cat = CAT_UNKNOWN;
    char found_ext[32] = "";

    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        ts->stats.cat_counts[CAT_UNKNOWN]++;
        ts->stats.total_files++;
        ATOMIC_ADD(&g_atomic_scanned, 1);
        found_cat = CAT_UNKNOWN;
        found_ext[0] = '\0';
    } else {
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
        int matched = 0;
        while (EXT_HASH_TABLE[idx] != -1) {
            int table_idx = EXT_HASH_TABLE[idx];
            if (strcmp(EXTENSION_TABLE[table_idx].ext, ext_lower) == 0) {
                if (ts->config->allowed_exts && !ts->config->allowed_exts[table_idx]) return;
                ts->stats.ext_counts[table_idx]++;
                ts->stats.cat_counts[EXTENSION_TABLE[table_idx].category]++;
                ts->stats.total_files++;
                ATOMIC_ADD(&g_atomic_scanned, 1);
                found_cat = EXTENSION_TABLE[table_idx].category;
                strcpy(found_ext, EXTENSION_TABLE[table_idx].ext);
                matched = 1;
                break;
            }
            idx = (idx + 1) % HASH_SIZE;
            if (idx == start_idx) break;
        }
        if (!matched) {
            if (ts->config->allowed_exts) return;
            ts->stats.cat_counts[CAT_UNKNOWN]++;
            ts->stats.total_files++;
            ATOMIC_ADD(&g_atomic_scanned, 1);
            found_cat = CAT_UNKNOWN;
            strcpy(found_ext, ext_lower);
        }
    }

    /* Collect full FileInfo when any of these features are active */
    if (ts->config->return_files || ts->config->export_csv || ts->config->find_duplicates) {
        if (ts->files.size >= ts->files.capacity) {
            ts->files.capacity = ts->files.capacity == 0 ? FILEARRAY_INIT_CAP : ts->files.capacity * 2;
            ts->files.data = realloc(ts->files.data, ts->files.capacity * sizeof(FileInfo));
        }
        FileInfo *fi = &ts->files.data[ts->files.size++];
        fi->path = strdup(ts->current_file_path);
        fi->size = ts->current_file_size;
        fi->mtime = ts->current_file_mtime;
        fi->category = found_cat;
        strcpy(fi->ext, found_ext);
    }

    /* Top-N largest files heap */
    if (ts->config->largest_n > 0) {
        FileInfo fi;
        fi.path = ts->current_file_path;
        fi.size = ts->current_file_size;
        fi.mtime = ts->current_file_mtime;
        fi.category = found_cat;
        strcpy(fi.ext, found_ext);
        heap_push(&ts->top_files, fi, ts->config->largest_n);
    }
}

/* -------------------------------------------------------------------------
   Report Printing
   ------------------------------------------------------------------------- */

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
    PySys_WriteStdout("===================================================\n");

    if (s->largest_n > 0 && s->top_files_count > 0) {
        PySys_WriteStdout("\n=== TOP %d LARGEST FILES ===========================\n", s->largest_n);
        int limit = s->top_files_count > s->largest_n ? s->largest_n : s->top_files_count;
        for (int k = 0; k < limit; k++) {
            PySys_WriteStdout("%12" PRIu64 " bytes : %s\n",
                s->top_files[k].size, s->top_files[k].path);
        }
        PySys_WriteStdout("===================================================\n");
    }

    if (s->find_duplicates_enabled) {
        PySys_WriteStdout("\n=== DUPLICATES SUMMARY ============================\n");
        PySys_WriteStdout("Groups found : %" PRIu64 "\n", s->duplicates_groups);
        PySys_WriteStdout("===================================================\n");
    }

    PySys_WriteStdout("\n\n");
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
   Core Scanning Logic (DFS-correct Tree + parallel BFS for speed)
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
            if (ts->config->ignore_junk && is_ignored(filenameUtf8)) continue;

            snprintf(current_slab, PATH_MAX, "%s\\%s", base_path, filenameUtf8);

            if (ts->config->show_tree) {
                mutex_lock(&ts->queue->lock);
                PyGILState_STATE gstate = PyGILState_Ensure();
                for (int i = 0; i < depth; i++) PySys_WriteStdout("  |   ");
                PySys_WriteStdout("  |-- [%s]\n", filenameUtf8);
                PyGILState_Release(gstate);
                if (ts->config->tree_file) {
                    for (int i = 0; i < depth; i++) fprintf(ts->config->tree_file, "  |   ");
                    fprintf(ts->config->tree_file, "  |-- [%s]\n", filenameUtf8);
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
        } else {
            uint64_t fsize = ((uint64_t)findData.nFileSizeHigh << 32) | findData.nFileSizeLow;
            uint64_t mtime = ((uint64_t)findData.ftLastWriteTime.dwHighDateTime << 32)
                             | findData.ftLastWriteTime.dwLowDateTime;
            mtime = (mtime - 116444736000000000ULL) / 10000000ULL;

            if (ts->config->min_size > 0 && fsize < ts->config->min_size) continue;

            ts->current_file_size = fsize;
            ts->current_file_mtime = mtime;
            snprintf(ts->current_file_path, PATH_MAX, "%s\\%s", base_path, filenameUtf8);

            if (ts->config->show_tree) {
                mutex_lock(&ts->queue->lock);
                PyGILState_STATE gstate = PyGILState_Ensure();
                for (int i = 0; i < depth; i++) PySys_WriteStdout("  |   ");
                PySys_WriteStdout("  |-- %s\n", filenameUtf8);
                PyGILState_Release(gstate);
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
            uint64_t file_mtime = 0;
            int stat_called = 0;

            if (type == DT_UNKNOWN) {
                if (fstatat(dirfd, d->d_name, &st, AT_SYMLINK_NOFOLLOW) == 0) {
                    stat_called = 1;
                    if (S_ISDIR(st.st_mode)) type = DT_DIR;
                    else if (S_ISREG(st.st_mode)) type = DT_REG;
                }
            }

            if (type == DT_REG &&
                (ts->config->min_size > 0 || ts->config->return_files ||
                 ts->config->export_csv || ts->config->find_duplicates ||
                 ts->config->largest_n > 0)) {
                if (!stat_called) {
                    if (fstatat(dirfd, d->d_name, &st, AT_SYMLINK_NOFOLLOW) == 0)
                        stat_called = 1;
                }
                if (stat_called) {
                    file_size = st.st_size;
                    file_mtime = st.st_mtime;
                }
            }

            if (type == DT_DIR) {
                if (ts->config->ignore_junk && is_ignored(d->d_name)) continue;
                snprintf(current_slab, PATH_MAX, "%s/%s", base_path, d->d_name);

                if (ts->config->show_tree) {
                    mutex_lock(&ts->queue->lock);
                    PyGILState_STATE gstate = PyGILState_Ensure();
                    for (int i = 0; i < depth; i++) PySys_WriteStdout("  |   ");
                    PySys_WriteStdout("  |-- [%s]\n", d->d_name);
                    PyGILState_Release(gstate);
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
                ts->current_file_size = file_size;
                ts->current_file_mtime = file_mtime;
                snprintf(ts->current_file_path, PATH_MAX, "%s/%s", base_path, d->d_name);

                if (ts->config->show_tree) {
                    mutex_lock(&ts->queue->lock);
                    PyGILState_STATE gstate = PyGILState_Ensure();
                    for (int i = 0; i < depth; i++) PySys_WriteStdout("  |   ");
                    PySys_WriteStdout("  |-- %s\n", d->d_name);
                    PyGILState_Release(gstate);
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
            ts->current_file_size = st.st_size;
            ts->current_file_mtime = st.st_mtime;
            snprintf(ts->current_file_path, PATH_MAX, "%s", current_slab);
            if (ts->config->show_tree) {
                mutex_lock(&ts->queue->lock);
                PyGILState_STATE gstate = PyGILState_Ensure();
                for (int i = 0; i < depth; i++) PySys_WriteStdout("  |   ");
                PySys_WriteStdout("  |-- %s\n", entry->d_name);
                PyGILState_Release(gstate);
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
   Duplicate Finding Helpers
   ------------------------------------------------------------------------- */

typedef struct {
    char *path;
    uint64_t size;
    uint32_t crc32;
} DupCand;

static int cmp_dupcand_size(const void *a, const void *b) {
    uint64_t sa = ((const DupCand*)a)->size;
    uint64_t sb = ((const DupCand*)b)->size;
    if (sa < sb) return -1;
    if (sa > sb) return 1;
    return 0;
}

static int cmp_dupcand_crc(const void *a, const void *b) {
    uint32_t ca = ((const DupCand*)a)->crc32;
    uint32_t cb = ((const DupCand*)b)->crc32;
    if (ca < cb) return -1;
    if (ca > cb) return 1;
    return 0;
}

/* -------------------------------------------------------------------------
   Python Interface — anscom.scan()
   ------------------------------------------------------------------------- */

static PyObject* anscom_scan(PyObject *self, PyObject *args, PyObject *keywds) {
    const char *input_path;
    int max_depth = 6;
    int show_tree = 0;
    int workers = 0;
    int silent = 0;
    int ignore_junk = 0;
    unsigned long long min_size = 0;
    PyObject *extensions_list = Py_None;
    PyObject *callback = Py_None;

    const char *export_json  = NULL;
    const char *export_tree  = NULL;
    int return_files         = 0;
    const char *export_csv   = NULL;
    int largest_n            = 0;
    int find_duplicates      = 0;
    const char *regex_filter = NULL;

    double elapsed = 0.0;
#ifdef _WIN32
    LARGE_INTEGER start_time, end_time, freq;
#else
    struct timespec start_time, end_time;
#endif

    static char *kwlist[] = {
        "path", "max_depth", "show_tree", "workers",
        "min_size", "extensions", "callback", "silent", "ignore_junk",
        "export_json", "export_tree",
        "return_files", "export_csv", "largest_n", "find_duplicates", "regex_filter", NULL
    };

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|ipiKOOppzzpzipz", kwlist,
                                     &input_path, &max_depth, &show_tree, &workers,
                                     &min_size, &extensions_list, &callback, &silent, &ignore_junk,
                                     &export_json, &export_tree,
                                     &return_files, &export_csv, &largest_n, &find_duplicates,
                                     &regex_filter)) {
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
    config.max_depth      = max_depth;
    config.show_tree      = show_tree;
    config.workers        = workers;
    config.min_size       = min_size;
    config.callback       = callback;
    config.ignore_junk    = ignore_junk;
    config.return_files   = return_files;
    config.export_csv     = export_csv;
    config.largest_n      = largest_n;
    config.find_duplicates = find_duplicates;

    /* Extension whitelist */
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

    /* Tree file output */
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

    /* Regex compilation — POSIX native on Linux for zero-overhead filtering */
    if (regex_filter) {
        PyObject *re_mod = PyImport_ImportModule("re");
        if (re_mod) {
            PyObject *compile_func = PyObject_GetAttrString(re_mod, "compile");
            if (compile_func) {
                config.regex_compiled = PyObject_CallFunction(compile_func, "s", regex_filter);
                Py_DECREF(compile_func);
            }
            Py_DECREF(re_mod);
        }
        if (!config.regex_compiled) {
            PyErr_SetString(PyExc_ValueError, "Failed to compile regex_filter.");
            if (config.allowed_exts) free(config.allowed_exts);
            if (config.tree_file) fclose(config.tree_file);
            return NULL;
        }
#ifndef _WIN32
        if (regcomp(&config.regex_compiled_posix, regex_filter, REG_EXTENDED | REG_NOSUB) == 0) {
            config.has_posix_regex = 1;
        } else {
            config.has_posix_regex = 0;
        }
#endif
    }

    WorkQueue *queue = (WorkQueue *)calloc(1, sizeof(WorkQueue));
    if (!queue) {
        PyErr_NoMemory();
        if (config.allowed_exts) free(config.allowed_exts);
        if (config.tree_file) fclose(config.tree_file);
        if (config.regex_compiled) Py_DECREF(config.regex_compiled);
#ifndef _WIN32
        if (config.has_posix_regex) regfree(&config.regex_compiled_posix);
#endif
        return NULL;
    }
    mutex_init(&queue->lock);
    cond_init(&queue->cond);

    ThreadState *threads = calloc(workers, sizeof(ThreadState));
    os_thread_t *thread_handles = calloc(workers, sizeof(os_thread_t));
    os_thread_t progress_thread;

    for (int i = 0; i < workers; i++) {
        threads[i].config = &config;
        threads[i].queue  = queue;
        threads[i].files.capacity = FILEARRAY_INIT_CAP;
        threads[i].files.data = malloc(FILEARRAY_INIT_CAP * sizeof(FileInfo));
        threads[i].files.size = 0;
        if (largest_n > 0) {
            threads[i].top_files.capacity = largest_n;
            threads[i].top_files.data = malloc(largest_n * sizeof(FileInfo));
            threads[i].top_files.size = 0;
        }
    }

    g_atomic_scanned = 0;

    PySys_WriteStdout("\nAnscom Enterprise v1.5.0 (Threads: %d)\n", workers);
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

    /* Merge file arrays */
    uint64_t global_files_count = 0;
    for (int i = 0; i < workers; i++) global_files_count += threads[i].files.size;

    FileInfo *global_files = NULL;
    if (global_files_count > 0) {
        global_files = malloc(global_files_count * sizeof(FileInfo));
        uint64_t offset = 0;
        for (int i = 0; i < workers; i++) {
            if (threads[i].files.size > 0) {
                memcpy(global_files + offset, threads[i].files.data,
                       threads[i].files.size * sizeof(FileInfo));
                offset += threads[i].files.size;
            }
        }
    }

    /* Merge top-N heaps */
    TopKHeap global_heap = {0};
    global_heap.capacity = largest_n;
    if (largest_n > 0) {
        global_heap.data = malloc(largest_n * sizeof(FileInfo));
        for (int i = 0; i < workers; i++) {
            for (int j = 0; j < threads[i].top_files.size; j++) {
                heap_push(&global_heap, threads[i].top_files.data[j], largest_n);
            }
        }
    }

    int top_count = global_heap.size;
    FileInfo *sorted_top = NULL;
    if (top_count > 0) {
        sorted_top = malloc(top_count * sizeof(FileInfo));
        for (int i = 0; i < top_count; i++) {
            sorted_top[i] = global_heap.data[i];
            sorted_top[i].path = strdup(global_heap.data[i].path);
        }
        /* Insertion sort — top_count is small (largest_n entries) */
        for (int i = 0; i < top_count - 1; i++) {
            for (int j = i + 1; j < top_count; j++) {
                if (sorted_top[j].size > sorted_top[i].size) {
                    FileInfo temp = sorted_top[i];
                    sorted_top[i] = sorted_top[j];
                    sorted_top[j] = temp;
                }
            }
        }
    }
    final_stats.top_files       = sorted_top;
    final_stats.top_files_count = top_count;
    final_stats.largest_n       = largest_n;
    final_stats.find_duplicates_enabled = find_duplicates;

    /* Duplicate detection: sort by size → CRC32 bucket within same size */
    PyObject *duplicates_list = NULL;
    if (find_duplicates) {
        duplicates_list = PyList_New(0);
        if (global_files_count > 1) {
            DupCand *cands = malloc(global_files_count * sizeof(DupCand));
            for (size_t i = 0; i < global_files_count; i++) {
                cands[i].path  = global_files[i].path;
                cands[i].size  = global_files[i].size;
                cands[i].crc32 = 0;
            }
            qsort(cands, global_files_count, sizeof(DupCand), cmp_dupcand_size);

            size_t start = 0;
            while (start < global_files_count) {
                size_t end = start + 1;
                while (end < global_files_count && cands[end].size == cands[start].size) end++;
                if (end - start > 1 && cands[start].size > 0) {
                    for (size_t i = start; i < end; i++)
                        cands[i].crc32 = get_file_crc(cands[i].path);
                    qsort(&cands[start], end - start, sizeof(DupCand), cmp_dupcand_crc);

                    size_t cstart = start;
                    while (cstart < end) {
                        size_t cend = cstart + 1;
                        while (cend < end && cands[cend].crc32 == cands[cstart].crc32) cend++;
                        if (cend - cstart > 1) {
                            PyObject *group = PyList_New(0);
                            for (size_t k = cstart; k < cend; k++) {
                                PyObject *pystr = PyUnicode_FromString(cands[k].path);
                                PyList_Append(group, pystr);
                                Py_DECREF(pystr);
                            }
                            PyList_Append(duplicates_list, group);
                            Py_DECREF(group);
                            final_stats.duplicates_groups++;
                        }
                        cstart = cend;
                    }
                }
                start = end;
            }
            free(cands);
        }
    }

    if (!silent) print_report(&final_stats, elapsed);

    /* CSV Export */
    if (export_csv != NULL && global_files_count > 0) {
        FILE *f = fopen(export_csv, "w");
        if (f) {
            fprintf(f, "path,size,ext,category,mtime\n");
            for (uint64_t i = 0; i < global_files_count; i++) {
                char *p = global_files[i].path;
                fputc('"', f);
                while (*p) {
                    if (*p == '"') fputc('"', f);
                    fputc(*p++, f);
                }
                fprintf(f, "\",%" PRIu64 ",%s,%s,%" PRIu64 "\n",
                        global_files[i].size,
                        global_files[i].ext,
                        CAT_NAMES[global_files[i].category],
                        global_files[i].mtime);
            }
            fclose(f);
        }
    }

    /* Build result dict */
    PyObject *res_dict = PyDict_New();
    PyDict_SetItemString(res_dict, "total_files",      PyLong_FromUnsignedLongLong(final_stats.total_files));
    PyDict_SetItemString(res_dict, "scan_errors",      PyLong_FromUnsignedLongLong(final_stats.scan_errors));
    PyDict_SetItemString(res_dict, "duration_seconds", PyFloat_FromDouble(elapsed));

    PyObject *cat_dict = PyDict_New();
    for (int i = 0; i < CAT_COUNT; i++)
        PyDict_SetItemString(cat_dict, CAT_NAMES[i],
                             PyLong_FromUnsignedLongLong(final_stats.cat_counts[i]));
    PyDict_SetItemString(res_dict, "categories", cat_dict);
    Py_DECREF(cat_dict);

    PyObject *ext_dict = PyDict_New();
    for (int i = 0; i < EXTENSION_COUNT; i++) {
        if (final_stats.ext_counts[i] > 0)
            PyDict_SetItemString(ext_dict, EXTENSION_TABLE[i].ext,
                                 PyLong_FromUnsignedLongLong(final_stats.ext_counts[i]));
    }
    PyDict_SetItemString(res_dict, "extensions", ext_dict);
    Py_DECREF(ext_dict);

    if (return_files) {
        PyObject *files_list = PyList_New(0);
        for (uint64_t i = 0; i < global_files_count; i++) {
            PyObject *file_dict = PyDict_New();
            PyDict_SetItemString(file_dict, "path",     PyUnicode_FromString(global_files[i].path));
            PyDict_SetItemString(file_dict, "size",     PyLong_FromUnsignedLongLong(global_files[i].size));
            PyDict_SetItemString(file_dict, "ext",      PyUnicode_FromString(global_files[i].ext));
            PyDict_SetItemString(file_dict, "category", PyUnicode_FromString(CAT_NAMES[global_files[i].category]));
            PyDict_SetItemString(file_dict, "mtime",    PyLong_FromUnsignedLongLong(global_files[i].mtime));
            PyList_Append(files_list, file_dict);
            Py_DECREF(file_dict);
        }
        PyDict_SetItemString(res_dict, "files", files_list);
        Py_DECREF(files_list);
    }

    if (largest_n > 0) {
        PyObject *top_list = PyList_New(0);
        for (int i = 0; i < top_count; i++) {
            PyObject *file_dict = PyDict_New();
            PyDict_SetItemString(file_dict, "path", PyUnicode_FromString(sorted_top[i].path));
            PyDict_SetItemString(file_dict, "size", PyLong_FromUnsignedLongLong(sorted_top[i].size));
            PyList_Append(top_list, file_dict);
            Py_DECREF(file_dict);
        }
        PyDict_SetItemString(res_dict, "largest_files", top_list);
        Py_DECREF(top_list);
    }

    if (find_duplicates && duplicates_list) {
        PyDict_SetItemString(res_dict, "duplicates", duplicates_list);
        Py_DECREF(duplicates_list);
    }

    /* JSON Export */
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
                        if (f) { fputs(json_str, f); fclose(f); }
                        else PyErr_SetFromErrnoWithFilename(PyExc_IOError, export_json);
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
            if (global_files) {
                for (uint64_t i = 0; i < global_files_count; i++) free(global_files[i].path);
                free(global_files);
            }
            free(threads); free(thread_handles); free(queue);
            if (config.allowed_exts) free(config.allowed_exts);
            if (config.regex_compiled) Py_DECREF(config.regex_compiled);
#ifndef _WIN32
            if (config.has_posix_regex) regfree(&config.regex_compiled_posix);
#endif
            return NULL;
        }
    }

    /* Cleanup */
    if (global_files) {
        for (uint64_t i = 0; i < global_files_count; i++) free(global_files[i].path);
        free(global_files);
    }
    for (int i = 0; i < workers; i++) {
        if (threads[i].files.data) free(threads[i].files.data);
        for (int j = 0; j < threads[i].top_files.size; j++) free(threads[i].top_files.data[j].path);
        if (threads[i].top_files.data) free(threads[i].top_files.data);
    }
    if (sorted_top) {
        for (int i = 0; i < top_count; i++) free(sorted_top[i].path);
        free(sorted_top);
    }
    if (global_heap.data) {
        for (int i = 0; i < global_heap.size; i++) free(global_heap.data[i].path);
        free(global_heap.data);
    }
    if (config.regex_compiled) {
        Py_DECREF(config.regex_compiled);
#ifndef _WIN32
        if (config.has_posix_regex) regfree(&config.regex_compiled_posix);
#endif
    }
    free(threads);
    free(thread_handles);
    free(queue);
    if (config.allowed_exts) free(config.allowed_exts);

    return res_dict;
}

/* -------------------------------------------------------------------------
   Module Registration
   ------------------------------------------------------------------------- */

static PyMethodDef AnscomMethods[] = {
    {"scan", (PyCFunction)(void(*)(void))anscom_scan, METH_VARARGS | METH_KEYWORDS,
     "scan(path, max_depth=6, show_tree=False, workers=0, min_size=0,\n"
     "     extensions=None, callback=None, silent=False, ignore_junk=False,\n"
     "     export_json=None, export_tree=None,\n"
     "     return_files=False, export_csv=None, largest_n=0,\n"
     "     find_duplicates=False, regex_filter=None) -> dict\n\n"
     "High-performance multi-threaded recursive file scanner.\n\n"
     "Parameters\n----------\n"
     "path         : str   — root directory to scan\n"
     "max_depth    : int   — maximum recursion depth (default 6, max 64)\n"
     "show_tree    : bool  — print directory tree to stdout (forces workers=1)\n"
     "workers      : int   — thread count (0 = auto-detect CPU count)\n"
     "min_size     : int   — skip files smaller than this many bytes\n"
     "extensions   : list  — whitelist of extensions to count, e.g. ['py','js']\n"
     "callback     : func  — called every ~1 s with (files_scanned,)\n"
     "silent       : bool  — suppress the summary report\n"
     "ignore_junk  : bool  — skip common junk dirs (node_modules, .git, etc.)\n"
     "export_json  : str   — path to write JSON report\n"
     "export_tree  : str   — path to write tree text file (requires show_tree)\n"
     "return_files : bool  — include full file list in returned dict\n"
     "export_csv   : str   — path to write CSV of all files\n"
     "largest_n    : int   — report top-N largest files\n"
     "find_duplicates: bool — detect duplicate files (CRC32 after size match)\n"
     "regex_filter : str   — only count files whose path matches this pattern\n\n"
     "Returns\n-------\n"
     "dict with keys: total_files, scan_errors, duration_seconds,\n"
     "                categories, extensions, [files], [largest_files], [duplicates]"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef anscommodule = {
    PyModuleDef_HEAD_INIT,
    "anscom",
    "Anscom Enterprise v1.5.0 — High-performance native C recursive file scanner.\n"
    "Multi-threaded, terabyte-scale. Features: tree, JSON, CSV, duplicates,\n"
    "largest-N, regex filter, extension whitelist, progress callback.",
    -1,
    AnscomMethods
};

PyMODINIT_FUNC PyInit_anscom(void) {
    /* Build open-addressing hash table for extension lookup */
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
