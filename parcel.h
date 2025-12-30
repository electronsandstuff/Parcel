/* =========================================================================
    Parcel - A C library for reading OpenPMD particle and mesh data
    Copyright (c) 2025 Christopher M. Pierce
    SPDX-License-Identifier: BSD-3-Clause
========================================================================= */

#ifndef PARCEL_H
#define PARCEL_H

#include <stddef.h>
#include <stdint.h>
#include <hdf5.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Status Codes
 * ========================================================================= */

/**
 * pmd_status - Return status codes for Parcel API functions
 */
typedef enum {
    PMD_SUCCESS = 0,                 /* Operation succeeded */
    PMD_ERROR = -1,                  /* General error */
    PMD_ERROR_NULL_POINTER = -2,     /* Null pointer argument */
    PMD_ERROR_INVALID_ITERATION = -3,/* Invalid iteration index */
    PMD_ERROR_INVALID_SPECIES = -4,  /* Species not found */
    PMD_ERROR_OUT_OF_MEMORY = -5,    /* Memory allocation failed */
    PMD_ERROR_FILE_NOT_FOUND = -6,   /* File doesn't exist */
    PMD_ERROR_FILE_FORMAT = -7,      /* Invalid/malformed file format */
    PMD_ERROR_HDF5 = -8              /* HDF5 library error */
} pmd_status;

/* =========================================================================
 * Iteration Encoding Types
 * ========================================================================= */

/**
 * pmd_iteration_encoding - How iterations are stored in the series
 */
typedef enum {
    PMD_FILE_BASED,                  /* Multiple files (one per iteration) */
    PMD_GROUP_BASED                  /* Single file with group per iteration */
} pmd_iteration_encoding;

/* =========================================================================
 * Logging
 * ========================================================================= */

/**
 * pmd_log_level - Log level for controlling message verbosity
 */
typedef enum {
    PMD_LOG_NONE = -1,   /* Suppress all messages */
    PMD_LOG_ERROR = 0,   /* Error messages only */
    PMD_LOG_WARNING = 1, /* Errors and warnings */
    PMD_LOG_INFO = 2,    /* Errors, warnings, and info */
    PMD_LOG_DEBUG = 3    /* All messages including debug */
} pmd_log_level;

/* =========================================================================
 * Particle Data Structures
 * ========================================================================= */

/**
 * particle_group - Represents a collection of particles, either allocated with
 * pmd_allocate_particle_group or array pointers are set by user to existing
 * arrays in beam physics code being intergrated with.
 */
typedef struct {
    int64_t num_particles;       /* Number of particles in group */
    char *species_type;          /* Species name (e.g., "electron") */

    /* Position arrays */
    double *x;                   /* x positions (m) */
    double *y;                   /* y positions (m) */
    double *z;                   /* z positions (m) */
    double *t;                   /* Time (s) */

    /* Momentum arrays */
    double *px;                  /* x momentum (eV/c) */
    double *py;                  /* y momentum (eV/c) */
    double *pz;                  /* z momentum (eV/c) */

    /* Optional per-particle data */
    double *weight;              /* Macro-particle weights */
    int64_t *status;             /* Particle status (1=alive) */
    int64_t *id;                 /* Particle IDs */

} particle_group;

/**
 * particle_group_read_info - Information about how particle data was read
 *
 * Used to report which optional fields were present in the file and other
 * read operation metadata.
 */
typedef struct {
    bool t_present;                 /* true if time dataset exists */
    bool px_present;                /* true if momentum/x dataset exists */
    bool py_present;                /* true if momentum/y dataset exists */
    bool pz_present;                /* true if momentum/z dataset exists */
    bool weight_present;            /* true if weight dataset exists */
    bool status_present;            /* true if particleStatus dataset exists */
    bool id_present;                /* true if id dataset exists */
} particle_group_read_info;

/* =========================================================================
 * Series and Iteration Handles
 * ========================================================================= */

/**
 * pmd_series - Handle for an OpenPMD data series
 *
 * Represents a collection of iterations that may be stored in:
 * - A single file with multiple groups (GROUP_BASED)
 * - Multiple files with one iteration each (FILE_BASED)
 */
typedef struct {
    /* File handle (for GROUP_BASED only, -1 for FILE_BASED) */
    hid_t file_id;

    /* OpenPMD version */
    int openpmd_version_major;
    int openpmd_version_minor;
    int openpmd_version_revision;

    /* Required OpenPMD metadata */
    char *base_path;                  /* e.g., "/data/%T/" */
    char *iteration_format;           /* e.g., "sim_%T.h5" or "/data/%T/" */

    /* Iteration encoding type */
    pmd_iteration_encoding iteration_encoding;

    /* Optional paths (private - use accessor functions) */
    char *_particles_path;            /* e.g., "particles/" */
    char *_meshes_path;               /* e.g., "meshes/" */

    /* For FILE_BASED: directory containing files */
    char *directory;                  /* Directory containing files */

    /* Cached metadata */
    int num_iterations;               /* -1 if not enumerated yet */
    int64_t *iteration_indices;       /* Array of available iterations */
} pmd_series;

/**
 * pmd_iteration - Handle for a single iteration in an OpenPMD series
 *
 * Represents a specific timestep with its metadata and particle data
 */
typedef struct {
    pmd_series *series;               /* Parent series */
    int64_t iteration_index;          /* Current iteration number */

    /* Handle to data (one will be valid, the other -1) */
    hid_t file_id;                    /* For FILE_BASED only */
    hid_t iteration_group_id;         /* For GROUP_BASED only */

    /* Iteration metadata */
    double time;                      /* Current time */
    double dt;                        /* Time step */
    double time_unit_si;              /* Conversion to seconds */

    /* Species info (cached) */
    int num_species;
    char **species_names;
    int64_t *num_particles;           /* Per-species particle counts */
} pmd_iteration;

/* =========================================================================
 * API Function Declarations
 * ========================================================================= */

/* --- Logging Configuration --- */

/**
 * Set the minimum log level for messages
 *
 * Messages with a level below this threshold will be suppressed.
 * Default level is PMD_LOG_WARNING (shows errors and warnings).
 *
 * @param level Minimum log level to display
 */
void pmd_set_log_level(pmd_log_level level);

/* --- Series Operations --- */

/**
 * Open an OpenPMD series from a file
 *
 * @param filename Path to OpenPMD file
 * @param series_out Output pointer to created series handle
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_open_series(const char *filename, pmd_series **series_out);

/**
 * Close an OpenPMD series and free resources
 *
 * @param series Series handle to close
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_close_series(pmd_series *series);

/**
 * Get list of available iterations in the series
 *
 * @param series Series handle
 * @param iterations Output pointer to array of iteration indices
 * @param count Output pointer to number of iterations
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_get_iterations(pmd_series *series, int64_t **iterations, int *count);

/* --- Iteration Operations --- */

/**
 * Open a specific iteration within a series
 *
 * @param series Parent series handle
 * @param index Iteration index to open
 * @param iter_out Output pointer to created iteration handle
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_open_iteration(pmd_series *series, int64_t index, pmd_iteration **iter_out);

/**
 * Close an iteration and free resources
 *
 * @param iter Iteration handle to close
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_close_iteration(pmd_iteration *iter);

/**
 * Get list of particle species in the iteration
 *
 * @param iter Iteration handle
 * @param species_names Output pointer to array of species name strings
 * @param count Output pointer to number of species
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_get_species(pmd_iteration *iter, char ***species_names, int *count);

/**
 * Get number of particles for a species in the iteration
 *
 * @param iter Iteration handle
 * @param species Species name
 * @param count Output pointer to particle count
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_get_num_particles(pmd_iteration *iter, const char *species, int64_t *count);

/* --- Series Metadata Operations --- */

/**
 * Get particlesPath (returns copy of cached value from series)
 *
 * @param series Series handle
 * @param value_out Output pointer to particles path string (caller must free)
 * @return PMD_SUCCESS or PMD_ERROR if not set
 */
pmd_status pmd_get_particles_path(pmd_series *series, char **value_out);

/**
 * Get meshesPath (returns copy of cached value from series)
 *
 * @param series Series handle
 * @param value_out Output pointer to meshes path string (caller must free)
 * @return PMD_SUCCESS or PMD_ERROR if not set
 */
pmd_status pmd_get_meshes_path(pmd_series *series, char **value_out);

/**
 * Get openPMDextension attribute (reads from file on-demand)
 *
 * @param series Series handle
 * @param value_out Output pointer to extension string (caller must free)
 * @return PMD_SUCCESS or error code (PMD_ERROR if attribute doesn't exist)
 */
pmd_status pmd_get_openpmd_extension(pmd_series *series, char **value_out);

/**
 * Get author attribute (reads from file on-demand)
 *
 * @param series Series handle
 * @param value_out Output pointer to author string (caller must free)
 * @return PMD_SUCCESS or error code (PMD_ERROR if attribute doesn't exist)
 */
pmd_status pmd_get_author(pmd_series *series, char **value_out);

/**
 * Get software attribute (reads from file on-demand)
 *
 * @param series Series handle
 * @param value_out Output pointer to software string (caller must free)
 * @return PMD_SUCCESS or error code (PMD_ERROR if attribute doesn't exist)
 */
pmd_status pmd_get_software(pmd_series *series, char **value_out);

/**
 * Get softwareVersion attribute (reads from file on-demand)
 *
 * @param series Series handle
 * @param value_out Output pointer to version string (caller must free)
 * @return PMD_SUCCESS or error code (PMD_ERROR if attribute doesn't exist)
 */
pmd_status pmd_get_software_version(pmd_series *series, char **value_out);

/**
 * Get date attribute (reads from file on-demand)
 *
 * @param series Series handle
 * @param value_out Output pointer to date string (caller must free)
 * @return PMD_SUCCESS or error code (PMD_ERROR if attribute doesn't exist)
 */
pmd_status pmd_get_date(pmd_series *series, char **value_out);

/**
 * Get softwareDependencies attribute (reads from file on-demand)
 *
 * @param series Series handle
 * @param value_out Output pointer to dependencies string (caller must free)
 * @return PMD_SUCCESS or error code (PMD_ERROR if attribute doesn't exist)
 */
pmd_status pmd_get_software_dependencies(pmd_series *series, char **value_out);

/**
 * Get machine attribute (reads from file on-demand)
 *
 * @param series Series handle
 * @param value_out Output pointer to machine string (caller must free)
 * @return PMD_SUCCESS or error code (PMD_ERROR if attribute doesn't exist)
 */
pmd_status pmd_get_machine(pmd_series *series, char **value_out);

/**
 * Get comment attribute (reads from file on-demand)
 *
 * @param series Series handle
 * @param value_out Output pointer to comment string (caller must free)
 * @return PMD_SUCCESS or error code (PMD_ERROR if attribute doesn't exist)
 */
pmd_status pmd_get_comment(pmd_series *series, char **value_out);

/* --- Particle Data Operations --- */

/**
 * Allocate memory for a particle_group
 *
 * Allocates all arrays based on the number of particles for the species.
 * User must call pmd_free_particle_group() when done.
 *
 * @param iter Iteration handle
 * @param species Species name
 * @param pg_out Output pointer to allocated particle_group
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_allocate_particle_group(pmd_iteration *iter, const char *species,
                                        particle_group **pg_out);

/**
 * Read particle group data
 *
 * Reads particle data into a pre-allocated particle_group.
 * The particle_group arrays must be allocated before calling (e.g., via pmd_allocate_particle_group).
 *
 * @param iter Iteration handle
 * @param species Species name
 * @param pg Pre-allocated particle_group to fill
 * @param read_info Optional output for read metadata (can be NULL)
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_read_particle_group(pmd_iteration *iter, const char *species,
                                    particle_group *pg, particle_group_read_info *read_info);

/**
 * Free a particle_group and its arrays
 *
 * @param pg particle_group to free
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_free_particle_group(particle_group *pg);

/**
 * Write particle group to iteration (not yet implemented)
 *
 * @param iter Iteration handle
 * @param species Species name
 * @param pg particle_group to write
 * @return PMD_ERROR (not implemented)
 */
pmd_status pmd_write_particle_group(pmd_iteration *iter, const char *species,
                                     const particle_group *pg);

/* --- Utility Functions --- */

#ifdef __cplusplus
}
#endif

/* =========================================================================
 * Implementation
 * ========================================================================= */

#ifdef PARCEL_IMPLEMENTATION

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>   /* For isdigit() */
#include <stdarg.h>  /* For variadic arguments */

/* =========================================================================
 * Platform-Specific Includes and Definitions
 * ========================================================================= */

#if defined(_WIN32) || defined(_WIN64)
    /* Windows platform */
    #define PMD_PLATFORM_WINDOWS
    #include <windows.h>
    #include <io.h>
    #define PMD_PATH_MAX MAX_PATH
    #define PMD_PATH_SEP "\\"
    #define PMD_PATH_SEP_CHAR '\\'
#else
    /* POSIX platform (Linux, macOS, etc.) */
    #define PMD_PLATFORM_POSIX
    #include <limits.h>  /* For PATH_MAX */
    #include <libgen.h>  /* For dirname() */
    #include <dirent.h>  /* For directory scanning */
    #define PMD_PATH_MAX PATH_MAX
    #define PMD_PATH_SEP "/"
    #define PMD_PATH_SEP_CHAR '/'
#endif

/* =========================================================================
 * Logging
 * ========================================================================= */

/**
 * Global log level threshold - messages below this level are suppressed
 * Default: PMD_LOG_WARNING (shows errors and warnings)
 */
static pmd_log_level pmd_log_threshold = PMD_LOG_WARNING;

/**
 * Set the log level threshold (implementation)
 */
void pmd_set_log_level(pmd_log_level level) {
    pmd_log_threshold = level;
}

/**
 * Log a message with the specified log level
 *
 * @param level Log level
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
static void pmd_log(pmd_log_level level, const char *format, ...) {
    if (level > pmd_log_threshold) {
        return;  /* Message level below threshold, suppress */
    }

    const char *level_str;
    switch (level) {
        case PMD_LOG_ERROR:   level_str = "Error"; break;
        case PMD_LOG_WARNING: level_str = "Warning"; break;
        case PMD_LOG_INFO:    level_str = "Info"; break;
        case PMD_LOG_DEBUG:   level_str = "Debug"; break;
        default:              level_str = "Unknown"; break;
    }

    va_list args;
    va_start(args, format);

    fprintf(stderr, "%s: ", level_str);
    vfprintf(stderr, format, args);

    va_end(args);
}

/* =========================================================================
 * Forward Declarations
 * ========================================================================= */

static int record_exists(hid_t group_id, const char *name);
static pmd_status validate_attribute_type(hid_t attr_id, H5T_class_t expected_class);
static pmd_status read_record_generic(hid_t group_id, const char *name,
                                       void *array, hid_t h5_type, size_t elem_size,
                                       int64_t num_particles, double *unit_si_out);
static pmd_status read_double_record(hid_t group_id, const char *name, double *array,
                                      int64_t num_particles, double unit_multiplier);
static pmd_status read_int64_record(hid_t group_id, const char *name, int64_t *array,
                                     int64_t num_particles);
static pmd_status read_double_attribute(hid_t loc_id, const char *attr_name, double *value_out);


/* Pattern matching forward declarations */
typedef struct {
    char *scan_parent;
    char *first_segment;
    const char *full_pattern;
} iteration_pattern;

static pmd_status parse_iteration_pattern(const char *pattern, iteration_pattern *info);
static void free_iteration_pattern(iteration_pattern *info);
static pmd_status extract_iteration_from_name(const char *name, const char *pattern,
                                                int64_t *iteration_out);
static char* replace_iteration(const char *pattern, int64_t iteration);

/* =========================================================================
 * Platform-Specific Directory and Path Utilities
 * ========================================================================= */

#ifdef PMD_PLATFORM_WINDOWS

/* Windows directory iteration structure */
typedef struct {
    HANDLE handle;
    WIN32_FIND_DATAA find_data;
    int first_call;
} pmd_dir;

/* Windows directory entry structure */
typedef struct {
    char d_name[MAX_PATH];
} pmd_dirent;

/**
 * Open directory for reading (Windows implementation)
 */
static pmd_dir* pmd_opendir(const char *path) {
    pmd_dir *dir = (pmd_dir*)malloc(sizeof(pmd_dir));
    if (!dir) return NULL;

    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", path);

    dir->handle = FindFirstFileA(search_path, &dir->find_data);
    if (dir->handle == INVALID_HANDLE_VALUE) {
        free(dir);
        return NULL;
    }

    dir->first_call = 1;
    return dir;
}

/**
 * Read next directory entry (Windows implementation)
 */
static pmd_dirent* pmd_readdir(pmd_dir *dir) {
    static pmd_dirent entry;

    if (!dir) return NULL;

    if (dir->first_call) {
        dir->first_call = 0;
        strncpy(entry.d_name, dir->find_data.cFileName, MAX_PATH - 1);
        entry.d_name[MAX_PATH - 1] = '\0';
        return &entry;
    }

    if (FindNextFileA(dir->handle, &dir->find_data)) {
        strncpy(entry.d_name, dir->find_data.cFileName, MAX_PATH - 1);
        entry.d_name[MAX_PATH - 1] = '\0';
        return &entry;
    }

    return NULL;
}

/**
 * Close directory (Windows implementation)
 */
static int pmd_closedir(pmd_dir *dir) {
    if (!dir) return -1;
    FindClose(dir->handle);
    free(dir);
    return 0;
}

/**
 * Extract directory path from filename (Windows implementation)
 * Caller must free the returned string
 */
static char* pmd_dirname(const char *path) {
    char *dir = strdup(path);
    if (!dir) return NULL;

    /* Find last backslash or forward slash */
    char *last_sep = NULL;
    char *p = dir;
    while (*p) {
        if (*p == '\\' || *p == '/') {
            last_sep = p;
        }
        p++;
    }

    if (last_sep) {
        *last_sep = '\0';
    } else {
        /* No separator found, return "." */
        free(dir);
        dir = strdup(".");
    }

    return dir;
}

/**
 * Extract basename from path (Windows implementation)
 * Caller must free the returned string
 */
static char* pmd_basename(const char *path) {
    /* Find last backslash or forward slash */
    const char *last_sep = NULL;
    const char *p = path;
    while (*p) {
        if (*p == '\\' || *p == '/') {
            last_sep = p;
        }
        p++;
    }

    if (last_sep) {
        return strdup(last_sep + 1);
    } else {
        return strdup(path);
    }
}

#else  /* PMD_PLATFORM_POSIX */

/* POSIX: use standard types and functions */
typedef DIR pmd_dir;
typedef struct dirent pmd_dirent;

#define pmd_opendir opendir
#define pmd_readdir readdir
#define pmd_closedir closedir

/**
 * Extract directory path from filename (POSIX implementation)
 * Caller must free the returned string
 */
static char* pmd_dirname(const char *path) {
    char *path_copy = strdup(path);
    if (!path_copy) return NULL;
    char *result = strdup(dirname(path_copy));
    free(path_copy);
    return result;
}

/**
 * Extract basename from path (POSIX implementation)
 * Caller must free the returned string
 */
static char* pmd_basename(const char *path) {
    char *path_copy = strdup(path);
    if (!path_copy) return NULL;
    char *result = strdup(basename(path_copy));
    free(path_copy);
    return result;
}

#endif  /* PMD_PLATFORM_WINDOWS */


/* =========================================================================
 * Helper Functions
 * ========================================================================= */

/**
 * Check if an attribute exists on an HDF5 object
 * Returns 1 if exists, 0 if not, -1 on error
 */
static int attribute_exists(hid_t loc_id, const char *attr_name) {
    htri_t exists = H5Aexists(loc_id, attr_name);
    return (int)exists;
}

/**
 * Read a string attribute from an HDF5 object
 * Returns PMD_SUCCESS on success, error code on failure
 */
static pmd_status read_string_attribute(hid_t loc_id, const char *attr_name, char **value_out) {
    hid_t attr_id, atype_id;
    char *str_value = NULL;
    htri_t is_variable;
    pmd_status status;

    /* Check if attribute exists */
    if (attribute_exists(loc_id, attr_name) <= 0) {
        pmd_log(PMD_LOG_ERROR, "Missing '%s' attribute\n", attr_name);
        return PMD_ERROR_FILE_FORMAT;
    }

    /* Open attribute */
    attr_id = H5Aopen(loc_id, attr_name, H5P_DEFAULT);
    if (attr_id < 0) {
        return PMD_ERROR_HDF5;
    }

    /* Check that attribute is right type */
    status = validate_attribute_type(attr_id, H5T_STRING);
    if (status != PMD_SUCCESS) {
        H5Aclose(attr_id);
        return status;
    }

    /* Get attribute type */
    atype_id = H5Aget_type(attr_id);

    /* Check if variable-length string */
    is_variable = H5Tis_variable_str(atype_id);

    if (is_variable > 0) {
        /* Variable-length string - HDF5 allocates memory */
        char *vlen_str = NULL;
        if (H5Aread(attr_id, atype_id, &vlen_str) < 0) {
            H5Tclose(atype_id);
            H5Aclose(attr_id);
            return PMD_ERROR_HDF5;
        }

        /* Copy to our own allocated memory */
        str_value = strdup(vlen_str);
        if (!str_value) {
            H5free_memory(vlen_str);
            H5Tclose(atype_id);
            H5Aclose(attr_id);
            return PMD_ERROR_OUT_OF_MEMORY;
        }

        /* Free HDF5-allocated memory */
        H5free_memory(vlen_str);
    } else {
        /* Fixed-length string */
        size_t size = H5Tget_size(atype_id);

        /* Allocate buffer */
        str_value = (char *)malloc(size + 1);
        if (!str_value) {
            H5Tclose(atype_id);
            H5Aclose(attr_id);
            return PMD_ERROR_OUT_OF_MEMORY;
        }

        /* Read attribute */
        if (H5Aread(attr_id, atype_id, str_value) < 0) {
            free(str_value);
            H5Tclose(atype_id);
            H5Aclose(attr_id);
            return PMD_ERROR_HDF5;
        }

        str_value[size] = '\0';  /* Null terminate */
    }

    *value_out = str_value;

    H5Tclose(atype_id);
    H5Aclose(attr_id);
    return PMD_SUCCESS;
}

pmd_status pmd_open_series(const char *filename, pmd_series **series_out) {
    pmd_series *series = NULL;
    hid_t file_id = -1;
    char *iter_encoding_str = NULL;
    pmd_status status = PMD_SUCCESS;
    char *actual_filename = NULL;
    int is_pattern = 0;

    /* Validate input */
    if (!filename || !series_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Allocate series struct */
    series = (pmd_series *)calloc(1, sizeof(pmd_series));
    if (!series) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    /* Initialize fields */
    series->file_id = -1;
    series->num_iterations = -1;
    series->iteration_indices = NULL;

    /* Check if filename contains %T pattern */
    if (strstr(filename, "%T") != NULL) {
        is_pattern = 1;
        /* For pattern-based filename, search directory for matching files */

        /* Parse the pattern to extract directory and filename components */
        iteration_pattern pattern_info;
        status = parse_iteration_pattern(filename, &pattern_info);
        if (status != PMD_SUCCESS) {
            free(series);
            return status;
        }

        /* Open directory (scan_parent is "." for root) */
        pmd_dir *dir = pmd_opendir(pattern_info.scan_parent);
        if (!dir) {
            free_iteration_pattern(&pattern_info);
            free(series);
            return PMD_ERROR_FILE_NOT_FOUND;
        }

        /* Find first file matching the pattern */
        int found = 0;
        pmd_dirent *entry;
        while ((entry = pmd_readdir(dir)) != NULL && !found) {
            int64_t iteration;
            /* Try to extract iteration from name matching first segment pattern */
            if (extract_iteration_from_name(entry->d_name, pattern_info.first_segment, &iteration) == PMD_SUCCESS) {
                /* Found a matching file, construct full path */
                char full_path[PMD_PATH_MAX];
                snprintf(full_path, sizeof(full_path), "%s" PMD_PATH_SEP "%s", pattern_info.scan_parent, entry->d_name);
                file_id = H5Fopen(full_path, H5F_ACC_RDONLY, H5P_DEFAULT);
                if (file_id >= 0) {
                    actual_filename = strdup(full_path);
                    found = 1;
                }
            }
        }

        pmd_closedir(dir);
        free_iteration_pattern(&pattern_info);

        if (!found) {
            free(series);
            return PMD_ERROR_FILE_NOT_FOUND;
        }
    } else {
        /* Open HDF5 file directly */
        file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
        if (file_id < 0) {
            free(series);
            return PMD_ERROR_FILE_NOT_FOUND;
        }
        actual_filename = strdup(filename);
    }

    /* Read and validate required openPMD attribute */
    char *openpmd_version = NULL;
    status = read_string_attribute(file_id, "openPMD", &openpmd_version);
    if (status != PMD_SUCCESS) {
        goto cleanup;
    }

    /* Parse version (format: "X.Y.Z") */
    int major, minor, revision;
    if (sscanf(openpmd_version, "%d.%d.%d", &major, &minor, &revision) != 3) {
        pmd_log(PMD_LOG_ERROR, "Invalid OpenPMD version format '%s' in '%s' (expected X.Y.Z)\n",
                openpmd_version, filename);
        free(openpmd_version);
        status = PMD_ERROR_FILE_FORMAT;
        goto cleanup;
    }
    series->openpmd_version_major = major;
    series->openpmd_version_minor = minor;
    series->openpmd_version_revision = revision;

    /* Warn if major version is greater than 2 (our implementation target) */
    if (major > 2) {
        pmd_log(PMD_LOG_WARNING, "File '%s' uses OpenPMD version %d.%d.%d, but this library implements version 2.x.x "
                "Some features may not be supported or may behave unexpectedly.\n",
                filename, major, minor, revision);
    }

    free(openpmd_version);

    /* Read required basePath attribute */
    status = read_string_attribute(file_id, "basePath", &series->base_path);
    if (status != PMD_SUCCESS) {
        goto cleanup;
    }

    /* Read iterationFormat (use basePath as default if missing) */
    if (attribute_exists(file_id, "iterationFormat") > 0) {
        status = read_string_attribute(file_id, "iterationFormat", &series->iteration_format);
        if (status != PMD_SUCCESS) goto cleanup;
    } else {
        pmd_log(PMD_LOG_WARNING, "Missing 'iterationFormat' attribute in '%s', using basePath as default\n", filename);
        series->iteration_format = strdup(series->base_path);
    }

    /* Read iterationEncoding (default to groupBased if missing) */
    if (attribute_exists(file_id, "iterationEncoding") > 0) {
        status = read_string_attribute(file_id, "iterationEncoding", &iter_encoding_str);
        if (status != PMD_SUCCESS) goto cleanup;
    } else {
        pmd_log(PMD_LOG_WARNING, "Missing 'iterationEncoding' attribute in '%s', defaulting to 'groupBased'\n", filename);
        iter_encoding_str = strdup("groupBased");
    }

    /* Read optional particlesPath attribute */
    if (attribute_exists(file_id, "particlesPath") > 0) {
        status = read_string_attribute(file_id, "particlesPath", &series->_particles_path);
        if (status != PMD_SUCCESS) goto cleanup;
    } else {
        /* particlesPath is optional - if not present, file has no particles */
        series->_particles_path = NULL;
    }

    /* Read optional meshesPath attribute */
    if (attribute_exists(file_id, "meshesPath") > 0) {
        status = read_string_attribute(file_id, "meshesPath", &series->_meshes_path);
        if (status != PMD_SUCCESS) goto cleanup;
    } else {
        series->_meshes_path = NULL;
    }

    /* Parse iteration encoding and handle file lifecycle */
    if (strcmp(iter_encoding_str, "fileBased") == 0) {
        series->iteration_encoding = PMD_FILE_BASED;

        /* Validate that iteration_format doesn't have subdirectories */
        iteration_pattern pattern_check;
        pmd_status pattern_status = parse_iteration_pattern(series->iteration_format, &pattern_check);
        if (pattern_status != PMD_SUCCESS) {
            status = pattern_status;
            goto cleanup;
        }

        /* For FILE_BASED, scan_parent must be "." (no subdirectories allowed) */
        if (strcmp(pattern_check.scan_parent, ".") != 0) {
            free_iteration_pattern(&pattern_check);
            status = PMD_ERROR_FILE_FORMAT;
            goto cleanup;
        }
        free_iteration_pattern(&pattern_check);

        /* For fileBased, extract directory */
        series->directory = pmd_dirname(filename);
        /* Don't keep file open for fileBased */
        H5Fclose(file_id);
        series->file_id = -1;
    } else if (strcmp(iter_encoding_str, "groupBased") == 0) {
        series->iteration_encoding = PMD_GROUP_BASED;
        /* For groupBased, keep file open */
        series->file_id = file_id;
    } else {
        status = PMD_ERROR_FILE_FORMAT;
        goto cleanup;
    }

    free(iter_encoding_str);
    free(actual_filename);
    *series_out = series;
    return PMD_SUCCESS;

cleanup:
    if (file_id >= 0 && series->file_id < 0) {
        H5Fclose(file_id);
    }
    if (series) {
        pmd_close_series(series);
    }
    free(iter_encoding_str);
    free(actual_filename);
    return status;
}

pmd_status pmd_close_series(pmd_series *series) {
    if (!series) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Close file if open */
    if (series->file_id >= 0) {
        H5Fclose(series->file_id);
    }

    /* Free all allocated strings */
    free(series->base_path);
    free(series->iteration_format);
    free(series->_particles_path);
    free(series->_meshes_path);
    free(series->directory);
    free(series->iteration_indices);

    /* Free the struct itself */
    free(series);

    return PMD_SUCCESS;
}

/**
 * Comparison function for sorting iteration indices
 */
static int compare_int64(const void *a, const void *b) {
    int64_t arg1 = *(const int64_t *)a;
    int64_t arg2 = *(const int64_t *)b;
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

/**
 * Helper struct for collecting iterations during HDF5 group iteration
 */
typedef struct {
    int64_t *indices;
    int count;
    int capacity;
    const char *first_segment;  /* Pattern for first segment (e.g., "data_%T") */
    const char *full_pattern;   /* Full pattern for validation (e.g., "/data/%T/step_%T/") */
    hid_t root_id;               /* Root file ID for validating full paths */
    pmd_status status;           /* Error status from memory allocation */
} iteration_collector;

/**
 * Callback for H5Literate to collect iteration group names
 */
static herr_t collect_iterations_callback(hid_t loc_id, const char *name,
                                           const H5L_info_t *info, void *op_data) {
    iteration_collector *collector = (iteration_collector *)op_data;
    int64_t iteration;

    /* Try to extract iteration number from name matching first segment pattern */
    if (extract_iteration_from_name(name, collector->first_segment, &iteration) != PMD_SUCCESS) {
        return 0;  /* Name doesn't match pattern, skip */
    }

    /* Validate full path if pattern has additional components after first segment */
    if (collector->full_pattern) {
        /* Replace all %T in full pattern with extracted iteration */
        char *full_path = replace_iteration(collector->full_pattern, iteration);
        if (!full_path) {
            collector->status = PMD_ERROR_OUT_OF_MEMORY;
            return -1;
        }

        /* Check if this path exists in the HDF5 file */
        htri_t exists = H5Lexists(collector->root_id, full_path, H5P_DEFAULT);
        free(full_path);

        if (exists <= 0) {
            /* Path doesn't exist or error checking - skip this iteration */
            return 0;
        }
    }

    /* Grow array if needed */
    if (collector->count >= collector->capacity) {
        collector->capacity = collector->capacity * 2 + 10;
        int64_t *temp = (int64_t *)realloc(collector->indices,
                                            collector->capacity * sizeof(int64_t));
        if (!temp) {
            collector->status = PMD_ERROR_OUT_OF_MEMORY;
            return -1;
        }
        collector->indices = temp;
    }

    collector->indices[collector->count++] = iteration;
    return 0;  /* Continue iteration */
}

pmd_status pmd_get_iterations(pmd_series *series, int64_t **iterations, int *count) {
    if (!series || !iterations || !count) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* If already enumerated, return cached results */
    if (series->num_iterations >= 0) {
        *iterations = series->iteration_indices;
        *count = series->num_iterations;
        return PMD_SUCCESS;
    }

    /* Check for single-snapshot file (no %T in iteration_format) */
    if (!strstr(series->iteration_format, "%T")) {
        /* Single iteration at index 0 */
        series->iteration_indices = (int64_t *)malloc(sizeof(int64_t));
        if (!series->iteration_indices) {
            return PMD_ERROR_OUT_OF_MEMORY;
        }
        series->iteration_indices[0] = 0;
        series->num_iterations = 1;
        *iterations = series->iteration_indices;
        *count = 1;
        return PMD_SUCCESS;
    }

    /* Parse iteration_format for both GROUP_BASED and FILE_BASED */
    iteration_pattern pattern_info;
    pmd_status status = parse_iteration_pattern(series->iteration_format, &pattern_info);
    if (status != PMD_SUCCESS) {
        return status;
    }

    /* Common collector for both GROUP_BASED and FILE_BASED */
    iteration_collector collector = {NULL, 0, 0, NULL, NULL, -1, PMD_SUCCESS};

    /* Initialize collector with pattern info */
    collector.first_segment = pattern_info.first_segment;
    collector.full_pattern = pattern_info.full_pattern;

    if (series->iteration_encoding == PMD_GROUP_BASED) {

        /* GROUP_BASED: enumerate groups using pattern matching */
        collector.root_id = series->file_id;

        /* Open parent group (scan_parent is "." for root) */
        if (record_exists(series->file_id, pattern_info.scan_parent) < 1) {
            free_iteration_pattern(&pattern_info);
            return PMD_ERROR_FILE_FORMAT;
        }
        hid_t group_id = H5Gopen(series->file_id, pattern_info.scan_parent, H5P_DEFAULT);

        if (group_id < 0) {
            free_iteration_pattern(&pattern_info);
            return PMD_ERROR_HDF5;
        }

        /* Iterate through groups to find iterations */
        herr_t iter_result = H5Literate(group_id, H5_INDEX_NAME, H5_ITER_NATIVE, NULL,
                                         collect_iterations_callback, &collector);

        H5Gclose(group_id);

        /* Check if iteration callback encountered an error */
        if (iter_result < 0 && collector.status != PMD_SUCCESS) {
            free(collector.indices);
            free_iteration_pattern(&pattern_info);
            return collector.status;
        }

    } else {  /* PMD_FILE_BASED */

        /* FILE_BASED: scan directory for matching files using pattern matching */
        /* For FILE_BASED, scan_parent is always "." (validated in pmd_open_series) */

        pmd_dir *dir = pmd_opendir(series->directory);
        if (!dir) {
            free_iteration_pattern(&pattern_info);
            return PMD_ERROR_FILE_NOT_FOUND;
        }

        pmd_dirent *entry;

        /* Scan directory for matching files/directories */
        while ((entry = pmd_readdir(dir)) != NULL) {
            int64_t iteration;

            /* Try to extract iteration from name matching first segment pattern */
            if (extract_iteration_from_name(entry->d_name, pattern_info.first_segment, &iteration) != PMD_SUCCESS) {
                continue;  /* Name doesn't match, skip */
            }

            /* Validate full path if pattern has additional components */
            if (pattern_info.full_pattern && strchr(pattern_info.full_pattern, '/')) {
                /* Build full file path with iteration substituted */
                char *rel_path = replace_iteration(series->iteration_format, iteration);
                if (!rel_path) {
                    collector.status = PMD_ERROR_OUT_OF_MEMORY;
                    break;
                }

                char full_path[PMD_PATH_MAX];
                snprintf(full_path, sizeof(full_path), "%s" PMD_PATH_SEP "%s", series->directory, rel_path);
                free(rel_path);

                /* Check if file exists using fopen (standard C) */
                FILE *test_file = fopen(full_path, "r");
                if (!test_file) {
                    continue;  /* Path doesn't exist, skip */
                }
                fclose(test_file);
            }

            /* Grow array if needed */
            if (collector.count >= collector.capacity) {
                collector.capacity = collector.capacity * 2 + 10;
                int64_t *temp = (int64_t *)realloc(collector.indices,
                                                   collector.capacity * sizeof(int64_t));
                if (!temp) {
                    collector.status = PMD_ERROR_OUT_OF_MEMORY;
                    break;
                }
                collector.indices = temp;
            }

            collector.indices[collector.count++] = iteration;
        }

        pmd_closedir(dir);

        /* Check for errors during collection */
        if (collector.status != PMD_SUCCESS) {
            free(collector.indices);
            free_iteration_pattern(&pattern_info);
            return collector.status;
        }
    }

    /* Free pattern info now that we're done with both branches */
    free_iteration_pattern(&pattern_info);

    /* Sort the iterations */
    if (collector.count > 0) {
        qsort(collector.indices, collector.count, sizeof(int64_t), compare_int64);
    }

    /* Cache results */
    series->iteration_indices = collector.indices;
    series->num_iterations = collector.count;
    *iterations = collector.indices;
    *count = collector.count;

    return PMD_SUCCESS;
}

/* =========================================================================
 * Iteration Pattern Parsing
 * ========================================================================= */

/**
 * Parse an iteration pattern into scan parent and first segment
 *
 * Splits "/a/path/to/data/data_%T/more/%T" into:
 * - scan_parent: "/a/path/to/data"
 * - first_segment: "data_%T"
 * - full_pattern: reference to input pattern
 *
 * @param pattern Pattern string with %T placeholder(s)
 * @param info Output structure (caller must call free_iteration_pattern)
 * @return PMD_SUCCESS or error code
 */
static pmd_status parse_iteration_pattern(const char *pattern, iteration_pattern *info) {
    if (!pattern || !info) {
        return PMD_ERROR_NULL_POINTER;
    }

    info->scan_parent = NULL;
    info->first_segment = NULL;
    info->full_pattern = pattern;

    /* Find first %T */
    const char *first_T = strstr(pattern, "%T");
    if (!first_T) {
        /* No %T - treat whole thing as parent, empty first_segment */
        info->scan_parent = strdup(pattern);
        info->first_segment = strdup("");
        if (!info->scan_parent || !info->first_segment) {
            free(info->scan_parent);
            free(info->first_segment);
            return PMD_ERROR_OUT_OF_MEMORY;
        }
        return PMD_SUCCESS;
    }

    /* Find last '/' before first %T to get scan parent */
    const char *last_slash = first_T;
    while (last_slash > pattern && *last_slash != '/') {
        last_slash--;
    }

    /* Extract scan_parent (everything up to last slash before %T) */
    if (last_slash == pattern) {
        /* %T is at root level (use "." to refer to root/current directory) */
        info->scan_parent = strdup(".");
    } else {
        size_t parent_len = last_slash - pattern;
        info->scan_parent = (char *)malloc(parent_len + 1);
        if (!info->scan_parent) {
            return PMD_ERROR_OUT_OF_MEMORY;
        }
        strncpy(info->scan_parent, pattern, parent_len);
        info->scan_parent[parent_len] = '\0';
    }

    /* Extract first_segment (from after last slash to next slash or end) */
    const char *segment_start = (*last_slash == '/') ? last_slash + 1 : last_slash;
    const char *segment_end = segment_start;

    /* Find end of first segment (next '/' or end of string) */
    while (*segment_end != '\0' && *segment_end != '/') {
        segment_end++;
    }

    size_t segment_len = segment_end - segment_start;
    info->first_segment = (char *)malloc(segment_len + 1);
    if (!info->first_segment) {
        free(info->scan_parent);
        return PMD_ERROR_OUT_OF_MEMORY;
    }
    strncpy(info->first_segment, segment_start, segment_len);
    info->first_segment[segment_len] = '\0';

    return PMD_SUCCESS;
}

/**
 * Free iteration pattern components
 */
static void free_iteration_pattern(iteration_pattern *info) {
    if (info) {
        free(info->scan_parent);
        free(info->first_segment);
        info->scan_parent = NULL;
        info->first_segment = NULL;
        info->full_pattern = NULL;
    }
}

/**
 * Extract iteration number from a name matching a pattern segment
 *
 * Matches names like "data_5" against patterns like "data_%T" and extracts 5.
 * Handles multiple %T in the pattern (all must match the same number).
 *
 * @param name Name to match (e.g., "data_5")
 * @param pattern Pattern to match against (e.g., "data_%T")
 * @param iteration_out Output for extracted iteration number
 * @return PMD_SUCCESS if match found and iteration extracted, error otherwise
 */
static pmd_status extract_iteration_from_name(const char *name, const char *pattern,
                                                int64_t *iteration_out) {
    const char *p = pattern;
    const char *n = name;
    char matched_number[64] = {0};
    int found_T = 0;

    if (!name || !pattern || !iteration_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Check for ambiguous consecutive %T patterns */
    const char *check = pattern;
    while (*check) {
        if (*check == '%' && *(check + 1) == 'T') {
            /* Found a %T, check if another %T immediately follows */
            if (*(check + 2) == '%' && *(check + 3) == 'T') {
                return PMD_ERROR;  /* Consecutive %T%T is ambiguous */
            }
            check += 2;
        } else {
            check++;
        }
    }

    while (*p && *n) {
        if (*p == '%' && *(p + 1) == 'T') {
            /* %T should match one or more digits */
            if (!isdigit(*n)) {
                return PMD_ERROR;
            }

            /* Extract digit sequence */
            const char *digit_start = n;
            while (isdigit(*n)) {
                n++;
            }
            size_t digit_len = n - digit_start;

            if (!found_T) {
                /* First %T - store matched digits */
                if (digit_len >= sizeof(matched_number)) {
                    return PMD_ERROR;
                }
                strncpy(matched_number, digit_start, digit_len);
                matched_number[digit_len] = '\0';
                found_T = 1;
            } else {
                /* Subsequent %T must match same digits */
                if (strlen(matched_number) != digit_len ||
                    strncmp(matched_number, digit_start, digit_len) != 0) {
                    return PMD_ERROR;
                }
            }

            p += 2;
        } else if (*p == *n) {
            p++;
            n++;
        } else {
            return PMD_ERROR;
        }
    }

    /* Both should be at end for complete match */
    if (*p != '\0' || *n != '\0') {
        return PMD_ERROR;
    }

    if (!found_T) {
        return PMD_ERROR;
    }

    /* Parse matched number */
    char *endptr;
    *iteration_out = strtoll(matched_number, &endptr, 10);
    if (*endptr != '\0' || endptr == matched_number) {
        return PMD_ERROR;
    }

    return PMD_SUCCESS;
}

/* =========================================================================
 * Iteration Operations Implementation
 * ========================================================================= */

/**
 * Replace all occurrences of %T in a pattern with an iteration number
 *
 * Handles patterns with multiple %T placeholders:
 * - "/data/%T/" with 42 -> "/data/42/"
 * - "/data/%T/step_%T/" with 5 -> "/data/5/step_5/"
 *
 * @param pattern Pattern string with %T placeholder(s)
 * @param iteration Iteration number to substitute
 * @return Newly allocated string with all %T replaced (caller must free), or NULL on error
 */
static char* replace_iteration(const char *pattern, int64_t iteration) {
    if (!pattern) {
        return NULL;
    }

    /* Count %T occurrences */
    int count = 0;
    const char *p = pattern;
    while ((p = strstr(p, "%T")) != NULL) {
        count++;
        p += 2;
    }

    /* No %T found */
    if (count == 0) {
        return strdup(pattern);
    }

    /* Format iteration number */
    char iter_str[32];
    snprintf(iter_str, sizeof(iter_str), "%lld", (long long)iteration);
    size_t iter_len = strlen(iter_str);

    /* Calculate result length: original - (count * 2) + (count * iter_len) */
    size_t result_len = strlen(pattern) - (count * 2) + (count * iter_len);
    char *result = (char *)malloc(result_len + 1);
    if (!result) {
        return NULL;
    }

    /* Build result string, replacing each %T */
    const char *src = pattern;
    char *dst = result;
    while (*src) {
        if (*src == '%' && *(src + 1) == 'T') {
            strcpy(dst, iter_str);
            dst += iter_len;
            src += 2;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';

    return result;
}

/**
 * Read a double attribute from an HDF5 object
 * Returns PMD_SUCCESS on success, error code on failure
 */
static pmd_status read_double_attribute(hid_t loc_id, const char *attr_name, double *value_out) {
    hid_t attr_id;

    attr_id = H5Aopen(loc_id, attr_name, H5P_DEFAULT);
    if (attr_id < 0) {
        return PMD_ERROR_HDF5;
    }

    if (H5Aread(attr_id, H5T_NATIVE_DOUBLE, value_out) < 0) {
        H5Aclose(attr_id);
        return PMD_ERROR_HDF5;
    }

    H5Aclose(attr_id);
    return PMD_SUCCESS;
}

/**
 * Callback to count species groups (only count groups, not datasets)
 */
static herr_t count_species_iteration_callback(hid_t loc_id, const char *name,
                                                const H5L_info_t *info, void *op_data) {
    int *count = (int *)op_data;

    /* Check if this is a group, not a dataset */
    H5O_info2_t obj_info;
    if (H5Oget_info_by_name(loc_id, name, &obj_info, H5O_INFO_BASIC, H5P_DEFAULT) >= 0) {
        if (obj_info.type == H5O_TYPE_GROUP) {
            (*count)++;
        }
    }

    return 0;
}

/**
 * Helper struct for collecting species during iteration
 */
typedef struct {
    char **names;
    int64_t *num_particles;
    int count;
    pmd_status status;  /* Error status from validation */
} species_collector;

/**
 * Callback to collect species names and particle counts
 */
static herr_t collect_species_iteration_callback(hid_t loc_id, const char *name,
                                                   const H5L_info_t *info, void *op_data) {
    species_collector *collector = (species_collector *)op_data;
    hid_t species_group_id, attr_id;
    int64_t num_particles;
    pmd_status status;

    /* Check if this is a group, not a dataset */
    H5O_info2_t obj_info;
    if (H5Oget_info_by_name(loc_id, name, &obj_info, H5O_INFO_BASIC, H5P_DEFAULT) < 0) {
        return 0;  /* Skip on error */
    }
    if (obj_info.type != H5O_TYPE_GROUP) {
        /* Species must be a group, not a dataset - file format error */
        pmd_log(PMD_LOG_ERROR, "Species '%s' is a dataset, expected a group\n", name);
        collector->status = PMD_ERROR_FILE_FORMAT;
        return -1;  /* Stop iteration with error */
    }

    /* Open species group */
    species_group_id = H5Gopen(loc_id, name, H5P_DEFAULT);
    if (species_group_id < 0) return 0;

    /* Read numParticles attribute */
    attr_id = H5Aopen(species_group_id, "numParticles", H5P_DEFAULT);
    if (attr_id >= 0) {
        /* Validate attribute type */
        status = validate_attribute_type(attr_id, H5T_INTEGER);
        if (status != PMD_SUCCESS) {
            H5Aclose(attr_id);
            H5Gclose(species_group_id);
            collector->status = status;
            return -1;  /* Stop iteration with error */
        }

        if (H5Aread(attr_id, H5T_NATIVE_INT64, &num_particles) >= 0) {
            /* Validate that numParticles is non-negative */
            if (num_particles < 0) {
                H5Aclose(attr_id);
                H5Gclose(species_group_id);
                collector->status = PMD_ERROR_FILE_FORMAT;
                return -1;  /* Stop iteration with error */
            }

            collector->names[collector->count] = strdup(name);
            if (!collector->names[collector->count]) {
                H5Aclose(attr_id);
                H5Gclose(species_group_id);
                collector->status = PMD_ERROR_OUT_OF_MEMORY;
                return -1;  /* Stop iteration with error */
            }
            collector->num_particles[collector->count] = num_particles;
            collector->count++;
        }
        H5Aclose(attr_id);
    } else {
        /* numParticles attribute is missing - file format error */
        pmd_log(PMD_LOG_ERROR, "Species '%s' is missing required 'numParticles' attribute\n", name);
        H5Gclose(species_group_id);
        collector->status = PMD_ERROR_FILE_FORMAT;
        return -1;  /* Stop iteration with error */
    }

    H5Gclose(species_group_id);
    return 0;
}

pmd_status pmd_open_iteration(pmd_series *series, int64_t index, pmd_iteration **iter_out) {
    pmd_iteration *iter = NULL;
    pmd_status status = PMD_SUCCESS;
    char *iteration_path = NULL;
    char *particles_full_path = NULL;
    hid_t particles_group_id = -1;

    if (!series || !iter_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Allocate iteration struct */
    iter = (pmd_iteration *)calloc(1, sizeof(pmd_iteration));
    if (!iter) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    /* Initialize fields */
    iter->series = series;
    iter->iteration_index = index;
    iter->file_id = -1;
    iter->iteration_group_id = -1;

    /* Construct iteration group path */
    iteration_path = replace_iteration(series->base_path, index);
    if (!iteration_path) {
        status = PMD_ERROR_OUT_OF_MEMORY;
        goto cleanup;
    }

    if (series->iteration_encoding == PMD_GROUP_BASED) {
        /* GROUP_BASED: borrow file_id from series, open iteration group */
        iter->file_id = series->file_id;  /* Borrowed reference */
        iter->iteration_group_id = H5Gopen(series->file_id, iteration_path, H5P_DEFAULT);

        if (iter->iteration_group_id < 0) {
            status = PMD_ERROR_INVALID_ITERATION;
            goto cleanup;
        }

    } else {  /* PMD_FILE_BASED */
        /* FILE_BASED: open file for this iteration, then open group within it */
        char *filename = replace_iteration(series->iteration_format, index);
        if (!filename) {
            status = PMD_ERROR_OUT_OF_MEMORY;
            goto cleanup;
        }
        char full_path[PMD_PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s" PMD_PATH_SEP "%s", series->directory, filename);
        free(filename);

        iter->file_id = H5Fopen(full_path, H5F_ACC_RDONLY, H5P_DEFAULT);
        if (iter->file_id < 0) {
            status = PMD_ERROR_FILE_NOT_FOUND;
            goto cleanup;
        }

        iter->iteration_group_id = H5Gopen(iter->file_id, iteration_path, H5P_DEFAULT);
        if (iter->iteration_group_id < 0) {
            status = PMD_ERROR_INVALID_ITERATION;
            goto cleanup;
        }
    }

    /* Read iteration metadata from iteration group (with defaults for non-compliant files) */
    if (attribute_exists(iter->iteration_group_id, "time") > 0) {
        status = read_double_attribute(iter->iteration_group_id, "time", &iter->time);
        if (status != PMD_SUCCESS) goto cleanup;
    } else {
        pmd_log(PMD_LOG_WARNING, "Missing 'time' attribute in iteration %lld, defaulting to 0.0\n",
                (long long)index);
        iter->time = 0.0;
    }

    if (attribute_exists(iter->iteration_group_id, "dt") > 0) {
        status = read_double_attribute(iter->iteration_group_id, "dt", &iter->dt);
        if (status != PMD_SUCCESS) goto cleanup;
    } else {
        pmd_log(PMD_LOG_WARNING, "Missing 'dt' attribute in iteration %lld, defaulting to 0.0\n",
                (long long)index);
        iter->dt = 0.0;
    }

    /* timeUnitSI is optional */
    if (attribute_exists(iter->iteration_group_id, "timeUnitSI") > 0) {
        read_double_attribute(iter->iteration_group_id, "timeUnitSI", &iter->time_unit_si);
    } else {
        iter->time_unit_si = 1.0;  /* Default: already in SI */
    }

    /* Handle case when particlesPath is undefined */
    if (series->_particles_path == NULL) {
        /* particlesPath attribute is not present - file has no particles */
        iter->num_species = 0;
        iter->species_names = NULL;
        iter->num_particles = NULL;

        /* Use normal cleanup path for consistency */
        free(iteration_path);
        free(particles_full_path);  /* NULL is safe to free */

        *iter_out = iter;
        return PMD_SUCCESS;
    }

    /* Construct particles path and open particles group */
    particles_full_path = (char *)malloc(strlen(series->_particles_path) + 1);
    strcpy(particles_full_path, series->_particles_path);

    /* Wrap in block scope to avoid C++ goto restrictions */
    {
        /* Remove trailing slash if present */
        size_t len = strlen(particles_full_path);
        if (len > 0 && particles_full_path[len-1] == '/') {
            particles_full_path[len-1] = '\0';
        }

        /* Check if particles group exists */
        if (!H5Lexists(iter->iteration_group_id, particles_full_path, H5P_DEFAULT)) {
            /* particlesPath is defined but group doesn't exist */
            status = PMD_ERROR_FILE_FORMAT;
            goto cleanup;
        }

        /* Check that particles is a group, not a dataset */
        H5O_info2_t obj_info;
        if (H5Oget_info_by_name(iter->iteration_group_id, particles_full_path, &obj_info,
                                H5O_INFO_BASIC, H5P_DEFAULT) >= 0) {
            if (obj_info.type != H5O_TYPE_GROUP) {
                /* particlesPath points to a dataset, not a group */
                status = PMD_ERROR_FILE_FORMAT;
                goto cleanup;
            }
        }

        /* Open particles group relative to iteration group */
        particles_group_id = H5Gopen(iter->iteration_group_id, particles_full_path, H5P_DEFAULT);
        if (particles_group_id < 0) {
            status = PMD_ERROR_FILE_FORMAT;
            goto cleanup;
        }

        /* First pass: count species */
        iter->num_species = 0;
        H5Literate(particles_group_id, H5_INDEX_NAME, H5_ITER_NATIVE, NULL,
                   count_species_iteration_callback, &iter->num_species);

        /* Allocate arrays for species */
        iter->species_names = (char **)calloc(iter->num_species, sizeof(char *));
        iter->num_particles = (int64_t *)calloc(iter->num_species, sizeof(int64_t));

        if (!iter->species_names || !iter->num_particles) {
            status = PMD_ERROR_OUT_OF_MEMORY;
            goto cleanup;
        }

        /* Second pass: collect species data */
        species_collector collector = {iter->species_names, iter->num_particles, 0, PMD_SUCCESS};
        herr_t iter_result = H5Literate(particles_group_id, H5_INDEX_NAME, H5_ITER_NATIVE, NULL,
                                         collect_species_iteration_callback, &collector);
        if (iter_result < 0) {
            /* Callback returned an error */
            status = collector.status;
            goto cleanup;
        }

        H5Gclose(particles_group_id);
        free(iteration_path);
        free(particles_full_path);

        *iter_out = iter;
        return PMD_SUCCESS;
    }

cleanup:
    if (particles_group_id >= 0) H5Gclose(particles_group_id);
    free(iteration_path);
    free(particles_full_path);

    if (iter) {
        /* Don't close file_id for GROUP_BASED (it's owned by series) */
        if (series->iteration_encoding == PMD_FILE_BASED && iter->file_id >= 0) {
            H5Fclose(iter->file_id);
        }
        if (iter->iteration_group_id >= 0) {
            H5Gclose(iter->iteration_group_id);
        }
        free(iter);
    }

    return status;
}

pmd_status pmd_close_iteration(pmd_iteration *iter) {
    if (!iter) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Close iteration group (always owned by iteration) */
    if (iter->iteration_group_id >= 0) {
        H5Gclose(iter->iteration_group_id);
    }

    /* Close file only for FILE_BASED (GROUP_BASED borrows from series) */
    if (iter->series->iteration_encoding == PMD_FILE_BASED && iter->file_id >= 0) {
        H5Fclose(iter->file_id);
    }

    /* Free species arrays */
    if (iter->species_names) {
        for (int i = 0; i < iter->num_species; i++) {
            free(iter->species_names[i]);
        }
        free(iter->species_names);
    }
    if (iter->num_particles != NULL) {
        free(iter->num_particles);
    }

    /* Free the struct */
    free(iter);

    return PMD_SUCCESS;
}

pmd_status pmd_get_species(pmd_iteration *iter, char ***species_names, int *count) {
    if (!iter || !species_names || !count) {
        return PMD_ERROR_NULL_POINTER;
    }

    *species_names = iter->species_names;
    *count = iter->num_species;
    return PMD_SUCCESS;
}

pmd_status pmd_get_num_particles(pmd_iteration *iter, const char *species, int64_t *count) {
    if (!iter || !species || !count) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Search for species in cached list */
    for (int i = 0; i < iter->num_species; i++) {
        if (strcmp(iter->species_names[i], species) == 0) {
            *count = iter->num_particles[i];
            return PMD_SUCCESS;
        }
    }

    return PMD_ERROR_INVALID_SPECIES;
}

/* =========================================================================
 * Series Metadata Operations Implementation
 * ========================================================================= */

/**
 * Get particlesPath (returns copy of cached value from series)
 */
pmd_status pmd_get_particles_path(pmd_series *series, char **value_out) {
    if (!series || !value_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    if (series->_particles_path == NULL) {
        return PMD_ERROR;  /* Attribute not set */
    }

    *value_out = strdup(series->_particles_path);
    if (!*value_out) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    return PMD_SUCCESS;
}

/**
 * Get meshesPath (returns copy of cached value from series)
 */
pmd_status pmd_get_meshes_path(pmd_series *series, char **value_out) {
    if (!series || !value_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    if (series->_meshes_path == NULL) {
        return PMD_ERROR;  /* Attribute not set */
    }

    *value_out = strdup(series->_meshes_path);
    if (!*value_out) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    return PMD_SUCCESS;
}

/**
 * Helper function to read a root-level string attribute from the series file
 * Handles both GROUP_BASED (file_id) and FILE_BASED (need to open file) cases
 */
static pmd_status read_series_root_attribute(pmd_series *series, const char *attr_name, char **value_out) {
    hid_t file_id = -1;
    pmd_status status;
    int should_close_file = 0;

    if (!series || !attr_name || !value_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Get file handle */
    if (series->iteration_encoding == PMD_GROUP_BASED) {
        /* Use series file_id directly */
        file_id = series->file_id;
    } else {
        /* FILE_BASED: need to open one of the files to read root attributes */
        /* Get first available iteration */
        int64_t *iterations;
        int num_iterations;
        status = pmd_get_iterations(series, &iterations, &num_iterations);
        if (status != PMD_SUCCESS || num_iterations == 0) {
            return PMD_ERROR_FILE_NOT_FOUND;
        }

        /* Use the first iteration's file */
        char *filename = replace_iteration(series->iteration_format, iterations[0]);
        if (!filename) {
            return PMD_ERROR_OUT_OF_MEMORY;
        }
        char full_path[PMD_PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s" PMD_PATH_SEP "%s", series->directory, filename);
        free(filename);

        file_id = H5Fopen(full_path, H5F_ACC_RDONLY, H5P_DEFAULT);
        if (file_id < 0) {
            return PMD_ERROR_FILE_NOT_FOUND;
        }
        should_close_file = 1;
    }

    /* Check if attribute exists */
    if (attribute_exists(file_id, attr_name) <= 0) {
        if (should_close_file) H5Fclose(file_id);
        return PMD_ERROR;  /* Attribute doesn't exist */
    }

    /* Read the attribute */
    status = read_string_attribute(file_id, attr_name, value_out);

    /* Clean up */
    if (should_close_file) {
        H5Fclose(file_id);
    }

    return status;
}

pmd_status pmd_get_openpmd_extension(pmd_series *series, char **value_out) {
    return read_series_root_attribute(series, "openPMDextension", value_out);
}

pmd_status pmd_get_author(pmd_series *series, char **value_out) {
    return read_series_root_attribute(series, "author", value_out);
}

pmd_status pmd_get_software(pmd_series *series, char **value_out) {
    return read_series_root_attribute(series, "software", value_out);
}

pmd_status pmd_get_software_version(pmd_series *series, char **value_out) {
    return read_series_root_attribute(series, "softwareVersion", value_out);
}

pmd_status pmd_get_date(pmd_series *series, char **value_out) {
    return read_series_root_attribute(series, "date", value_out);
}

pmd_status pmd_get_software_dependencies(pmd_series *series, char **value_out) {
    return read_series_root_attribute(series, "softwareDependencies", value_out);
}

pmd_status pmd_get_machine(pmd_series *series, char **value_out) {
    return read_series_root_attribute(series, "machine", value_out);
}

pmd_status pmd_get_comment(pmd_series *series, char **value_out) {
    return read_series_root_attribute(series, "comment", value_out);
}

/* =========================================================================
 * Constants
 * ========================================================================= */

 /* Exact speed of light (m/s) */
 #define CLIGHT 299792456

 /* Conversion from eV/c^2 to kg (CODATA recommended value 2022) */
 #define EV_C2_TO_SI 1.782661921e-36

/* Conversion factor from eV/c to SI (kg⋅m/s) */
#define EV_C_TO_SI (EV_C2_TO_SI*CLIGHT)

/* =========================================================================
 * Particle Data Operations Implementation
 * ========================================================================= */

pmd_status pmd_allocate_particle_group(pmd_iteration *iter, const char *species,
                                        particle_group **pg_out) {
    particle_group *pg = NULL;
    pmd_status status;
    int64_t num_particles;

    if (!iter || !species || !pg_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Get particle count */
    status = pmd_get_num_particles(iter, species, &num_particles);
    if (status != PMD_SUCCESS) {
        return status;
    }

    /* Allocate particle_group */
    pg = (particle_group *)calloc(1, sizeof(particle_group));
    if (!pg) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    /* Set metadata */
    pg->num_particles = num_particles;
    pg->species_type = strdup(species);

    /* Allocate all arrays */
    pg->x = (double *)calloc(num_particles, sizeof(double));
    pg->y = (double *)calloc(num_particles, sizeof(double));
    pg->z = (double *)calloc(num_particles, sizeof(double));
    pg->t = (double *)calloc(num_particles, sizeof(double));
    pg->px = (double *)calloc(num_particles, sizeof(double));
    pg->py = (double *)calloc(num_particles, sizeof(double));
    pg->pz = (double *)calloc(num_particles, sizeof(double));
    pg->weight = (double *)calloc(num_particles, sizeof(double));
    pg->status = (int64_t *)calloc(num_particles, sizeof(int64_t));
    pg->id = (int64_t *)calloc(num_particles, sizeof(int64_t));

    /* Check allocation */
    if (!pg->species_type || !pg->x || !pg->y || !pg->z || !pg->t ||
        !pg->px || !pg->py || !pg->pz || !pg->weight || !pg->status || !pg->id) {
        pmd_free_particle_group(pg);
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    *pg_out = pg;
    return PMD_SUCCESS;
}

pmd_status pmd_read_particle_group(pmd_iteration *iter, const char *species,
                                    particle_group *pg, particle_group_read_info *read_info) {
    hid_t particles_group_id = -1;
    hid_t species_group_id = -1;
    pmd_status status = PMD_SUCCESS;
    char *particles_path = NULL;

    if (!iter || !species || !pg) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Initialize read_info if provided */
    if (read_info) {
        read_info->t_present = false;
        read_info->px_present = false;
        read_info->py_present = false;
        read_info->pz_present = false;
        read_info->weight_present = false;
        read_info->status_present = false;
        read_info->id_present = false;
    }

    /* Verify species exists and get particle count */
    int64_t num_particles;
    status = pmd_get_num_particles(iter, species, &num_particles);
    if (status != PMD_SUCCESS) {
        return status;
    }

    /* Check if particlesPath is defined */
    if (iter->series->_particles_path == NULL) {
        return PMD_ERROR_INVALID_SPECIES;
    }

    /* Suppress HDF5 error messages for expected failures */
    H5Eset_auto(H5E_DEFAULT, NULL, NULL);

    /* Construct path to particles group */
    particles_path = strdup(iter->series->_particles_path);
    if (!particles_path) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }
    size_t len = strlen(particles_path);
    if (len > 0 && particles_path[len-1] == '/') {
        particles_path[len-1] = '\0';
    }

    /* Open particles group */
    particles_group_id = H5Gopen(iter->iteration_group_id, particles_path, H5P_DEFAULT);
    free(particles_path);

    if (particles_group_id < 0) {
        return PMD_ERROR_HDF5;
    }

    /* Open species group */
    species_group_id = H5Gopen(particles_group_id, species, H5P_DEFAULT);
    H5Gclose(particles_group_id);

    if (species_group_id < 0) {
        return PMD_ERROR_INVALID_SPECIES;
    }

    /* Read required position components (in SI units: meters) */
    /* First check that position is a group, not a dataset */
    if (record_exists(species_group_id, "position")) {
        H5O_info2_t obj_info;
        if (H5Oget_info_by_name(species_group_id, "position", &obj_info, H5O_INFO_BASIC, H5P_DEFAULT) >= 0) {
            if (obj_info.type != H5O_TYPE_GROUP) {
                pmd_log(PMD_LOG_ERROR, "pmd_read_particle_group: 'position' is not a group\n");
                status = PMD_ERROR_FILE_FORMAT;
                goto cleanup;
            }
        }
    }

    if (pg->x && record_exists(species_group_id, "position/x")) {
        status = read_double_record(species_group_id, "position/x", pg->x, num_particles, 1.0);
        if (status != PMD_SUCCESS) goto cleanup;
    }

    if (pg->y && record_exists(species_group_id, "position/y")) {
        status = read_double_record(species_group_id, "position/y", pg->y, num_particles, 1.0);
        if (status != PMD_SUCCESS) goto cleanup;
    }

    if (pg->z && record_exists(species_group_id, "position/z")) {
        status = read_double_record(species_group_id, "position/z", pg->z, num_particles, 1.0);
        if (status != PMD_SUCCESS) goto cleanup;
    }

    /* Read optional time (in SI units: seconds) */
    if (pg->t && record_exists(species_group_id, "time")) {
        status = read_double_record(species_group_id, "time", pg->t, num_particles, 1.0);
        if (status != PMD_SUCCESS) goto cleanup;
        if (read_info) read_info->t_present = true;
    } else if (pg->t) {
        for (int64_t i = 0; i < num_particles; i++) pg->t[i] = NAN;
    }

    /* Read optional momentum components (convert from SI to eV/c) */
    /* Check that momentum is a group, not a dataset */
    if (record_exists(species_group_id, "momentum")) {
        H5O_info2_t obj_info;
        if (H5Oget_info_by_name(species_group_id, "momentum", &obj_info, H5O_INFO_BASIC, H5P_DEFAULT) >= 0) {
            if (obj_info.type != H5O_TYPE_GROUP) {
                pmd_log(PMD_LOG_ERROR, "pmd_read_particle_group: 'momentum' is not a group\n");
                status = PMD_ERROR_FILE_FORMAT;
                goto cleanup;
            }
        }
    }

    if (pg->px && record_exists(species_group_id, "momentum/x")) {
        status = read_double_record(species_group_id, "momentum/x", pg->px, num_particles, 1.0 / EV_C_TO_SI);
        if (status != PMD_SUCCESS) goto cleanup;
        if (read_info) read_info->px_present = true;
    } else if (pg->px) {
        for (int64_t i = 0; i < num_particles; i++) pg->px[i] = NAN;
    }

    if (pg->py && record_exists(species_group_id, "momentum/y")) {
        status = read_double_record(species_group_id, "momentum/y", pg->py, num_particles, 1.0 / EV_C_TO_SI);
        if (status != PMD_SUCCESS) goto cleanup;
        if (read_info) read_info->py_present = true;
    } else if (pg->py) {
        for (int64_t i = 0; i < num_particles; i++) pg->py[i] = NAN;
    }

    if (pg->pz && record_exists(species_group_id, "momentum/z")) {
        status = read_double_record(species_group_id, "momentum/z", pg->pz, num_particles, 1.0 / EV_C_TO_SI);
        if (status != PMD_SUCCESS) goto cleanup;
        if (read_info) read_info->pz_present = true;
    } else if (pg->pz) {
        for (int64_t i = 0; i < num_particles; i++) pg->pz[i] = NAN;
    }

    /* Read optional weight (dimensionless) */
    if (pg->weight && record_exists(species_group_id, "weight")) {
        status = read_double_record(species_group_id, "weight", pg->weight, num_particles, 1.0);
        if (status != PMD_SUCCESS) goto cleanup;
        if (read_info) read_info->weight_present = true;
    } else if (pg->weight) {
        for (int64_t i = 0; i < num_particles; i++) pg->weight[i] = 1.0;
    }

    /* Read optional status */
    if (pg->status && record_exists(species_group_id, "particleStatus")) {
        status = read_int64_record(species_group_id, "particleStatus", pg->status, num_particles);
        if (status != PMD_SUCCESS) goto cleanup;
        if (read_info) read_info->status_present = true;
    } else if (pg->status) {
        for (int64_t i = 0; i < num_particles; i++) pg->status[i] = 1;
    }

    /* Read optional id */
    if (pg->id && record_exists(species_group_id, "id")) {
        status = read_int64_record(species_group_id, "id", pg->id, num_particles);
        if (status != PMD_SUCCESS) goto cleanup;
        if (read_info) read_info->id_present = true;
    } else if (pg->id) {
        for (int64_t i = 0; i < num_particles; i++) pg->id[i] = i;
    }

cleanup:
    H5Gclose(species_group_id);
    return status;
}

pmd_status pmd_free_particle_group(particle_group *pg) {
    if (!pg) {
        return PMD_ERROR_NULL_POINTER;
    }

    free(pg->species_type);
    free(pg->x);
    free(pg->y);
    free(pg->z);
    free(pg->t);
    free(pg->px);
    free(pg->py);
    free(pg->pz);
    free(pg->weight);
    free(pg->status);
    free(pg->id);
    memset(pg, 0, sizeof(particle_group));
    free(pg);

    return PMD_SUCCESS;
}

pmd_status pmd_write_particle_group(pmd_iteration *iter, const char *species,
                                     const particle_group *pg) {
    /* Not yet implemented */
    pmd_log(PMD_LOG_ERROR, "pmd_write_particle_group not yet implemented\n");
    return PMD_ERROR;
}

/**
 * Check if a record (dataset or group) exists
 * Returns: 1 if exists, 0 if not
 */
static int record_exists(hid_t group_id, const char *name) {
    htri_t exists = H5Lexists(group_id, name, H5P_DEFAULT);
    return (exists > 0) ? 1 : 0;
}

/**
 * Validate that an attribute has the expected type class
 * Returns PMD_SUCCESS if type matches, PMD_ERROR_FILE_FORMAT if wrong type
 */
static pmd_status validate_attribute_type(hid_t attr_id, H5T_class_t expected_class) {
    hid_t attr_type = H5Aget_type(attr_id);
    if (attr_type < 0) {
        return PMD_ERROR_HDF5;
    }

    H5T_class_t type_class = H5Tget_class(attr_type);
    H5Tclose(attr_type);

    if (type_class != expected_class) {
        pmd_log(PMD_LOG_ERROR, "validate_attribute_type: Attribute has type class %d, expected %d\n",
                type_class, expected_class);
        return PMD_ERROR_FILE_FORMAT;
    }

    return PMD_SUCCESS;
}

/**
 * Read unitSI attribute from a record component
 * Returns PMD_SUCCESS on success, error code on failure
 * Sets unit_si_out to conversion factor, or 1.0 if not present
 */
static pmd_status read_unit_si(hid_t loc_id, double *unit_si_out) {
    double unit_si = 1.0;
    hid_t attr_id;
    herr_t h5_status;
    pmd_status status;

    if (!unit_si_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    if (attribute_exists(loc_id, "unitSI") > 0) {
        attr_id = H5Aopen(loc_id, "unitSI", H5P_DEFAULT);
        if (attr_id < 0) {
            return PMD_ERROR_HDF5;
        }

        /* Check attribute type - accept both float and integer */
        hid_t attr_type = H5Aget_type(attr_id);
        if (attr_type < 0) {
            H5Aclose(attr_id);
            return PMD_ERROR_HDF5;
        }

        H5T_class_t type_class = H5Tget_class(attr_type);
        H5Tclose(attr_type);

        if (type_class == H5T_FLOAT) {
            /* Read as double */
            h5_status = H5Aread(attr_id, H5T_NATIVE_DOUBLE, &unit_si);
        } else if (type_class == H5T_INTEGER) {
            /* Read as int64 and convert to double */
            int64_t unit_si_int;
            h5_status = H5Aread(attr_id, H5T_NATIVE_INT64, &unit_si_int);
            if (h5_status >= 0) {
                unit_si = (double)unit_si_int;
            }
        } else {
            /* Wrong type */
            pmd_log(PMD_LOG_ERROR, "read_unit_si: 'unitSI' attribute has type class %d, expected float or integer\n", type_class);
            H5Aclose(attr_id);
            return PMD_ERROR_FILE_FORMAT;
        }

        H5Aclose(attr_id);

        if (h5_status < 0) {
            return PMD_ERROR_HDF5;
        }
    }

    *unit_si_out = unit_si;
    return PMD_SUCCESS;
}

/**
 * Generic record reader - reads from dataset or constant group
 * Returns: PMD_SUCCESS on success, error code on failure
 * Also returns unit_si via pointer if not NULL
 */
static pmd_status read_record_generic(hid_t group_id, const char *name,
                                       void *array, hid_t h5_type, size_t elem_size,
                                       int64_t num_particles, double *unit_si_out) {
    hid_t dataset_id, group_id_local, attr_id;
    double unit_si = 1.0;

    if (!array) return PMD_ERROR_NULL_POINTER;

    /* Try to open as dataset first */
    dataset_id = H5Dopen(group_id, name, H5P_DEFAULT);
    if (dataset_id >= 0) {
        /* Validate dataset dimensions before reading */
        hid_t dataspace_id = H5Dget_space(dataset_id);
        if (dataspace_id < 0) {
            H5Dclose(dataset_id);
            return PMD_ERROR_HDF5;
        }

        /* Check rank (must be 1D) */
        int ndims = H5Sget_simple_extent_ndims(dataspace_id);
        if (ndims != 1) {
            H5Sclose(dataspace_id);
            H5Dclose(dataset_id);
            pmd_log(PMD_LOG_ERROR, "Dataset '%s' has rank %d, expected 1\n", name, ndims);
            return PMD_ERROR_FILE_FORMAT;
        }

        /* Check size matches num_particles */
        hsize_t dims[1];
        H5Sget_simple_extent_dims(dataspace_id, dims, NULL);
        if (dims[0] != (hsize_t)num_particles) {
            H5Sclose(dataspace_id);
            H5Dclose(dataset_id);
            pmd_log(PMD_LOG_ERROR, "Dataset '%s' has size %llu, expected %lld\n",
                    name, (unsigned long long)dims[0], (long long)num_particles);
            return PMD_ERROR_FILE_FORMAT;
        }

        H5Sclose(dataspace_id);

        /* Read the array */
        if (H5Dread(dataset_id, h5_type, H5S_ALL, H5S_ALL, H5P_DEFAULT, array) < 0) {
            H5Dclose(dataset_id);
            return PMD_ERROR_HDF5;
        }

        /* Read unitSI if requested */
        if (unit_si_out) {
            pmd_status status = read_unit_si(dataset_id, &unit_si);
            if (status != PMD_SUCCESS) {
                H5Dclose(dataset_id);
                return status;
            }
            *unit_si_out = unit_si;
        }

        H5Dclose(dataset_id);
        return PMD_SUCCESS;
    }

    /* Try as constant record (group with 'value' attribute) */
    group_id_local = H5Gopen(group_id, name, H5P_DEFAULT);
    if (group_id_local >= 0) {
        /* Check if 'value' attribute exists before trying to open it */
        if (attribute_exists(group_id_local, "value") <= 0) {
            /* Group exists but no 'value' attribute - format error */
            pmd_log(PMD_LOG_ERROR, "read_record_generic: Constant record '%s' missing 'value' attribute\n", name);
            H5Gclose(group_id_local);
            return PMD_ERROR_FILE_FORMAT;
        }

        attr_id = H5Aopen(group_id_local, "value", H5P_DEFAULT);
        if (attr_id >= 0) {
            /* Validate attribute type based on expected h5_type */
            H5T_class_t expected_class;
            if (h5_type == H5T_NATIVE_DOUBLE) {
                expected_class = H5T_FLOAT;
            } else if (h5_type == H5T_NATIVE_INT64) {
                expected_class = H5T_INTEGER;
            } else {
                /* Unknown type - skip validation */
                expected_class = H5T_NO_CLASS;
            }

            if (expected_class != H5T_NO_CLASS) {
                pmd_status type_status = validate_attribute_type(attr_id, expected_class);
                if (type_status != PMD_SUCCESS) {
                    pmd_log(PMD_LOG_ERROR, "read_record_generic: Constant record '%s' has wrong type for 'value' attribute\n", name);
                    H5Aclose(attr_id);
                    H5Gclose(group_id_local);
                    return type_status;
                }
            }

            /* Validate that value is scalar, not an array */
            hid_t attr_space = H5Aget_space(attr_id);
            if (attr_space < 0) {
                H5Aclose(attr_id);
                H5Gclose(group_id_local);
                return PMD_ERROR_HDF5;
            }

            H5S_class_t space_type = H5Sget_simple_extent_type(attr_space);
            H5Sclose(attr_space);

            if (space_type != H5S_SCALAR) {
                /* value must be a scalar, not an array */
                pmd_log(PMD_LOG_ERROR, "read_record_generic: Constant record '%s' has array 'value', expected scalar\n", name);
                H5Aclose(attr_id);
                H5Gclose(group_id_local);
                return PMD_ERROR_FILE_FORMAT;
            }

            char constant_buffer[16];  /* Large enough for double or int64_t */
            if (H5Aread(attr_id, h5_type, constant_buffer) >= 0) {
                /* Read unitSI if requested */
                if (unit_si_out) {
                    pmd_status status = read_unit_si(group_id_local, &unit_si);
                    if (status != PMD_SUCCESS) {
                        H5Aclose(attr_id);
                        H5Gclose(group_id_local);
                        return status;
                    }
                    *unit_si_out = unit_si;
                }

                /* Fill array with constant value */
                char *array_ptr = (char *)array;
                for (int64_t i = 0; i < num_particles; i++) {
                    memcpy(array_ptr + i * elem_size, constant_buffer, elem_size);
                }

                H5Aclose(attr_id);
                H5Gclose(group_id_local);
                return PMD_SUCCESS;
            }
            H5Aclose(attr_id);
        }
        H5Gclose(group_id_local);
    }

    return PMD_ERROR_HDF5;
}

/**
 * Read a double array record with SI conversion
 */
static pmd_status read_double_record(hid_t group_id, const char *name, double *array,
                                      int64_t num_particles, double unit_multiplier) {
    double unit_si;
    pmd_status status;

    status = read_record_generic(group_id, name, array, H5T_NATIVE_DOUBLE,
                                  sizeof(double), num_particles, &unit_si);

    if (status != PMD_SUCCESS) {
        return status;
    }

    /* Apply SI unit conversion with multiplier */
    double conversion = unit_si * unit_multiplier;
    if (conversion != 1.0) {
        for (int64_t i = 0; i < num_particles; i++) {
            array[i] *= conversion;
        }
    }

    return PMD_SUCCESS;
}

/**
 * Read an int64 array record
 */
static pmd_status read_int64_record(hid_t group_id, const char *name, int64_t *array,
                                     int64_t num_particles) {
    return read_record_generic(group_id, name, array, H5T_NATIVE_INT64,
                                sizeof(int64_t), num_particles, NULL);
}

#endif /* PARCEL_IMPLEMENTATION */

#endif /* PARCEL_H */