/*
 * anscom.c
 * 
 * Version: v0.5 (Enterprise Edition)
 * Description: Native Python extension with:
 *              - Hard Exclusion (Blacklist) for heavy folders
 *              - Max Recursion Depth Control
 *              - Live In-Place Progress Bar
 *              - Summary-Only Mode by default
 * 
 * Compilation: python setup.py build_ext --inplace
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Cross-platform headers */
#ifdef _WIN32
    #include <windows.h>
    #define PATH_SEP '\\'
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #define PATH_SEP '/'
#endif

/* -------------------------------------------------------------------------
   Configuration & Constants
   ------------------------------------------------------------------------- */

/* HARD EXCLUSION LIST: Folders to strictly skip */
static const char* IGNORE_DIRS[] = {
    ".venv", "venv", "env", 
    "site-packages", "node_modules", "bower_components",
    ".git", ".svn", ".hg", 
    "__pycache__", ".idea", ".vscode",
    "build", "dist", "target", 
    "temp", "tmp", ".cache",
    ".pytest_cache", ".mypy_cache"
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

/* Mutable extension mapping */
typedef struct {
    char ext[16];
    FileCategory category;
    unsigned long long count;
} ExtMap;

/* 
 * MASTER EXTENSION DATABASE 
 * (Preserved from v0.4)
 */
static ExtMap EXTENSION_TABLE[] = {
    {"3g2", CAT_VIDEO, 0}, {"3gp", CAT_VIDEO, 0}, {"7z", CAT_ARCHIVE, 0}, {"aac", CAT_AUDIO, 0},
    {"accdb", CAT_DOCUMENT, 0}, {"ai", CAT_IMAGE, 0}, {"aif", CAT_AUDIO, 0}, {"apk", CAT_ARCHIVE, 0},
    {"app", CAT_EXECUTABLE, 0}, {"asf", CAT_VIDEO, 0}, {"asm", CAT_CODE, 0}, {"asp", CAT_CODE, 0},
    {"aspx", CAT_CODE, 0}, {"avi", CAT_VIDEO, 0}, {"avif", CAT_IMAGE, 0}, {"awk", CAT_CODE, 0},
    {"bak", CAT_SYSTEM, 0}, {"bas", CAT_CODE, 0}, {"bat", CAT_CODE, 0}, {"bin", CAT_EXECUTABLE, 0},
    {"bmp", CAT_IMAGE, 0}, {"bz2", CAT_ARCHIVE, 0}, {"c", CAT_CODE, 0}, {"cab", CAT_ARCHIVE, 0},
    {"cbr", CAT_ARCHIVE, 0}, {"cc", CAT_CODE, 0}, {"cfg", CAT_SYSTEM, 0}, {"class", CAT_EXECUTABLE, 0},
    {"cmd", CAT_CODE, 0}, {"cnf", CAT_SYSTEM, 0}, {"com", CAT_EXECUTABLE, 0}, {"conf", CAT_SYSTEM, 0},
    {"cpp", CAT_CODE, 0}, {"cr2", CAT_IMAGE, 0}, {"crt", CAT_SYSTEM, 0}, {"cs", CAT_CODE, 0},
    {"css", CAT_CODE, 0}, {"csv", CAT_DOCUMENT, 0}, {"cue", CAT_AUDIO, 0}, {"cur", CAT_IMAGE, 0},
    {"dat", CAT_SYSTEM, 0}, {"db", CAT_SYSTEM, 0}, {"dbf", CAT_DOCUMENT, 0}, {"deb", CAT_ARCHIVE, 0},
    {"dll", CAT_EXECUTABLE, 0}, {"dmg", CAT_ARCHIVE, 0}, {"doc", CAT_DOCUMENT, 0}, {"docx", CAT_DOCUMENT, 0},
    {"dot", CAT_DOCUMENT, 0}, {"dotx", CAT_DOCUMENT, 0}, {"drw", CAT_IMAGE, 0}, {"dxf", CAT_IMAGE, 0},
    {"ebook", CAT_DOCUMENT, 0}, {"elf", CAT_EXECUTABLE, 0}, {"eml", CAT_DOCUMENT, 0}, {"env", CAT_SYSTEM, 0},
    {"eps", CAT_IMAGE, 0}, {"epub", CAT_DOCUMENT, 0}, {"exe", CAT_EXECUTABLE, 0}, {"flac", CAT_AUDIO, 0},
    {"flv", CAT_VIDEO, 0}, {"fnt", CAT_SYSTEM, 0}, {"fon", CAT_SYSTEM, 0}, {"fth", CAT_CODE, 0},
    {"gif", CAT_IMAGE, 0}, {"git", CAT_SYSTEM, 0}, {"gitignore", CAT_SYSTEM, 0}, {"go", CAT_CODE, 0},
    {"gpg", CAT_SYSTEM, 0}, {"gradle", CAT_CODE, 0}, {"groovy", CAT_CODE, 0}, {"gz", CAT_ARCHIVE, 0},
    {"h", CAT_CODE, 0}, {"heic", CAT_IMAGE, 0}, {"heif", CAT_IMAGE, 0}, {"hpp", CAT_CODE, 0},
    {"htm", CAT_CODE, 0}, {"html", CAT_CODE, 0}, {"hwp", CAT_DOCUMENT, 0}, {"ico", CAT_IMAGE, 0},
    {"ics", CAT_DOCUMENT, 0}, {"iff", CAT_IMAGE, 0}, {"img", CAT_ARCHIVE, 0}, {"indd", CAT_DOCUMENT, 0},
    {"ini", CAT_SYSTEM, 0}, {"iso", CAT_ARCHIVE, 0}, {"jar", CAT_ARCHIVE, 0}, {"java", CAT_CODE, 0},
    {"jpeg", CAT_IMAGE, 0}, {"jpg", CAT_IMAGE, 0}, {"js", CAT_CODE, 0}, {"json", CAT_CODE, 0},
    {"jsp", CAT_CODE, 0}, {"jsx", CAT_CODE, 0}, {"key", CAT_DOCUMENT, 0}, {"kt", CAT_CODE, 0},
    {"kts", CAT_CODE, 0}, {"less", CAT_CODE, 0}, {"log", CAT_SYSTEM, 0}, {"lua", CAT_CODE, 0},
    {"m", CAT_CODE, 0}, {"m3u", CAT_AUDIO, 0}, {"m4a", CAT_AUDIO, 0}, {"m4v", CAT_VIDEO, 0},
    {"mak", CAT_CODE, 0}, {"md", CAT_DOCUMENT, 0}, {"mdb", CAT_DOCUMENT, 0}, {"mid", CAT_AUDIO, 0},
    {"midi", CAT_AUDIO, 0}, {"mkv", CAT_VIDEO, 0}, {"mm", CAT_CODE, 0}, {"mobi", CAT_DOCUMENT, 0},
    {"mov", CAT_VIDEO, 0}, {"mp3", CAT_AUDIO, 0}, {"mp4", CAT_VIDEO, 0}, {"mpeg", CAT_VIDEO, 0},
    {"mpg", CAT_VIDEO, 0}, {"msi", CAT_EXECUTABLE, 0}, {"nef", CAT_IMAGE, 0}, {"numbers", CAT_DOCUMENT, 0},
    {"obj", CAT_SYSTEM, 0}, {"odp", CAT_DOCUMENT, 0}, {"ods", CAT_DOCUMENT, 0}, {"odt", CAT_DOCUMENT, 0},
    {"ogg", CAT_AUDIO, 0}, {"ogv", CAT_VIDEO, 0}, {"orf", CAT_IMAGE, 0}, {"otf", CAT_SYSTEM, 0},
    {"pages", CAT_DOCUMENT, 0}, {"pak", CAT_ARCHIVE, 0}, {"pas", CAT_CODE, 0}, {"pdf", CAT_DOCUMENT, 0},
    {"pem", CAT_SYSTEM, 0}, {"php", CAT_CODE, 0}, {"pkg", CAT_ARCHIVE, 0}, {"pl", CAT_CODE, 0},
    {"pm", CAT_CODE, 0}, {"png", CAT_IMAGE, 0}, {"ppt", CAT_DOCUMENT, 0}, {"pptx", CAT_DOCUMENT, 0},
    {"ps", CAT_IMAGE, 0}, {"ps1", CAT_CODE, 0}, {"psd", CAT_IMAGE, 0}, {"pub", CAT_DOCUMENT, 0},
    {"py", CAT_CODE, 0}, {"pyc", CAT_SYSTEM, 0}, {"pyd", CAT_EXECUTABLE, 0}, {"pyw", CAT_CODE, 0},
    {"r", CAT_CODE, 0}, {"rar", CAT_ARCHIVE, 0}, {"raw", CAT_IMAGE, 0}, {"rb", CAT_CODE, 0},
    {"reg", CAT_SYSTEM, 0}, {"rm", CAT_VIDEO, 0}, {"rpm", CAT_ARCHIVE, 0}, {"rs", CAT_CODE, 0},
    {"rst", CAT_DOCUMENT, 0}, {"rtf", CAT_DOCUMENT, 0}, {"sass", CAT_CODE, 0}, {"scala", CAT_CODE, 0},
    {"scss", CAT_CODE, 0}, {"sh", CAT_CODE, 0}, {"sln", CAT_CODE, 0}, {"so", CAT_EXECUTABLE, 0},
    {"sql", CAT_CODE, 0}, {"srt", CAT_VIDEO, 0}, {"svg", CAT_IMAGE, 0}, {"swf", CAT_VIDEO, 0},
    {"swift", CAT_CODE, 0}, {"sys", CAT_SYSTEM, 0}, {"tar", CAT_ARCHIVE, 0}, {"tga", CAT_IMAGE, 0},
    {"tgz", CAT_ARCHIVE, 0}, {"tif", CAT_IMAGE, 0}, {"tiff", CAT_IMAGE, 0}, {"tmp", CAT_SYSTEM, 0},
    {"ts", CAT_CODE, 0}, {"tsv", CAT_DOCUMENT, 0}, {"ttf", CAT_SYSTEM, 0}, {"txt", CAT_DOCUMENT, 0},
    {"vb", CAT_CODE, 0}, {"vbox", CAT_SYSTEM, 0}, {"vcd", CAT_ARCHIVE, 0}, {"vcf", CAT_DOCUMENT, 0},
    {"vcxproj", CAT_CODE, 0}, {"vob", CAT_VIDEO, 0}, {"vue", CAT_CODE, 0}, {"wav", CAT_AUDIO, 0},
    {"webm", CAT_VIDEO, 0}, {"webp", CAT_IMAGE, 0}, {"wma", CAT_AUDIO, 0}, {"wmv", CAT_VIDEO, 0},
    {"woff", CAT_SYSTEM, 0}, {"woff2", CAT_SYSTEM, 0}, {"wpd", CAT_DOCUMENT, 0}, {"wps", CAT_DOCUMENT, 0},
    {"wsf", CAT_CODE, 0}, {"xcodeproj", CAT_CODE, 0}, {"xls", CAT_DOCUMENT, 0}, {"xlsm", CAT_DOCUMENT, 0},
    {"xlsx", CAT_DOCUMENT, 0}, {"xml", CAT_CODE, 0}, {"yaml", CAT_CODE, 0}, {"yml", CAT_CODE, 0},
    {"zip", CAT_ARCHIVE, 0}
};

static const int EXTENSION_COUNT = sizeof(EXTENSION_TABLE) / sizeof(ExtMap);

typedef struct {
    unsigned long long counts[CAT_COUNT];
    unsigned long long total_files;
} ScanStats;

/* New Configuration Struct to pass options recursively */
typedef struct {
    int max_depth;
    int show_tree;
    unsigned long long scanned_count;
} ScanConfig;

/* -------------------------------------------------------------------------
   Helper Functions
   ------------------------------------------------------------------------- */

static int compare_ext(const void *key, const void *elem) {
    const char *k = (const char *)key;
    const ExtMap *m = (const ExtMap *)elem;
    return strcmp(k, m->ext);
}

/* Case-insensitive string comparison for blacklist */
static int str_iequal(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

/* Check if directory is in the Blacklist */
static int is_ignored(const char *dirname) {
    int i;
    for (i = 0; i < IGNORE_COUNT; i++) {
        if (str_iequal(dirname, IGNORE_DIRS[i])) {
            return 1;
        }
    }
    return 0;
}

static FileCategory identify_and_count(const char *filename) {
    const char *dot = strrchr(filename, '.');
    char ext_lower[32];
    const char *ext_ptr;
    size_t i = 0;
    ExtMap *found;

    if (!dot || dot == filename) return CAT_UNKNOWN;
    ext_ptr = dot + 1;
    
    while (ext_ptr[i] && i < 31) {
        ext_lower[i] = tolower((unsigned char)ext_ptr[i]);
        i++;
    }
    ext_lower[i] = '\0';

    if (i == 0) return CAT_UNKNOWN;

    found = (ExtMap *)bsearch(ext_lower, EXTENSION_TABLE, EXTENSION_COUNT, sizeof(ExtMap), compare_ext);

    if (found) {
        found->count++;
        return found->category;
    }
    return CAT_UNKNOWN;
}

static void print_tree_prefix(int depth) {
    int i;
    for (i = 0; i < depth; i++) printf("  |   ");
    printf("  |-- ");
}

/* -------------------------------------------------------------------------
   Core Scanning Logic (Refactored)
   ------------------------------------------------------------------------- */

#ifdef _WIN32
static void scan_recursive(const wchar_t *path, ScanStats *stats, ScanConfig *config, int depth) {
    WIN32_FIND_DATAW findData;
    HANDLE hFind;
    wchar_t searchPath[32768];
    wchar_t subPath[32768];
    char filenameUtf8[1024];
    int ret;
    FileCategory cat;

    /* Guard: Depth Limit */
    if (depth > config->max_depth) return;

    if (wcslen(path) + 3 >= 32768) return;
    swprintf(searchPath, 32768, L"%s\\*", path);

    hFind = FindFirstFileW(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
            continue;

        /* Prepare UTF-8 name early */
        ret = WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, -1, 
                                  filenameUtf8, sizeof(filenameUtf8), NULL, NULL);

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            /* Guard: Ignore List */
            if (ret > 0 && is_ignored(filenameUtf8)) continue;

            /* Visual Tree (Only if enabled) */
            if (config->show_tree && ret > 0) {
                print_tree_prefix(depth);
                printf("[%s]\n", filenameUtf8);
            }

            if (wcslen(path) + wcslen(findData.cFileName) + 2 < 32768) {
                swprintf(subPath, 32768, L"%s\\%s", path, findData.cFileName);
                scan_recursive(subPath, stats, config, depth + 1);
            }
        } else {
            /* File handling */
            if (ret > 0) {
                cat = identify_and_count(filenameUtf8);
                stats->counts[cat]++;
                stats->total_files++;
                config->scanned_count++;

                /* Live Progress: Update line every 100 files, ONLY if tree is hidden */
                if (!config->show_tree && (config->scanned_count % 100 == 0)) {
                    printf("Scanned files: %llu ...\r", config->scanned_count);
                    fflush(stdout); /* Force terminal update */
                }
            }
        }
    } while (FindNextFileW(hFind, &findData) != 0);

    FindClose(hFind);
}

#else
static void scan_recursive(const char *path, ScanStats *stats, ScanConfig *config, int depth) {
    DIR *dir;
    struct dirent *entry;
    char full_path[4096];
    struct stat path_stat;
    FileCategory cat;

    /* Guard: Depth Limit */
    if (depth > config->max_depth) return;

    if (!(dir = opendir(path))) return;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        /* Guard: Ignore List */
        if (is_ignored(entry->d_name)) continue;

        if (snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name) >= (int)sizeof(full_path))
            continue;

        if (lstat(full_path, &path_stat) != 0) continue;

        if (S_ISDIR(path_stat.st_mode)) {
            if (config->show_tree) {
                print_tree_prefix(depth);
                printf("[%s]\n", entry->d_name);
            }
            scan_recursive(full_path, stats, config, depth + 1);
        } else if (S_ISREG(path_stat.st_mode)) {
            cat = identify_and_count(entry->d_name);
            stats->counts[cat]++;
            stats->total_files++;
            config->scanned_count++;

            /* Live Progress */
            if (!config->show_tree && (config->scanned_count % 100 == 0)) {
                printf("Scanned files: %llu ...\r", config->scanned_count);
                fflush(stdout);
            }
        }
    }
    closedir(dir);
}
#endif

/* -------------------------------------------------------------------------
   Python Interface
   ------------------------------------------------------------------------- */

static PyObject* anscom_scan(PyObject *self, PyObject *args, PyObject *keywds) {
    const char *input_path;
    int max_depth = 6;      /* Default Depth */
    int show_tree = 0;      /* Default Summary Only (False) */
    
    /* Argument Keywords */
    static char *kwlist[] = {"path", "max_depth", "show_tree", NULL};

    ScanStats stats;
    ScanConfig config;
    int i;
    double percent;
#ifdef _WIN32
    int wlen;
    wchar_t *wpath;
#endif

    /* Reset global stats */
    memset(&stats, 0, sizeof(stats));
    for (i = 0; i < EXTENSION_COUNT; i++) {
        EXTENSION_TABLE[i].count = 0;
    }

    /* Parse arguments with keywords: scan(path, max_depth=6, show_tree=False) */
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|ip", kwlist, 
                                     &input_path, &max_depth, &show_tree)) {
        return NULL;
    }

    /* Setup Config */
    config.max_depth = max_depth;
    config.show_tree = show_tree;
    config.scanned_count = 0;

    printf("\nAnscom Enterprise v0.5\n");
    printf("Target: %s\n", input_path);
    printf("Config: Depth=%d, TreeView=%s\n", max_depth, show_tree ? "ON" : "OFF");
    printf("Guards: Excluding .venv, node_modules, .git, etc.\n");

    if (show_tree) {
        printf("Structure Map:\n.\n");
    } else {
        printf("Status: Scanning...\n");
    }

#ifdef _WIN32
    wlen = MultiByteToWideChar(CP_UTF8, 0, input_path, -1, NULL, 0);
    if (wlen > 0) {
        wpath = (wchar_t *)malloc(wlen * sizeof(wchar_t));
        if (wpath) {
            MultiByteToWideChar(CP_UTF8, 0, input_path, -1, wpath, wlen);
            scan_recursive(wpath, &stats, &config, 0);
            free(wpath);
        }
    }
#else
    scan_recursive(input_path, &stats, &config, 0);
#endif

    /* Clear the progress line if needed */
    if (!show_tree) {
        printf("                                         \r");
    }

    /* 1. General Summary Table */
    printf("\n");
    printf("=== SUMMARY REPORT ================================\n");
    printf("+-----------------+--------------+----------+\n");
    printf("| %-15s | %-12s | %-8s |\n", "Category", "Count", "Percent");
    printf("+-----------------+--------------+----------+\n");

    if (stats.total_files == 0) {
        printf("| %-38s |\n", "No files found.");
    } else {
        for (i = 0; i < CAT_COUNT; i++) {
            if (stats.counts[i] == 0 && i != CAT_UNKNOWN) continue;

            percent = (double)stats.counts[i] / (double)stats.total_files * 100.0;
            printf("| %-15s | %12llu | %7.2f%% |\n", 
                   CAT_NAMES[i], 
                   stats.counts[i], 
                   percent);
        }
    }
    printf("+-----------------+--------------+----------+\n");
    printf("| %-15s | %12llu | %-8s |\n", "TOTAL FILES", stats.total_files, "100.00%");
    printf("+-----------------+--------------+----------+\n");

    /* 2. Detailed Extension Table */
    if (stats.total_files > 0) {
        printf("\n=== DETAILED BREAKDOWN ============================\n");
        printf("+-----------------+--------------+\n");
        printf("| %-15s | %-12s |\n", "Extension Type", "Count");
        printf("+-----------------+--------------+\n");
        
        for (i = 0; i < EXTENSION_COUNT; i++) {
            if (EXTENSION_TABLE[i].count > 0) {
                printf("| .%-14s | %12llu |\n", 
                       EXTENSION_TABLE[i].ext, 
                       EXTENSION_TABLE[i].count);
            }
        }
        printf("+-----------------+--------------+\n");
    }

    return Py_BuildValue("K", stats.total_files);
}

/* -------------------------------------------------------------------------
   Module Registration (Updated for KEYWORDS)
   ------------------------------------------------------------------------- */

static PyMethodDef AnscomMethods[] = {
    {"scan", (PyCFunction)(void(*)(void))anscom_scan, METH_VARARGS | METH_KEYWORDS, 
     "scan(path, max_depth=6, show_tree=False) -> Total count"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef anscommodule = {
    PyModuleDef_HEAD_INIT,
    "anscom",
    "Analyst grade recursive file scanner.",
    -1,
    AnscomMethods
};

PyMODINIT_FUNC PyInit_anscom(void) {
    return PyModule_Create(&anscommodule);
}