/* =========================================================================
    Parcel - A C library for reading OpenPMD particle and mesh data
    Copyright (c) 2025 Christopher M. Pierce
    SPDX-License-Identifier: BSD-3-Clause
========================================================================= */

#ifndef PARCEL_H
#define PARCEL_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <hdf5.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Library Version
 * ========================================================================= */

/* Parcel library version */
#define PARCEL_VERSION_STRING "0.1.0"

/* OpenPMD version that this library writes */
#define OPENPMD_VERSION_STRING "2.0.0"

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

/**
 * pmd_access_mode - File access mode for opening series
 */
typedef enum {
    PMD_RDONLY,                      /* Read-only access */
    PMD_RDWR,                        /* Read-write access to existing file */
    PMD_TRUNC,                       /* Create new file, truncate if exists */
    PMD_EXCL                         /* Create new file, fail if exists */
} pmd_access_mode;

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
 * pmd_unit_dimension - Physical dimensionality of a record in SI base units
 *
 * Represents the powers of the 7 SI base dimensions:
 * - length (L): meter
 * - mass (M): kilogram
 * - time (T): second
 * - current (I): ampere
 * - temperature (theta): kelvin
 * - amount (N): mole
 * - intensity (J): candela
 *
 * Example: For velocity (m/s), use: {1.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0}
 * Example: For force (N = kg*m/s^2), use: {1.0, 1.0, -2.0, 0.0, 0.0, 0.0, 0.0}
 */
typedef struct {
    double length;                  /* L - meter */
    double mass;                    /* M - kilogram */
    double time;                    /* T - second */
    double current;        /* I - ampere */
    double temperature;             /* theta - kelvin */
    double amount;     /* N - mole */
    double intensity;      /* J - candela */
} pmd_unit_dimension;

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

    /* Position offset */
    double *x_offset;            /* x position offset (m) */
    double *y_offset;            /* y position offset (m) */
    double *z_offset;            /* z position offset (m) */

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

/* Forward declaration for circular reference */
typedef struct pmd_iteration pmd_iteration;

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

    /* Access mode used to open this series */
    pmd_access_mode access_mode;

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

    /* Metadata attributes (private - use accessor functions) */
    char *_author;
    char *_software;
    char *_software_version;
    char *_software_dependencies;
    char *_machine;
    char *_comment;
    char *_date;

    /* Cached metadata */
    int num_iterations;               /* -1 if not enumerated yet */
    int64_t *iteration_indices;       /* Array of available iterations */

    /* Track open iterations (for FILE_BASED to prevent multiple file handles) */
    pmd_iteration **open_iterations;  /* Array of pointers to open iteration handles */
    int num_open_iterations;          /* Number of currently open iterations */
    int open_iterations_capacity;     /* Allocated capacity for open_iterations array */
} pmd_series;

/**
 * pmd_iteration - Handle for a single iteration in an OpenPMD series
 *
 * Represents a specific timestep with its metadata and particle data
 */
struct pmd_iteration {
    pmd_series *series;               /* Parent series */
    int64_t iteration_index;          /* Current iteration number */

    /* Handle to data */
    hid_t file_id;                    /* For FILE_BASED only. -1 if not used */
    hid_t iteration_group_id;
};

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
 * @param filename Path to OpenPMD file (may contain %T pattern for FILE_BASED)
 * @param series_out Output pointer to created series handle
 * @param mode Access mode (PMD_RDONLY, PMD_RDWR, PMD_TRUNC, PMD_EXCL)
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_open_series(const char *filename, pmd_series **series_out, pmd_access_mode mode);

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

/* --- Iteration Metadata Operations --- */

/**
 * Get the time attribute from an iteration
 *
 * @param iter Iteration handle
 * @param time Output pointer to time value
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_get_time(pmd_iteration *iter, double *time);

/**
 * Get the dt (time step) attribute from an iteration
 *
 * @param iter Iteration handle
 * @param dt Output pointer to dt value
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_get_dt(pmd_iteration *iter, double *dt);

/**
 * Get the timeUnitSI attribute from an iteration
 *
 * @param iter Iteration handle
 * @param time_unit_si Output pointer to timeUnitSI value
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_get_time_unit_si(pmd_iteration *iter, double *time_unit_si);

/**
 * Set the time attribute for an iteration
 *
 * @param iter Iteration handle
 * @param time Time value to set
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_set_time(pmd_iteration *iter, double time);

/**
 * Set the dt (time step) attribute for an iteration
 *
 * @param iter Iteration handle
 * @param dt Time step value to set
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_set_dt(pmd_iteration *iter, double dt);

/**
 * Set the timeUnitSI attribute for an iteration
 *
 * @param iter Iteration handle
 * @param time_unit_si TimeUnitSI value to set
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_set_time_unit_si(pmd_iteration *iter, double time_unit_si);

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
 * Get the openPMD version from the series
 *
 * @param series Series handle
 * @param major Output pointer to major version
 * @param minor Output pointer to minor version
 * @param revision Output pointer to revision version
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_get_openpmd_version(pmd_series *series, int *major, int *minor, int *revision);

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

/**
 * Set author attribute
 *
 * @param series Series handle
 * @param value Author string to write
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_set_author(pmd_series *series, const char *value);

/**
 * Set software attribute
 *
 * @param series Series handle
 * @param value Software name to write
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_set_software(pmd_series *series, const char *value);

/**
 * Set softwareVersion attribute
 *
 * @param series Series handle
 * @param value Software version to write
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_set_software_version(pmd_series *series, const char *value);

/**
 * Set softwareDependencies attribute
 *
 * @param series Series handle
 * @param value Dependencies string to write
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_set_software_dependencies(pmd_series *series, const char *value);

/**
 * Set machine attribute
 *
 * @param series Series handle
 * @param value Machine description to write
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_set_machine(pmd_series *series, const char *value);

/**
 * Set comment attribute
 *
 * @param series Series handle
 * @param value Comment string to write
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_set_comment(pmd_series *series, const char *value);

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
 * Write particle group to iteration
 *
 * @param iter Iteration handle
 * @param pg particle_group to write
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_write_particle_group(pmd_iteration *iter, const particle_group *pg);

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
 * Constants
 * ========================================================================= */

/* Exact speed of light (m/s) */
#define CLIGHT 299792456

/* Conversion from eV/c^2 to kg (CODATA recommended value 2022) */
#define EV_C2_TO_SI 1.782661921e-36

/* Conversion factor from eV/c to SI (kg⋅m/s) */
#define EV_C_TO_SI (EV_C2_TO_SI*CLIGHT)

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
    #include <sys/stat.h>  /* For stat() and S_ISDIR() */
    #include <errno.h>  /* For errno and EEXIST */
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
static pmd_status write_string_attribute(hid_t loc_id, const char *attr_name, const char *value);
static pmd_status write_double_attribute(hid_t loc_id, const char *attr_name, double value);
static unsigned int pmd_access_mode_to_hdf5(pmd_access_mode mode);
static pmd_status write_root_attributes(hid_t file_id, pmd_series *series);
static pmd_status write_iteration_attributes(hid_t group_id);
static pmd_status ensure_parent_groups(hid_t file_id, const char *path);
static pmd_status write_series_root_attributes(pmd_series *series);
static pmd_status write_double_dataset(hid_t group_id, const char *name, const double *data,
                                        int64_t num_particles, double unit_si,
                                        const pmd_unit_dimension *unit_dim, double time_offset);
static pmd_status write_int64_dataset(hid_t group_id, const char *name, const int64_t *data,
                                       int64_t num_particles,
                                       const pmd_unit_dimension *unit_dim, double time_offset);
static pmd_status write_int64_attribute(hid_t loc_id, const char *attr_name, int64_t value);

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

/**
 * Recursively create all parent directories of a file path
 *
 * @param dirpath Directory path to create
 * @return PMD_SUCCESS on success, error code otherwise
 */
static pmd_status create_directory_recursive(const char *dirpath) {
    char *path_copy;
    char *last_sep;
    pmd_status status;

    /* Handle NULL or empty path */
    if (!dirpath || dirpath[0] == '\0') {
        return PMD_SUCCESS;
    }

    /* Check if directory already exists */
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(dirpath);
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return PMD_SUCCESS;
    }
#else
    struct stat st;
    if (stat(dirpath, &st) == 0 && S_ISDIR(st.st_mode)) {
        return PMD_SUCCESS;
    }
#endif

    /* Make a copy to find parent */
    path_copy = strdup(dirpath);
    if (!path_copy) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    /* Find last path separator */
    last_sep = strrchr(path_copy, '/');
#ifdef _WIN32
    char *last_backslash = strrchr(path_copy, '\\');
    if (last_backslash && (!last_sep || last_backslash > last_sep)) {
        last_sep = last_backslash;
    }
#endif

    /* Recursively create parent directory first */
    if (last_sep && last_sep != path_copy) {
        *last_sep = '\0';
        status = create_directory_recursive(path_copy);
        if (status != PMD_SUCCESS) {
            free(path_copy);
            return status;
        }
    }

    free(path_copy);

    /* Now create this directory */
#ifdef _WIN32
    if (CreateDirectoryA(dirpath, NULL) == 0) {
        DWORD error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS) {
            return PMD_ERROR_FILE_NOT_FOUND;
        }
    }
#else
    if (mkdir(dirpath, 0755) != 0 && errno != EEXIST) {
        return PMD_ERROR_FILE_NOT_FOUND;
    }
#endif

    return PMD_SUCCESS;
}

/**
 * Create parent directory for a file path if it doesn't exist
 * Returns PMD_SUCCESS if directory exists or was created, error otherwise
 */
static pmd_status create_parent_directory(const char *filepath) {
    char *path_copy;
    char *last_sep;
    pmd_status status;

    /* Handle NULL or empty path */
    if (!filepath || filepath[0] == '\0') {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Make a copy to modify */
    path_copy = strdup(filepath);
    if (!path_copy) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    /* Find last path separator */
    last_sep = strrchr(path_copy, '/');
#ifdef _WIN32
    char *last_backslash = strrchr(path_copy, '\\');
    if (last_backslash && (!last_sep || last_backslash > last_sep)) {
        last_sep = last_backslash;
    }
#endif

    if (!last_sep || last_sep == path_copy) {
        /* No parent directory or at root */
        free(path_copy);
        return PMD_SUCCESS;
    }

    /* Terminate at last separator to get parent directory */
    *last_sep = '\0';

    /* Recursively create all parent directories */
    status = create_directory_recursive(path_copy);
    free(path_copy);
    return status;
}

/**
 * Check if parent directory of a file path exists
 * For patterns with %T, checks the real parent directory before the %T
 * Returns 1 if exists, 0 if not
 */
static int parent_directory_exists(const char *filepath) {
    char *path_copy;
    char *first_T;
    char *last_sep;
    int exists = 0;

    /* Handle NULL or empty path */
    if (!filepath || filepath[0] == '\0') {
        return 0;
    }

    /* Make a copy to modify */
    path_copy = strdup(filepath);
    if (!path_copy) {
        return 0;
    }

    /* If pattern contains %T, find the parent directory before the first %T */
    first_T = strstr(path_copy, "%T");
    if (first_T) {
        /* Find last separator before %T */
        last_sep = first_T;
        while (last_sep > path_copy && *last_sep != '/' && *last_sep != '\\') {
            last_sep--;
        }

        if (last_sep == path_copy || (*last_sep != '/' && *last_sep != '\\')) {
            /* %T is at root level - current directory */
            free(path_copy);
            return 1;
        }

        /* Terminate at last separator before %T */
        *last_sep = '\0';
    } else {
        /* No %T pattern - find last separator in path */
        last_sep = strrchr(path_copy, '/');
#ifdef _WIN32
        char *last_backslash = strrchr(path_copy, '\\');
        if (last_backslash && (!last_sep || last_backslash > last_sep)) {
            last_sep = last_backslash;
        }
#endif

        if (!last_sep) {
            /* No directory separator - file is in current directory */
            free(path_copy);
            return 1;
        }

        /* Terminate string at last separator to get parent directory */
        *last_sep = '\0';
    }

    /* Empty string means root directory */
    if (path_copy[0] == '\0') {
        free(path_copy);
        return 1;
    }

    /* Check if directory exists */
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path_copy);
    exists = (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    exists = (stat(path_copy, &st) == 0 && S_ISDIR(st.st_mode));
#endif

    free(path_copy);
    return exists;
}

static pmd_status write_string_attribute(hid_t loc_id, const char *attr_name, const char *value) {
    hid_t aspace_id, atype_id, attr_id;
    herr_t status;

    /* Create scalar dataspace */
    aspace_id = H5Screate(H5S_SCALAR);
    if (aspace_id < 0) {
        return PMD_ERROR_HDF5;
    }

    /* Create fixed-length string datatype */
    atype_id = H5Tcopy(H5T_C_S1);
    if (atype_id < 0) {
        H5Sclose(aspace_id);
        return PMD_ERROR_HDF5;
    }

    /* Set size to exact length of string (including null terminator) */
    status = H5Tset_size(atype_id, strlen(value) + 1);
    if (status < 0) {
        H5Tclose(atype_id);
        H5Sclose(aspace_id);
        return PMD_ERROR_HDF5;
    }

    /* Create or overwrite attribute */
    if (H5Aexists(loc_id, attr_name) > 0) {
        H5Adelete(loc_id, attr_name);
    }

    attr_id = H5Acreate2(loc_id, attr_name, atype_id, aspace_id, H5P_DEFAULT, H5P_DEFAULT);
    if (attr_id < 0) {
        H5Tclose(atype_id);
        H5Sclose(aspace_id);
        return PMD_ERROR_HDF5;
    }

    /* Write attribute (pass string pointer directly for fixed-length strings) */
    status = H5Awrite(attr_id, atype_id, value);
    if (status < 0) {
        H5Aclose(attr_id);
        H5Tclose(atype_id);
        H5Sclose(aspace_id);
        return PMD_ERROR_HDF5;
    }

    H5Aclose(attr_id);
    H5Tclose(atype_id);
    H5Sclose(aspace_id);
    return PMD_SUCCESS;
}

static unsigned int pmd_access_mode_to_hdf5(pmd_access_mode mode) {
    switch (mode) {
        case PMD_RDONLY:
            return H5F_ACC_RDONLY;
        case PMD_RDWR:
            return H5F_ACC_RDWR;
        case PMD_TRUNC:
            return H5F_ACC_TRUNC;
        case PMD_EXCL:
            return H5F_ACC_EXCL;
        default:
            return H5F_ACC_RDONLY;
    }
}

/**
 * Write all root-level attributes to a file
 * Includes OpenPMD required attributes and optional user metadata
 */
static pmd_status write_root_attributes(hid_t file_id, pmd_series *series) {
    pmd_status status;

    /* Write openPMD version */
    status = write_string_attribute(file_id, "openPMD", OPENPMD_VERSION_STRING);
    if (status != PMD_SUCCESS) return status;

    /* Write parcel library version */
    status = write_string_attribute(file_id, "parcel", PARCEL_VERSION_STRING);
    if (status != PMD_SUCCESS) return status;

    /* Write openPMDextension */
    status = write_string_attribute(file_id, "openPMDextension", "BeamPhysics;SpeciesType");
    if (status != PMD_SUCCESS) return status;

    /* Write basePath */
    status = write_string_attribute(file_id, "basePath", series->base_path);
    if (status != PMD_SUCCESS) return status;

    /* Write iterationFormat */
    status = write_string_attribute(file_id, "iterationFormat", series->iteration_format);
    if (status != PMD_SUCCESS) return status;

    /* Write iterationEncoding */
    const char *encoding_str = (series->iteration_encoding == PMD_FILE_BASED) ? "fileBased" : "groupBased";
    status = write_string_attribute(file_id, "iterationEncoding", encoding_str);
    if (status != PMD_SUCCESS) return status;

    /* Write particlesPath if set */
    char *particles_path_value = NULL;
    if (pmd_get_particles_path(series, &particles_path_value) == PMD_SUCCESS) {
        status = write_string_attribute(file_id, "particlesPath", particles_path_value);
        free(particles_path_value);
        if (status != PMD_SUCCESS) return status;
    }

    /* Write meshesPath if set */
    if (series->_meshes_path) {
        status = write_string_attribute(file_id, "meshesPath", series->_meshes_path);
        if (status != PMD_SUCCESS) return status;
    }

    /* Write optional user metadata */
    if (series->_author) {
        status = write_string_attribute(file_id, "author", series->_author);
        if (status != PMD_SUCCESS) return status;
    }

    if (series->_software) {
        status = write_string_attribute(file_id, "software", series->_software);
        if (status != PMD_SUCCESS) return status;
    }

    if (series->_software_version) {
        status = write_string_attribute(file_id, "softwareVersion", series->_software_version);
        if (status != PMD_SUCCESS) return status;
    }

    if (series->_software_dependencies) {
        status = write_string_attribute(file_id, "softwareDependencies", series->_software_dependencies);
        if (status != PMD_SUCCESS) return status;
    }

    if (series->_machine) {
        status = write_string_attribute(file_id, "machine", series->_machine);
        if (status != PMD_SUCCESS) return status;
    }

    if (series->_comment) {
        status = write_string_attribute(file_id, "comment", series->_comment);
        if (status != PMD_SUCCESS) return status;
    }

    /* Write date: use stored value if available, otherwise generate current timestamp */
    if (series->_date) {
        status = write_string_attribute(file_id, "date", series->_date);
        if (status != PMD_SUCCESS) return status;
    } else {
        time_t now = time(NULL);
        struct tm *tm_info = gmtime(&now);
        char date_str[64];
        strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S +0000", tm_info);
        status = write_string_attribute(file_id, "date", date_str);
        if (status != PMD_SUCCESS) return status;
    }

    return PMD_SUCCESS;
}

static pmd_status write_double_attribute(hid_t loc_id, const char *attr_name, double value) {
    hid_t aspace_id, attr_id;
    herr_t status;

    /* Create scalar dataspace */
    aspace_id = H5Screate(H5S_SCALAR);
    if (aspace_id < 0) {
        return PMD_ERROR_HDF5;
    }

    /* Create or overwrite attribute */
    if (H5Aexists(loc_id, attr_name) > 0) {
        H5Adelete(loc_id, attr_name);
    }

    attr_id = H5Acreate2(loc_id, attr_name, H5T_IEEE_F64LE, aspace_id, H5P_DEFAULT, H5P_DEFAULT);
    if (attr_id < 0) {
        H5Sclose(aspace_id);
        return PMD_ERROR_HDF5;
    }

    /* Write attribute */
    status = H5Awrite(attr_id, H5T_NATIVE_DOUBLE, &value);
    if (status < 0) {
        H5Aclose(attr_id);
        H5Sclose(aspace_id);
        return PMD_ERROR_HDF5;
    }

    H5Aclose(attr_id);
    H5Sclose(aspace_id);
    return PMD_SUCCESS;
}

/**
 * Write unitDimension array attribute
 * Writes the 7-element array representing SI base unit powers
 *
 * @param loc_id HDF5 location to write attribute to (record group)
 * @param unit_dim Pointer to unit dimension struct (NULL to skip writing)
 * @return PMD_SUCCESS or error code
 */
static pmd_status write_unit_dimension_attribute(hid_t loc_id, const pmd_unit_dimension *unit_dim) {
    hid_t aspace_id, attr_id;
    hsize_t dims[1] = {7};
    double values[7];
    herr_t status;

    /* If NULL, don't write attribute */
    if (!unit_dim) {
        return PMD_SUCCESS;
    }

    /* Pack struct into array in correct order */
    values[0] = unit_dim->length;
    values[1] = unit_dim->mass;
    values[2] = unit_dim->time;
    values[3] = unit_dim->current;
    values[4] = unit_dim->temperature;
    values[5] = unit_dim->amount;
    values[6] = unit_dim->intensity;

    /* Create 1D dataspace with 7 elements */
    aspace_id = H5Screate_simple(1, dims, NULL);
    if (aspace_id < 0) {
        return PMD_ERROR_HDF5;
    }

    /* Delete attribute if it exists */
    if (H5Aexists(loc_id, "unitDimension") > 0) {
        H5Adelete(loc_id, "unitDimension");
    }

    /* Create attribute */
    attr_id = H5Acreate2(loc_id, "unitDimension", H5T_IEEE_F64LE, aspace_id,
                         H5P_DEFAULT, H5P_DEFAULT);
    if (attr_id < 0) {
        H5Sclose(aspace_id);
        return PMD_ERROR_HDF5;
    }

    /* Write attribute */
    status = H5Awrite(attr_id, H5T_NATIVE_DOUBLE, values);
    if (status < 0) {
        H5Aclose(attr_id);
        H5Sclose(aspace_id);
        return PMD_ERROR_HDF5;
    }

    H5Aclose(attr_id);
    H5Sclose(aspace_id);
    return PMD_SUCCESS;
}

static pmd_status write_iteration_attributes(hid_t group_id) {
    pmd_status status;

    /* Write default time = 0.0 */
    status = write_double_attribute(group_id, "time", 0.0);
    if (status != PMD_SUCCESS) return status;

    /* Write default dt = 0.0 */
    status = write_double_attribute(group_id, "dt", 0.0);
    if (status != PMD_SUCCESS) return status;

    /* Write default timeUnitSI = 1.0 (already in SI) */
    status = write_double_attribute(group_id, "timeUnitSI", 1.0);
    if (status != PMD_SUCCESS) return status;

    return PMD_SUCCESS;
}

static pmd_status ensure_parent_groups(hid_t file_id, const char *path) {
    char *path_copy = strdup(path);
    if (!path_copy) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    /* Remove trailing slash if present */
    size_t len = strlen(path_copy);
    if (len > 1 && path_copy[len-1] == '/') {
        path_copy[len-1] = '\0';
    }

    /* Skip leading slash */
    char *p = path_copy;
    if (*p == '/') p++;

    /* Create each parent group in the path (but not the final component) */
    while (*p) {
        /* Find next slash */
        char *slash = strchr(p, '/');
        if (!slash) break;  /* No more slashes - this is the final component */

        /* Temporarily null-terminate at slash to get parent path */
        *slash = '\0';
        char parent_path[512];
        snprintf(parent_path, sizeof(parent_path), "/%s", path_copy);
        *slash = '/';  /* Restore slash */

        /* Try to open parent group, create if doesn't exist */
        hid_t group_id = H5Gopen(file_id, parent_path, H5P_DEFAULT);
        if (group_id < 0) {
            /* Create parent group */
            group_id = H5Gcreate(file_id, parent_path, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            if (group_id < 0) {
                free(path_copy);
                return PMD_ERROR_HDF5;
            }
        }
        H5Gclose(group_id);

        /* Move to next component */
        p = slash + 1;
    }

    free(path_copy);
    return PMD_SUCCESS;
}

pmd_status pmd_open_series(const char *filename, pmd_series **series_out, pmd_access_mode mode) {
    pmd_series *series = NULL;
    hid_t file_id = -1;
    char *iter_encoding_str = NULL;
    pmd_status status = PMD_SUCCESS;
    char *actual_filename = NULL;
    int is_pattern = 0;
    int is_write_mode = (mode != PMD_RDONLY);
    int file_exists = 0;
    int series_ready = 0;  /* Flag to indicate series is successfully initialized */

    /* Validate input */
    if (!filename || !series_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Check if parent directory exists */
    if (!parent_directory_exists(filename)) {
        return PMD_ERROR_FILE_NOT_FOUND;
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
    series->access_mode = mode;
    series->open_iterations = NULL;
    series->num_open_iterations = 0;
    series->open_iterations_capacity = 0;

    /* Check if filename contains %T pattern */
    if (strstr(filename, "%T") != NULL) {
        /* For pattern-based filename, first check if any matching files exist */
        iteration_pattern pattern_info;
        status = parse_iteration_pattern(filename, &pattern_info);
        if (status != PMD_SUCCESS) {
            free(series);
            return status;
        }

        /* Open directory and search for matching files */
        pmd_dir *dir = pmd_opendir(pattern_info.scan_parent);
        if (!dir) {
            if (!is_write_mode) {
                /* Read mode requires existing files */
                free_iteration_pattern(&pattern_info);
                free(series);
                return PMD_ERROR_FILE_NOT_FOUND;
            }
            /* Write mode can proceed without existing directory */
        }

        /* Find first file matching the pattern */
        int found = 0;
        pmd_dirent *entry;
        if (dir) {
            while ((entry = pmd_readdir(dir)) != NULL && !found) {
                int64_t iteration;
                /* Try to extract iteration from name matching first segment pattern */
                if (extract_iteration_from_name(entry->d_name, pattern_info.first_segment, &iteration) == PMD_SUCCESS) {
                    /* Reconstruct full file path from pattern */
                    char *full_path = replace_iteration(filename, iteration);
                    if (!full_path) {
                        pmd_closedir(dir);
                        free_iteration_pattern(&pattern_info);
                        free(series);
                        return PMD_ERROR_OUT_OF_MEMORY;
                    }

                    file_id = H5Fopen(full_path, H5F_ACC_RDONLY, H5P_DEFAULT);
                    if (file_id >= 0) {
                        actual_filename = full_path;
                        found = 1;
                    } else {
                        free(full_path);
                    }
                }
            }
            pmd_closedir(dir);
        }

        if (!found) {
            /* No existing files found */
            if (!is_write_mode) {
                /* Read mode requires existing files */
                free_iteration_pattern(&pattern_info);
                free(series);
                return PMD_ERROR_FILE_NOT_FOUND;
            }

            /* Write mode - creating new file-based series */
            series->directory = strdup(pattern_info.scan_parent);
            series->iteration_encoding = PMD_FILE_BASED;

            /* Extract filename pattern (everything after directory) */
            const char *filename_pattern = filename;
            if (strcmp(pattern_info.scan_parent, ".") != 0) {
                /* Skip past directory and separator */
                size_t dir_len = strlen(pattern_info.scan_parent);
                if (strncmp(filename, pattern_info.scan_parent, dir_len) == 0) {
                    filename_pattern = filename + dir_len;
                    /* Skip separator */
                    if (*filename_pattern == '/' || *filename_pattern == '\\') {
                        filename_pattern++;
                    }
                }
            }
            series->iteration_format = strdup(filename_pattern);
            series->base_path = strdup("/data/%T/");
            series->_particles_path = strdup("particles/");
            series->_meshes_path = NULL;

            /* Don't create any files yet - will be created when iterations are added */
            series->file_id = -1;
        } else {
            /* Found existing file - save directory and read metadata from it */
            series->directory = strdup(pattern_info.scan_parent);

            /* If opening in TRUNC mode, delete all existing iteration files */
            if (mode == PMD_TRUNC || mode == PMD_EXCL) {
                /* Close the file we just opened for inspection */
                if (file_id >= 0) {
                    H5Fclose(file_id);
                    file_id = -1;
                }

                /* Enumerate and delete all matching iteration files */
                pmd_dir *del_dir = pmd_opendir(pattern_info.scan_parent);
                if (del_dir) {
                    pmd_dirent *del_entry;
                    while ((del_entry = pmd_readdir(del_dir)) != NULL) {
                        int64_t iter_index;
                        if (extract_iteration_from_name(del_entry->d_name, pattern_info.first_segment, &iter_index) == PMD_SUCCESS) {
                            char del_path[PMD_PATH_MAX];
                            snprintf(del_path, sizeof(del_path), "%s" PMD_PATH_SEP "%s",
                                    pattern_info.scan_parent, del_entry->d_name);
                            remove(del_path);  /* Delete the file */
                        }
                    }
                    pmd_closedir(del_dir);
                }

                /* Set actual_filename to NULL since we deleted the files */
                free((char*)actual_filename);
                actual_filename = NULL;

                /* Set up series for FILE_BASED mode (files will be created on demand) */
                series->iteration_encoding = PMD_FILE_BASED;

                /* Extract filename pattern (everything after directory) */
                const char *filename_pattern = filename;
                if (strcmp(pattern_info.scan_parent, ".") != 0) {
                    /* Skip past directory and separator */
                    size_t dir_len = strlen(pattern_info.scan_parent);
                    if (strncmp(filename, pattern_info.scan_parent, dir_len) == 0) {
                        filename_pattern = filename + dir_len;
                        /* Skip separator */
                        if (*filename_pattern == '/' || *filename_pattern == '\\') {
                            filename_pattern++;
                        }
                    }
                }
                series->iteration_format = strdup(filename_pattern);
                series->base_path = strdup("/data/%T/");
                series->_particles_path = strdup("particles/");
                series->_meshes_path = NULL;

                /* Don't create any files yet - will be created when iterations are added */
                series->file_id = -1;
                series_ready = 1;
            }

            free_iteration_pattern(&pattern_info);
        }
    }
    else {
        /* Check if file exists */
        FILE *test = fopen(filename, "rb");
        if (test) {
            file_exists = 1;
            fclose(test);
        }

        if (mode == PMD_TRUNC || mode == PMD_EXCL) {
            /* Creating new file */
            unsigned int h5_flags = pmd_access_mode_to_hdf5(mode);
            file_id = H5Fcreate(filename, h5_flags, H5P_DEFAULT, H5P_DEFAULT);
            if (file_id < 0) {
                free(series);
                return PMD_ERROR_HDF5;
            }

            /* Set up group-based encoding defaults */
            series->iteration_encoding = PMD_GROUP_BASED;
            series->iteration_format = strdup("/data/%T/");
            series->base_path = strdup("/data/%T/");
            series->_particles_path = strdup("particles/");
            series->_meshes_path = NULL;
            series->directory = NULL;

            /* Write required attributes */
            status = write_root_attributes(file_id, series);
            if (status != PMD_SUCCESS) {
                H5Fclose(file_id);
                free(series);
                return status;
            }

            /* Create base path structure up to scan_parent for GROUP_BASED */
            /* This ensures pmd_get_iterations can enumerate groups even when no iterations exist yet */
            iteration_pattern pattern_info;
            status = parse_iteration_pattern(series->base_path, &pattern_info);
            if (status == PMD_SUCCESS) {
                /* Create the scan_parent group if it doesn't exist (e.g., "/data" for "/data/%T/") */
                if (strcmp(pattern_info.scan_parent, ".") != 0) {
                    status = ensure_parent_groups(file_id, pattern_info.scan_parent);
                    if (status == PMD_SUCCESS) {
                        /* Create the scan_parent group itself */
                        htri_t exists = H5Lexists(file_id, pattern_info.scan_parent, H5P_DEFAULT);
                        if (exists == 0) {
                            hid_t group_id = H5Gcreate(file_id, pattern_info.scan_parent,
                                                        H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                            if (group_id >= 0) {
                                H5Gclose(group_id);
                            }
                        }
                    }
                }
                free_iteration_pattern(&pattern_info);
            }

            /* Keep file open for group-based */
            series->file_id = file_id;
        }
        else{
            /* Opening existing file (PMD_RDONLY or PMD_RDWR) */
            if (!file_exists) {
                free(series);
                return PMD_ERROR_FILE_NOT_FOUND;
            }

            unsigned int h5_flags = pmd_access_mode_to_hdf5(mode);
            file_id = H5Fopen(filename, h5_flags, H5P_DEFAULT);
            if (file_id < 0) {
                free(series);
                return PMD_ERROR_HDF5;
            }
            actual_filename = strdup(filename);
            series->file_id = file_id;
        }
    }

    /* If there is an existing file which we must read metadata from */
    if (file_id > 0){
        /* Read and validate required openPMD attribute */
        char *openpmd_version = NULL;
        status = read_string_attribute(file_id, "openPMD", &openpmd_version);
        if (status != PMD_SUCCESS) {
            goto cleanup;
        }

        /* Parse version (format: "X.Y.Z") for validation */
        int major, minor, revision;
        if (sscanf(openpmd_version, "%d.%d.%d", &major, &minor, &revision) != 3) {
            pmd_log(PMD_LOG_ERROR, "Invalid OpenPMD version format '%s' in '%s' (expected X.Y.Z)\n",
                    openpmd_version, filename);
            free(openpmd_version);
            status = PMD_ERROR_FILE_FORMAT;
            goto cleanup;
        }

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

        /* Read optional metadata attributes */
        if (attribute_exists(file_id, "author") > 0) {
            read_string_attribute(file_id, "author", &series->_author);
        } else {
            series->_author = NULL;
        }

        if (attribute_exists(file_id, "software") > 0) {
            read_string_attribute(file_id, "software", &series->_software);
        } else {
            series->_software = NULL;
        }

        if (attribute_exists(file_id, "softwareVersion") > 0) {
            read_string_attribute(file_id, "softwareVersion", &series->_software_version);
        } else {
            series->_software_version = NULL;
        }

        if (attribute_exists(file_id, "softwareDependencies") > 0) {
            read_string_attribute(file_id, "softwareDependencies", &series->_software_dependencies);
        } else {
            series->_software_dependencies = NULL;
        }

        if (attribute_exists(file_id, "machine") > 0) {
            read_string_attribute(file_id, "machine", &series->_machine);
        } else {
            series->_machine = NULL;
        }

        if (attribute_exists(file_id, "comment") > 0) {
            read_string_attribute(file_id, "comment", &series->_comment);
        } else {
            series->_comment = NULL;
        }

        if (attribute_exists(file_id, "date") > 0) {
            read_string_attribute(file_id, "date", &series->_date);
        } else {
            series->_date = NULL;
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

            /* For fileBased, extract directory if not already set */
            if (!series->directory) {
                series->directory = pmd_dirname(filename);
            }
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
    }

    free(iter_encoding_str);
    free(actual_filename);

    /* Final check - if series initialized successfully, return it */
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

    /* Free metadata attributes */
    free(series->_author);
    free(series->_software);
    free(series->_software_version);
    free(series->_software_dependencies);
    free(series->_machine);
    free(series->_comment);
    free(series->_date);

    /* Free open iterations tracking */
    free(series->open_iterations);

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

            /* Validate full path if pattern has additional path components */
            if (pattern_info.full_pattern && (strchr(pattern_info.full_pattern, '/') || strchr(pattern_info.full_pattern, '\\'))) {
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
/**
 * Validate iteration pattern for ambiguous or invalid constructs
 * Returns PMD_SUCCESS if valid, PMD_ERROR_FILE_FORMAT if invalid
 */
static pmd_status validate_iteration_pattern(const char *pattern) {
    if (!pattern) {
        return PMD_ERROR_NULL_POINTER;
    }

    const char *ptr = pattern;
    while (*ptr != '\0') {
        if (*ptr == '%' && *(ptr + 1) == 'T') {
            /* Found a %T */

            /* Check for ambiguous %T%T (adjacent with no separator) */
            if (ptr[2] == '%' && ptr[3] == 'T') {
                return PMD_ERROR_FILE_FORMAT;
            }

            /* Move to next character after %T */
            ptr += 2;
        } else {
            ptr++;
        }
    }

    return PMD_SUCCESS;
}

static pmd_status parse_iteration_pattern(const char *pattern, iteration_pattern *info) {
    if (!pattern || !info) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Validate pattern first */
    pmd_status status = validate_iteration_pattern(pattern);
    if (status != PMD_SUCCESS) {
        return status;
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

    /* Find last path separator before first %T to get scan parent
     * Check for both / and \ to support both HDF5 paths and Windows file paths */
    const char *last_slash = first_T;
    while (last_slash > pattern && *last_slash != '/' && *last_slash != '\\') {
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
    const char *segment_start = (*last_slash == '/' || *last_slash == '\\') ? last_slash + 1 : last_slash;
    const char *segment_end = segment_start;

    /* Find end of first segment (next path separator or end of string) */
    while (*segment_end != '\0' && *segment_end != '/' && *segment_end != '\\') {
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
 * Helper struct for collecting species names during iteration
 */
typedef struct {
    char **names;
    int count;
    pmd_status status;  /* Error status from validation */
} species_collector;

/**
 * Callback to collect species names
 */
static herr_t collect_species_iteration_callback(hid_t loc_id, const char *name,
                                                   const H5L_info_t *info, void *op_data) {
    species_collector *collector = (species_collector *)op_data;

    /* Check if this is a group, not a dataset */
    H5O_info2_t obj_info;
    if (H5Oget_info_by_name(loc_id, name, &obj_info, H5O_INFO_BASIC, H5P_DEFAULT) < 0) {
        return 0;  /* Skip on error */
    }
    if (obj_info.type != H5O_TYPE_GROUP) {
        /* Species must be a group, not a dataset - skip it */
        return 0;
    }

    /* Collect species name */
    collector->names[collector->count] = strdup(name);
    if (!collector->names[collector->count]) {
        collector->status = PMD_ERROR_OUT_OF_MEMORY;
        return -1;  /* Stop iteration with error */
    }
    collector->count++;
    return 0;
}

/**
 * Find an already-open iteration by index
 * Returns the iteration handle if found, NULL otherwise
 */
static pmd_iteration *find_open_iteration(pmd_series *series, int64_t index) {
    if (!series || !series->open_iterations) {
        return NULL;
    }

    for (int i = 0; i < series->num_open_iterations; i++) {
        if (series->open_iterations[i] && series->open_iterations[i]->iteration_index == index) {
            return series->open_iterations[i];
        }
    }

    return NULL;
}

/**
 * Register an iteration as open
 */
static pmd_status register_open_iteration(pmd_series *series, pmd_iteration *iter) {
    if (!series || !iter) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Only track for FILE_BASED to prevent multiple file handles */
    if (series->iteration_encoding != PMD_FILE_BASED) {
        return PMD_SUCCESS;
    }

    /* Expand array if needed */
    if (series->num_open_iterations >= series->open_iterations_capacity) {
        int new_capacity = series->open_iterations_capacity == 0 ? 4 : series->open_iterations_capacity * 2;
        pmd_iteration **new_array = (pmd_iteration **)realloc(series->open_iterations,
                                                               new_capacity * sizeof(pmd_iteration *));
        if (!new_array) {
            return PMD_ERROR_OUT_OF_MEMORY;
        }
        series->open_iterations = new_array;
        series->open_iterations_capacity = new_capacity;
    }

    series->open_iterations[series->num_open_iterations++] = iter;
    return PMD_SUCCESS;
}

/**
 * Unregister an iteration (when closing)
 */
static void unregister_open_iteration(pmd_series *series, pmd_iteration *iter) {
    if (!series || !iter || !series->open_iterations) {
        return;
    }

    /* Find and remove the iteration */
    for (int i = 0; i < series->num_open_iterations; i++) {
        if (series->open_iterations[i] == iter) {
            /* Shift remaining elements */
            for (int j = i; j < series->num_open_iterations - 1; j++) {
                series->open_iterations[j] = series->open_iterations[j + 1];
            }
            series->num_open_iterations--;
            return;
        }
    }
}

pmd_status pmd_open_iteration(pmd_series *series, int64_t index, pmd_iteration **iter_out) {
    pmd_iteration *iter = NULL;
    pmd_status status = PMD_SUCCESS;
    char *iteration_path = NULL;

    if (!series || !iter_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Check for negative iteration index */
    if (index < 0) {
        pmd_log(PMD_LOG_ERROR, "Iteration index must be non-negative, got %lld", (long long)index);
        return PMD_ERROR;
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
        /* GROUP_BASED: borrow file_id from series, open or create iteration group */
        iter->file_id = series->file_id;  /* Borrowed reference */

        /* Try to open existing group */
        iter->iteration_group_id = H5Gopen(series->file_id, iteration_path, H5P_DEFAULT);

        if (iter->iteration_group_id < 0) {
            /* Group doesn't exist - check if we're in write mode */
            if (series->access_mode == PMD_RDONLY) {
                status = PMD_ERROR_INVALID_ITERATION;
                goto cleanup;
            }

            /* Ensure parent groups exist */
            status = ensure_parent_groups(series->file_id, iteration_path);
            if (status != PMD_SUCCESS) {
                goto cleanup;
            }

            /* Create new iteration group */
            iter->iteration_group_id = H5Gcreate(series->file_id, iteration_path,
                                                  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            if (iter->iteration_group_id < 0) {
                status = PMD_ERROR_HDF5;
                goto cleanup;
            }

            /* Write default iteration attributes */
            status = write_iteration_attributes(iter->iteration_group_id);
            if (status != PMD_SUCCESS) {
                goto cleanup;
            }

            /* Invalidate iteration cache since we just created a new iteration */
            series->num_iterations = -1;
            free(series->iteration_indices);
            series->iteration_indices = NULL;

            /* Create particles group if particlesPath is defined */
            char *particles_path_copy = NULL;
            status = pmd_get_particles_path(series, &particles_path_copy);
            if (status == PMD_SUCCESS) {
                /* particlesPath is defined, create the group */

                /* Remove trailing slash */
                size_t len = strlen(particles_path_copy);
                if (len > 0 && particles_path_copy[len-1] == '/') {
                    particles_path_copy[len-1] = '\0';
                }

                hid_t particles_group = H5Gcreate(iter->iteration_group_id, particles_path_copy,
                                                    H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                free(particles_path_copy);
                if (particles_group < 0) {
                    status = PMD_ERROR_HDF5;
                    goto cleanup;
                }
                H5Gclose(particles_group);
            }
            /* If particlesPath not defined, that's OK - file has no particles */
        }

    } else {  /* PMD_FILE_BASED */
        /* FILE_BASED: Check if this iteration is already open to reuse file handle */
        pmd_iteration *existing = find_open_iteration(series, index);
        if (existing) {
            /* Iteration already open - reuse the file_id */
            iter->file_id = existing->file_id;

            /* Try to open the iteration group */
            iter->iteration_group_id = H5Gopen(iter->file_id, iteration_path, H5P_DEFAULT);
            if (iter->iteration_group_id < 0) {
                status = PMD_ERROR_HDF5;
                goto cleanup;
            }

            free(iteration_path);
            iteration_path = NULL;
        } else {
        /* Not already open - open or create file for this iteration */
        char *filename = replace_iteration(series->iteration_format, index);
        if (!filename) {
            status = PMD_ERROR_OUT_OF_MEMORY;
            goto cleanup;
        }
        char full_path[PMD_PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s" PMD_PATH_SEP "%s", series->directory, filename);
        free(filename);

        /* Try to open existing file */
        /* Note: Convert TRUNC/EXCL to RDWR for opening existing files */
        unsigned int h5_flags;
        if (series->access_mode == PMD_TRUNC || series->access_mode == PMD_EXCL) {
            h5_flags = H5F_ACC_RDWR;
        } else {
            h5_flags = pmd_access_mode_to_hdf5(series->access_mode);
        }
        iter->file_id = H5Fopen(full_path, h5_flags, H5P_DEFAULT);

        if (iter->file_id < 0) {
            /* File doesn't exist - check if we're in write mode */
            if (series->access_mode == PMD_RDONLY) {
                status = PMD_ERROR_FILE_NOT_FOUND;
                goto cleanup;
            }

            /* Create parent directory if needed */
            status = create_parent_directory(full_path);
            if (status != PMD_SUCCESS) {
                goto cleanup;
            }

            /* Create new file with openPMD attributes */
            iter->file_id = H5Fcreate(full_path, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
            if (iter->file_id < 0) {
                status = PMD_ERROR_HDF5;
                goto cleanup;
            }

            /* Write all root attributes to new file */
            status = write_root_attributes(iter->file_id, series);
            if (status != PMD_SUCCESS) {
                goto cleanup;
            }

            /* Invalidate iteration cache since we just created a new iteration file */
            series->num_iterations = -1;
            free(series->iteration_indices);
            series->iteration_indices = NULL;

            /* Ensure parent groups exist */
            status = ensure_parent_groups(iter->file_id, iteration_path);
            if (status != PMD_SUCCESS) {
                goto cleanup;
            }

            /* Create iteration group */
            iter->iteration_group_id = H5Gcreate(iter->file_id, iteration_path,
                                                  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            if (iter->iteration_group_id < 0) {
                status = PMD_ERROR_HDF5;
                goto cleanup;
            }

            /* Write iteration attributes */
            status = write_iteration_attributes(iter->iteration_group_id);
            if (status != PMD_SUCCESS) {
                goto cleanup;
            }

            /* Create particles group if particlesPath is defined */
            char *particles_path_copy = NULL;
            status = pmd_get_particles_path(series, &particles_path_copy);
            if (status == PMD_SUCCESS) {
                /* particlesPath is defined, create the group */

                /* Remove trailing slash */
                size_t len = strlen(particles_path_copy);
                if (len > 0 && particles_path_copy[len-1] == '/') {
                    particles_path_copy[len-1] = '\0';
                }

                hid_t particles_group = H5Gcreate(iter->iteration_group_id, particles_path_copy,
                                                    H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                free(particles_path_copy);
                if (particles_group < 0) {
                    status = PMD_ERROR_HDF5;
                    goto cleanup;
                }
                H5Gclose(particles_group);
            }
            /* If particlesPath not defined, that's OK - file has no particles */
        } else {
            /* File exists - open iteration group */
            iter->iteration_group_id = H5Gopen(iter->file_id, iteration_path, H5P_DEFAULT);
            if (iter->iteration_group_id < 0) {
                status = PMD_ERROR_INVALID_ITERATION;
                goto cleanup;
            }
        }
        }
    }

    free(iteration_path);
    iteration_path = NULL;

    /* Register and return it */
    status = register_open_iteration(series, iter);
    if (status != PMD_SUCCESS) {
        /* If registration fails, close what we opened and return error */
        if (iter->iteration_group_id >= 0) H5Gclose(iter->iteration_group_id);
        if (series->iteration_encoding == PMD_FILE_BASED && iter->file_id >= 0) {
            H5Fclose(iter->file_id);
        }
        free(iter);
        return status;
    }

    *iter_out = iter;
    return PMD_SUCCESS;

cleanup:
    free(iteration_path);

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

    /* Unregister this iteration */
    unregister_open_iteration(iter->series, iter);

    /* Close iteration group (always owned by iteration) */
    if (iter->iteration_group_id >= 0) {
        H5Gclose(iter->iteration_group_id);
    }

    /* Close file only for FILE_BASED (GROUP_BASED borrows from series) */
    /* Only close if no other iterations are using this file */
    if (iter->series->iteration_encoding == PMD_FILE_BASED && iter->file_id >= 0) {
        int other_using_file = 0;
        /* Check if any other open iteration is using the same file_id */
        for (int i = 0; i < iter->series->num_open_iterations; i++) {
            if (iter->series->open_iterations[i] &&
                iter->series->open_iterations[i]->file_id == iter->file_id) {
                other_using_file = 1;
                break;
            }
        }

        /* Only close if this is the last one */
        if (!other_using_file) {
            H5Fclose(iter->file_id);
        }
    }

    /* Free the struct */
    free(iter);

    return PMD_SUCCESS;
}

pmd_status pmd_get_species(pmd_iteration *iter, char ***species_names, int *count) {
    pmd_status status = PMD_SUCCESS;
    char *particles_full_path = NULL;
    hid_t particles_group_id = -1;
    int num_species = 0;
    char **names = NULL;

    if (!iter || !species_names || !count) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Try to get particlesPath */
    status = pmd_get_particles_path(iter->series, &particles_full_path);
    if (status != PMD_SUCCESS) {
        /* particlesPath attribute is not present - file has no particles */
        *species_names = NULL;
        *count = 0;
        return PMD_SUCCESS;
    }

    /* Remove trailing slash if present */
    size_t len = strlen(particles_full_path);
    if (len > 0 && particles_full_path[len-1] == '/') {
        particles_full_path[len-1] = '\0';
    }

    /* Check if particles group exists */
    if (!H5Lexists(iter->iteration_group_id, particles_full_path, H5P_DEFAULT)) {
        /* particlesPath is defined but group doesn't exist */
        free(particles_full_path);
        return PMD_ERROR_FILE_FORMAT;
    }

    /* Check that particles is a group, not a dataset */
    H5O_info2_t obj_info;
    if (H5Oget_info_by_name(iter->iteration_group_id, particles_full_path, &obj_info,
                            H5O_INFO_BASIC, H5P_DEFAULT) >= 0) {
        if (obj_info.type != H5O_TYPE_GROUP) {
            /* particlesPath points to a dataset, not a group */
            free(particles_full_path);
            return PMD_ERROR_FILE_FORMAT;
        }
    }

    /* Open particles group relative to iteration group */
    particles_group_id = H5Gopen(iter->iteration_group_id, particles_full_path, H5P_DEFAULT);
    free(particles_full_path);
    if (particles_group_id < 0) {
        return PMD_ERROR_FILE_FORMAT;
    }

    /* First pass: count species */
    H5Literate(particles_group_id, H5_INDEX_NAME, H5_ITER_NATIVE, NULL,
               count_species_iteration_callback, &num_species);

    if (num_species == 0) {
        *species_names = NULL;
        *count = 0;
        H5Gclose(particles_group_id);
        return PMD_SUCCESS;
    }

    /* Allocate array for species names */
    names = (char **)calloc(num_species, sizeof(char *));
    if (!names) {
        H5Gclose(particles_group_id);
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    /* Second pass: collect species names */
    species_collector collector = {names, 0, PMD_SUCCESS};
    herr_t iter_result = H5Literate(particles_group_id, H5_INDEX_NAME, H5_ITER_NATIVE, NULL,
                                     collect_species_iteration_callback, &collector);
    if (iter_result < 0) {
        /* Callback returned an error - cleanup */
        for (int i = 0; i < collector.count; i++) {
            free(names[i]);
        }
        free(names);
        H5Gclose(particles_group_id);
        return collector.status;
    }

    H5Gclose(particles_group_id);
    *species_names = names;
    *count = num_species;
    return PMD_SUCCESS;
}

pmd_status pmd_get_num_particles(pmd_iteration *iter, const char *species, int64_t *count) {
    pmd_status status;
    char *particles_full_path = NULL;
    hid_t particles_group_id = -1;
    hid_t species_group_id = -1;
    hid_t attr_id = -1;
    int64_t num_particles = 0;

    if (!iter || !species || !count) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Get particlesPath */
    status = pmd_get_particles_path(iter->series, &particles_full_path);
    if (status != PMD_SUCCESS) {
        return PMD_ERROR_INVALID_SPECIES;  /* No particles at all */
    }

    /* Remove trailing slash if present */
    size_t len = strlen(particles_full_path);
    if (len > 0 && particles_full_path[len-1] == '/') {
        particles_full_path[len-1] = '\0';
    }

    /* Open particles group */
    particles_group_id = H5Gopen(iter->iteration_group_id, particles_full_path, H5P_DEFAULT);
    free(particles_full_path);
    if (particles_group_id < 0) {
        return PMD_ERROR_FILE_FORMAT;
    }

    /* Open species group */
    species_group_id = H5Gopen(particles_group_id, species, H5P_DEFAULT);
    H5Gclose(particles_group_id);
    if (species_group_id < 0) {
        return PMD_ERROR_INVALID_SPECIES;
    }

    /* Read numParticles attribute */
    attr_id = H5Aopen(species_group_id, "numParticles", H5P_DEFAULT);
    if (attr_id < 0) {
        H5Gclose(species_group_id);
        return PMD_ERROR_FILE_FORMAT;
    }

    /* Validate attribute type */
    status = validate_attribute_type(attr_id, H5T_INTEGER);
    if (status != PMD_SUCCESS) {
        H5Aclose(attr_id);
        H5Gclose(species_group_id);
        return status;
    }

    /* Read the value */
    if (H5Aread(attr_id, H5T_NATIVE_INT64, &num_particles) < 0) {
        H5Aclose(attr_id);
        H5Gclose(species_group_id);
        return PMD_ERROR_FILE_FORMAT;
    }

    /* Validate that numParticles is non-negative */
    if (num_particles < 0) {
        H5Aclose(attr_id);
        H5Gclose(species_group_id);
        return PMD_ERROR_FILE_FORMAT;
    }

    H5Aclose(attr_id);
    H5Gclose(species_group_id);

    *count = num_particles;
    return PMD_SUCCESS;
}

/* =========================================================================
 * Iteration Metadata Operations Implementation
 * ========================================================================= */

pmd_status pmd_get_time(pmd_iteration *iter, double *time) {
    if (!iter || !time) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Read time attribute - required, error if missing */
    if (attribute_exists(iter->iteration_group_id, "time") <= 0) {
        return PMD_ERROR_FILE_FORMAT;
    }

    return read_double_attribute(iter->iteration_group_id, "time", time);
}

pmd_status pmd_get_dt(pmd_iteration *iter, double *dt) {
    if (!iter || !dt) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Read dt attribute - required, error if missing */
    if (attribute_exists(iter->iteration_group_id, "dt") <= 0) {
        return PMD_ERROR_FILE_FORMAT;
    }

    return read_double_attribute(iter->iteration_group_id, "dt", dt);
}

pmd_status pmd_get_time_unit_si(pmd_iteration *iter, double *time_unit_si) {
    if (!iter || !time_unit_si) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Read timeUnitSI attribute - optional, error if missing */
    if (attribute_exists(iter->iteration_group_id, "timeUnitSI") <= 0) {
        return PMD_ERROR;
    }

    return read_double_attribute(iter->iteration_group_id, "timeUnitSI", time_unit_si);
}

pmd_status pmd_set_time(pmd_iteration *iter, double time) {
    if (!iter) {
        return PMD_ERROR_NULL_POINTER;
    }

    return write_double_attribute(iter->iteration_group_id, "time", time);
}

pmd_status pmd_set_dt(pmd_iteration *iter, double dt) {
    if (!iter) {
        return PMD_ERROR_NULL_POINTER;
    }

    return write_double_attribute(iter->iteration_group_id, "dt", dt);
}

pmd_status pmd_set_time_unit_si(pmd_iteration *iter, double time_unit_si) {
    if (!iter) {
        return PMD_ERROR_NULL_POINTER;
    }

    return write_double_attribute(iter->iteration_group_id, "timeUnitSI", time_unit_si);
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

pmd_status pmd_get_openpmd_version(pmd_series *series, int *major, int *minor, int *revision) {
    hid_t file_id = -1;
    pmd_iteration *iter = NULL;
    pmd_status status;
    char *version_str = NULL;

    if (!series || !major || !minor || !revision) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Get file handle */
    if (series->iteration_encoding == PMD_GROUP_BASED) {
        file_id = series->file_id;
    } else {
        /* FILE_BASED: need to open first iteration to get to file */
        int64_t *iterations;
        int num_iterations;
        status = pmd_get_iterations(series, &iterations, &num_iterations);
        if (status != PMD_SUCCESS) {
            return PMD_ERROR_FILE_FORMAT;
        }
        if (num_iterations == 0) {
            /* No iterations yet, return our version */
            if (sscanf(OPENPMD_VERSION_STRING, "%d.%d.%d", major, minor, revision) != 3) {
                free(iterations);
                return PMD_ERROR;
            }
            free(iterations);
            return PMD_SUCCESS;
        }
        status = pmd_open_iteration(series, iterations[0], &iter);
        free(iterations);
        if (status != PMD_SUCCESS) return status;
        file_id = iter->file_id;
    }

    /* Read openPMD attribute */
    status = read_string_attribute(file_id, "openPMD", &version_str);
    if (status != PMD_SUCCESS) {
        if (iter) pmd_close_iteration(iter);
        return status;
    }

    /* Parse version string (format: "major.minor.revision") */
    if (sscanf(version_str, "%d.%d.%d", major, minor, revision) != 3) {
        free(version_str);
        if (iter) pmd_close_iteration(iter);
        return PMD_ERROR_FILE_FORMAT;
    }

    free(version_str);
    if (iter) pmd_close_iteration(iter);
    return PMD_SUCCESS;
}

/**
 * Helper function to read a root-level string attribute from the series file
 * Handles both GROUP_BASED (file_id) and FILE_BASED (need to open file) cases
 */
static pmd_status read_series_root_attribute(pmd_series *series, const char *attr_name, char **value_out) {
    hid_t file_id = -1;
    pmd_status status;
    pmd_iteration *iter = NULL;

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

        /* Use pmd_open_iteration to get access to the first iteration's file */
        status = pmd_open_iteration(series, iterations[0], &iter);
        if (status != PMD_SUCCESS) {
            return status;
        }
        file_id = iter->file_id;
    }

    /* Check if attribute exists */
    if (attribute_exists(file_id, attr_name) <= 0) {
        if (iter) pmd_close_iteration(iter);
        return PMD_ERROR;  /* Attribute doesn't exist */
    }

    /* Read the attribute */
    status = read_string_attribute(file_id, attr_name, value_out);

    /* Clean up */
    if (iter) {
        pmd_close_iteration(iter);
    }

    return status;
}

pmd_status pmd_get_openpmd_extension(pmd_series *series, char **value_out) {
    return read_series_root_attribute(series, "openPMDextension", value_out);
}

pmd_status pmd_get_author(pmd_series *series, char **value_out) {
    if (!series || !value_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    if (!series->_author) {
        return PMD_ERROR;  /* Attribute not set */
    }

    *value_out = strdup(series->_author);
    if (!*value_out) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    return PMD_SUCCESS;
}

pmd_status pmd_get_software(pmd_series *series, char **value_out) {
    if (!series || !value_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    if (!series->_software) {
        return PMD_ERROR;  /* Attribute not set */
    }

    *value_out = strdup(series->_software);
    if (!*value_out) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    return PMD_SUCCESS;
}

pmd_status pmd_get_software_version(pmd_series *series, char **value_out) {
    if (!series || !value_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    if (!series->_software_version) {
        return PMD_ERROR;  /* Attribute not set */
    }

    *value_out = strdup(series->_software_version);
    if (!*value_out) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    return PMD_SUCCESS;
}

pmd_status pmd_get_date(pmd_series *series, char **value_out) {
    if (!series || !value_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    if (!series->_date) {
        return PMD_ERROR;  /* Attribute not set */
    }

    *value_out = strdup(series->_date);
    if (!*value_out) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    return PMD_SUCCESS;
}

pmd_status pmd_get_software_dependencies(pmd_series *series, char **value_out) {
    if (!series || !value_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    if (!series->_software_dependencies) {
        return PMD_ERROR;  /* Attribute not set */
    }

    *value_out = strdup(series->_software_dependencies);
    if (!*value_out) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    return PMD_SUCCESS;
}

pmd_status pmd_get_machine(pmd_series *series, char **value_out) {
    if (!series || !value_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    if (!series->_machine) {
        return PMD_ERROR;  /* Attribute not set */
    }

    *value_out = strdup(series->_machine);
    if (!*value_out) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    return PMD_SUCCESS;
}

pmd_status pmd_get_comment(pmd_series *series, char **value_out) {
    if (!series || !value_out) {
        return PMD_ERROR_NULL_POINTER;
    }

    if (!series->_comment) {
        return PMD_ERROR;  /* Attribute not set */
    }

    *value_out = strdup(series->_comment);
    if (!*value_out) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    return PMD_SUCCESS;
}

static pmd_status write_series_root_attribute(pmd_series *series, const char *attr_name, const char *value) {
    hid_t file_id = -1;
    pmd_status status;
    int should_close_file = 0;

    if (!series || !attr_name || !value) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Check if we're in write mode */
    if (series->access_mode == PMD_RDONLY) {
        return PMD_ERROR;
    }

    if (series->iteration_encoding == PMD_GROUP_BASED) {
        /* Use series file_id directly */
        file_id = series->file_id;
        status = write_string_attribute(file_id, attr_name, value);
    } else {
        /* FILE_BASED: need to write to all iteration files */
        int64_t *iterations;
        int num_iterations;
        status = pmd_get_iterations(series, &iterations, &num_iterations);
        if (status != PMD_SUCCESS) {
            return status;
        }

        /* Write to each iteration file */
        for (int i = 0; i < num_iterations; i++) {
            pmd_iteration *iter;
            status = pmd_open_iteration(series, iterations[i], &iter);
            if (status != PMD_SUCCESS) {
                continue;  /* Skip files that can't be opened */
            }

            status = write_string_attribute(iter->file_id, attr_name, value);
            pmd_close_iteration(iter);
            if (status != PMD_SUCCESS) {
                return status;
            }
        }
    }

    return PMD_SUCCESS;
}

/**
 * Write all series root metadata attributes to all relevant files
 * For GROUP_BASED: writes to the single series file
 * For FILE_BASED: writes to all iteration files
 */
static pmd_status write_series_root_attributes(pmd_series *series) {
    pmd_status status;

    if (!series) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Check if we're in write mode */
    if (series->access_mode == PMD_RDONLY) {
        return PMD_ERROR;
    }

    if (series->iteration_encoding == PMD_GROUP_BASED) {
        /* Write to the single series file */
        status = write_root_attributes(series->file_id, series);
    } else {
        /* FILE_BASED: write to all iteration files */
        int64_t *iterations;
        int num_iterations;
        status = pmd_get_iterations(series, &iterations, &num_iterations);
        if (status != PMD_SUCCESS) {
            /* If no iterations exist yet, that's OK - metadata will be written when iterations are created */
            return PMD_SUCCESS;
        }

        /* Write to each iteration file */
        for (int i = 0; i < num_iterations; i++) {
            pmd_iteration *iter;

            /* Use pmd_open_iteration to get access to the file (reuses handle if already open) */
            status = pmd_open_iteration(series, iterations[i], &iter);
            if (status != PMD_SUCCESS) {
                continue;  /* Skip iterations that can't be opened */
            }

            /* Write metadata to the iteration's file */
            status = write_root_attributes(iter->file_id, series);

            /* Close the iteration (won't actually close file if other handles are using it) */
            pmd_close_iteration(iter);

            if (status != PMD_SUCCESS) {
                return status;
            }
        }
    }

    return PMD_SUCCESS;
}

pmd_status pmd_set_author(pmd_series *series, const char *value) {
    if (!series || !value) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Free existing value if any */
    if (series->_author) {
        free(series->_author);
    }

    /* Store in series handle */
    series->_author = strdup(value);
    if (!series->_author) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    /* Write to file(s) */
    return write_series_root_attributes(series);
}

pmd_status pmd_set_software(pmd_series *series, const char *value) {
    if (!series || !value) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Free existing value if any */
    if (series->_software) {
        free(series->_software);
    }

    /* Store in series handle */
    series->_software = strdup(value);
    if (!series->_software) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    /* Write to file(s) */
    return write_series_root_attributes(series);
}

pmd_status pmd_set_software_version(pmd_series *series, const char *value) {
    if (!series || !value) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Free existing value if any */
    if (series->_software_version) {
        free(series->_software_version);
    }

    /* Store in series handle */
    series->_software_version = strdup(value);
    if (!series->_software_version) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    /* Write to file(s) */
    return write_series_root_attributes(series);
}

pmd_status pmd_set_software_dependencies(pmd_series *series, const char *value) {
    if (!series || !value) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Free existing value if any */
    if (series->_software_dependencies) {
        free(series->_software_dependencies);
    }

    /* Store in series handle */
    series->_software_dependencies = strdup(value);
    if (!series->_software_dependencies) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    /* Write to file(s) */
    return write_series_root_attributes(series);
}

pmd_status pmd_set_machine(pmd_series *series, const char *value) {
    if (!series || !value) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Free existing value if any */
    if (series->_machine) {
        free(series->_machine);
    }

    /* Store in series handle */
    series->_machine = strdup(value);
    if (!series->_machine) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    /* Write to file(s) */
    return write_series_root_attributes(series);
}

pmd_status pmd_set_comment(pmd_series *series, const char *value) {
    if (!series || !value) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Free existing value if any */
    if (series->_comment) {
        free(series->_comment);
    }

    /* Store in series handle */
    series->_comment = strdup(value);
    if (!series->_comment) {
        return PMD_ERROR_OUT_OF_MEMORY;
    }

    /* Write to file(s) */
    return write_series_root_attributes(series);
}

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
    pg->x_offset = (double *)calloc(num_particles, sizeof(double));
    pg->y_offset = (double *)calloc(num_particles, sizeof(double));
    pg->z_offset = (double *)calloc(num_particles, sizeof(double));
    pg->px = (double *)calloc(num_particles, sizeof(double));
    pg->py = (double *)calloc(num_particles, sizeof(double));
    pg->pz = (double *)calloc(num_particles, sizeof(double));
    pg->weight = (double *)calloc(num_particles, sizeof(double));
    pg->status = (int64_t *)calloc(num_particles, sizeof(int64_t));
    pg->id = (int64_t *)calloc(num_particles, sizeof(int64_t));

    /* Check allocation */
    if (!pg->species_type || !pg->x || !pg->y || !pg->z || !pg->t ||
        !pg->x_offset || !pg->y_offset || !pg->z_offset ||
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

    /* Get particlesPath */
    status = pmd_get_particles_path(iter->series, &particles_path);
    if (status != PMD_SUCCESS) {
        return PMD_ERROR_INVALID_SPECIES;
    }

    /* Suppress HDF5 error messages for expected failures */
    H5Eset_auto(H5E_DEFAULT, NULL, NULL);
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

    /* Read positionOffset components - warn and use zeros if not present */
    if (pg->x_offset) {
        if (record_exists(species_group_id, "positionOffset/x")) {
            status = read_double_record(species_group_id, "positionOffset/x", pg->x_offset, num_particles, 1.0);
            if (status != PMD_SUCCESS) goto cleanup;
        } else {
            pmd_log(PMD_LOG_WARNING, "pmd_read_particle_group: positionOffset/x not found, using zeros\n");
            for (int64_t i = 0; i < num_particles; i++) pg->x_offset[i] = 0.0;
        }
    }

    if (pg->y_offset) {
        if (record_exists(species_group_id, "positionOffset/y")) {
            status = read_double_record(species_group_id, "positionOffset/y", pg->y_offset, num_particles, 1.0);
            if (status != PMD_SUCCESS) goto cleanup;
        } else {
            pmd_log(PMD_LOG_WARNING, "pmd_read_particle_group: positionOffset/y not found, using zeros\n");
            for (int64_t i = 0; i < num_particles; i++) pg->y_offset[i] = 0.0;
        }
    }

    if (pg->z_offset) {
        if (record_exists(species_group_id, "positionOffset/z")) {
            status = read_double_record(species_group_id, "positionOffset/z", pg->z_offset, num_particles, 1.0);
            if (status != PMD_SUCCESS) goto cleanup;
        } else {
            pmd_log(PMD_LOG_WARNING, "pmd_read_particle_group: positionOffset/z not found, using zeros\n");
            for (int64_t i = 0; i < num_particles; i++) pg->z_offset[i] = 0.0;
        }
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
    free(pg->x_offset);
    free(pg->y_offset);
    free(pg->z_offset);
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

static pmd_status write_int64_attribute(hid_t loc_id, const char *attr_name, int64_t value) {
    hid_t aspace_id, attr_id;
    herr_t status;

    aspace_id = H5Screate(H5S_SCALAR);
    if (aspace_id < 0) return PMD_ERROR_HDF5;

    if (H5Aexists(loc_id, attr_name) > 0) {
        H5Adelete(loc_id, attr_name);
    }

    attr_id = H5Acreate2(loc_id, attr_name, H5T_STD_I64LE, aspace_id, H5P_DEFAULT, H5P_DEFAULT);
    if (attr_id < 0) {
        H5Sclose(aspace_id);
        return PMD_ERROR_HDF5;
    }

    status = H5Awrite(attr_id, H5T_NATIVE_INT64, &value);
    if (status < 0) {
        H5Aclose(attr_id);
        H5Sclose(aspace_id);
        return PMD_ERROR_HDF5;
    }

    H5Aclose(attr_id);
    H5Sclose(aspace_id);
    return PMD_SUCCESS;
}

/**
 * Write double dataset with no attributes (private helper)
 */
static pmd_status _write_double_dataset(hid_t group_id, const char *name, const double *data,
                                         int64_t num_particles) {
    hid_t dspace_id, dset_id;
    hsize_t dims[1] = {(hsize_t)num_particles};

    dspace_id = H5Screate_simple(1, dims, NULL);
    if (dspace_id < 0) return PMD_ERROR_HDF5;

    dset_id = H5Dcreate(group_id, name, H5T_IEEE_F64LE, dspace_id,
                        H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (dset_id < 0) {
        H5Sclose(dspace_id);
        return PMD_ERROR_HDF5;
    }

    if (H5Dwrite(dset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data) < 0) {
        H5Dclose(dset_id);
        H5Sclose(dspace_id);
        return PMD_ERROR_HDF5;
    }

    H5Dclose(dset_id);
    H5Sclose(dspace_id);
    return PMD_SUCCESS;
}

/**
 * Write double dataset with unitSI attribute, and optionally unitDimension and timeOffset
 */
static pmd_status write_double_dataset(hid_t group_id, const char *name, const double *data,
                                        int64_t num_particles, double unit_si,
                                        const pmd_unit_dimension *unit_dim, double time_offset) {
    pmd_status status;
    hid_t dset_id;

    /* Write the dataset */
    status = _write_double_dataset(group_id, name, data, num_particles);
    if (status != PMD_SUCCESS) return status;

    /* Reopen dataset to add attributes */
    dset_id = H5Dopen(group_id, name, H5P_DEFAULT);
    if (dset_id < 0) return PMD_ERROR_HDF5;

    /* Write unitSI (always required) */
    status = write_double_attribute(dset_id, "unitSI", unit_si);
    if (status != PMD_SUCCESS) {
        H5Dclose(dset_id);
        return status;
    }

    /* Write unitDimension and timeOffset if provided (for scalar records) */
    if (unit_dim) {
        status = write_unit_dimension_attribute(dset_id, unit_dim);
        if (status != PMD_SUCCESS) {
            H5Dclose(dset_id);
            return status;
        }

        status = write_double_attribute(dset_id, "timeOffset", time_offset);
        if (status != PMD_SUCCESS) {
            H5Dclose(dset_id);
            return status;
        }
    }

    H5Dclose(dset_id);
    return PMD_SUCCESS;
}

/**
 * Write vector record with 3 components (x, y, z)
 * Creates a group and writes component datasets with proper attributes
 *
 * @param parent_group_id Parent group to create record group in
 * @param record_name Name of the record group (e.g., "position", "momentum")
 * @param component_data Array of 3 pointers to component data [x, y, z]
 * @param num_particles Number of particles
 * @param unit_si Unit conversion factor for components
 * @param unit_dim Unit dimension for the record (written on group)
 * @param time_offset Time offset for the record (written on group)
 * @return PMD_SUCCESS or error code
 */
static pmd_status write_vector_record(hid_t parent_group_id, const char *record_name,
                                       const double **component_data, int64_t num_particles,
                                       double unit_si, const pmd_unit_dimension *unit_dim,
                                       double time_offset) {
    hid_t record_group_id = -1;
    pmd_status status;
    const char *component_names[] = {"x", "y", "z"};

    /* Create record group */
    record_group_id = H5Gcreate(parent_group_id, record_name,
                                 H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (record_group_id < 0) {
        return PMD_ERROR_HDF5;
    }

    /* Write record-level attributes */
    status = write_unit_dimension_attribute(record_group_id, unit_dim);
    if (status != PMD_SUCCESS) {
        H5Gclose(record_group_id);
        return status;
    }

    status = write_double_attribute(record_group_id, "timeOffset", time_offset);
    if (status != PMD_SUCCESS) {
        H5Gclose(record_group_id);
        return status;
    }

    /* Write component datasets */
    for (int i = 0; i < 3; i++) {
        if (component_data[i]) {
            /* Write dataset */
            status = _write_double_dataset(record_group_id, component_names[i],
                                           component_data[i], num_particles);
            if (status != PMD_SUCCESS) {
                H5Gclose(record_group_id);
                return status;
            }

            /* Add unitSI attribute to component */
            hid_t dset_id = H5Dopen(record_group_id, component_names[i], H5P_DEFAULT);
            if (dset_id < 0) {
                H5Gclose(record_group_id);
                return PMD_ERROR_HDF5;
            }

            status = write_double_attribute(dset_id, "unitSI", unit_si);
            H5Dclose(dset_id);
            if (status != PMD_SUCCESS) {
                H5Gclose(record_group_id);
                return status;
            }
        }
    }

    H5Gclose(record_group_id);
    return PMD_SUCCESS;
}

static pmd_status write_int64_dataset(hid_t group_id, const char *name, const int64_t *data,
                                       int64_t num_particles,
                                       const pmd_unit_dimension *unit_dim, double time_offset) {
    hid_t dspace_id, dset_id;
    hsize_t dims[1] = {(hsize_t)num_particles};
    pmd_status status;

    dspace_id = H5Screate_simple(1, dims, NULL);
    if (dspace_id < 0) return PMD_ERROR_HDF5;

    dset_id = H5Dcreate(group_id, name, H5T_STD_I64LE, dspace_id,
                        H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (dset_id < 0) {
        H5Sclose(dspace_id);
        return PMD_ERROR_HDF5;
    }

    if (H5Dwrite(dset_id, H5T_NATIVE_INT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, data) < 0) {
        H5Dclose(dset_id);
        H5Sclose(dspace_id);
        return PMD_ERROR_HDF5;
    }

    /* Write unitDimension if provided (for scalar records) */
    if (unit_dim) {
        status = write_unit_dimension_attribute(dset_id, unit_dim);
        if (status != PMD_SUCCESS) {
            H5Dclose(dset_id);
            H5Sclose(dspace_id);
            return status;
        }

        /* Write timeOffset for scalar records */
        status = write_double_attribute(dset_id, "timeOffset", time_offset);
        if (status != PMD_SUCCESS) {
            H5Dclose(dset_id);
            H5Sclose(dspace_id);
            return status;
        }

        /* Write unitSI for scalar records */
        status = write_double_attribute(dset_id, "unitSI", 1.0);
        if (status != PMD_SUCCESS) {
            H5Dclose(dset_id);
            H5Sclose(dspace_id);
            return status;
        }
    }

    H5Dclose(dset_id);
    H5Sclose(dspace_id);
    return PMD_SUCCESS;
}

pmd_status pmd_write_particle_group(pmd_iteration *iter, const particle_group *pg) {
    pmd_status status = PMD_SUCCESS;
    hid_t particles_group_id = -1, species_group_id = -1;
    char *particles_path = NULL;
    const double *position_components[3];
    const double *momentum_components[3];
    double *offset_x, *offset_y, *offset_z;
    int allocated_offset_x = 0, allocated_offset_y = 0, allocated_offset_z = 0;

    /* Unit dimensions for records */
    pmd_unit_dimension position_dim = {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};  /* length */
    pmd_unit_dimension momentum_dim = {1.0, 1.0, -1.0, 0.0, 0.0, 0.0, 0.0};  /* kg*m/s */
    pmd_unit_dimension time_dim = {0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0};  /* time */
    pmd_unit_dimension dimensionless = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};  /* dimensionless */

    /* Validate inputs */
    if (!iter || !pg) return PMD_ERROR_NULL_POINTER;
    if (!pg->species_type) {
        pmd_log(PMD_LOG_ERROR, "species_type is required\n");
        return PMD_ERROR_NULL_POINTER;
    }

    /* Check write mode */
    if (iter->series->access_mode == PMD_RDONLY) {
        pmd_log(PMD_LOG_ERROR, "Cannot write in read-only mode\n");
        return PMD_ERROR;
    }

    /* Validate required position fields */
    if (!pg->x || !pg->y || !pg->z) {
        pmd_log(PMD_LOG_ERROR, "Position arrays (x, y, z) are required\n");
        return PMD_ERROR_NULL_POINTER;
    }
    if (pg->num_particles <= 0) {
        pmd_log(PMD_LOG_ERROR, "num_particles must be positive\n");
        return PMD_ERROR;
    }

    /* Get particles path */
    status = pmd_get_particles_path(iter->series, &particles_path);
    if (status != PMD_SUCCESS) {
        pmd_log(PMD_LOG_ERROR, "Series has no particlesPath\n");
        return PMD_ERROR;
    }

    size_t len = strlen(particles_path);
    if (len > 0 && particles_path[len-1] == '/') {
        particles_path[len-1] = '\0';
    }

    /* Open particles group */
    particles_group_id = H5Gopen(iter->iteration_group_id, particles_path, H5P_DEFAULT);
    if (particles_group_id < 0) {
        status = PMD_ERROR_FILE_FORMAT;
        goto cleanup;
    }

    /* Create species group */
    species_group_id = H5Gcreate(particles_group_id, pg->species_type,
                                  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (species_group_id < 0) {
        status = PMD_ERROR_HDF5;
        goto cleanup;
    }

    /* Write attributes */
    status = write_int64_attribute(species_group_id, "numParticles", pg->num_particles);
    if (status != PMD_SUCCESS) goto cleanup;

    status = write_string_attribute(species_group_id, "speciesType", pg->species_type);
    if (status != PMD_SUCCESS) goto cleanup;

    /* Write position record using vector record writer */
    position_components[0] = pg->x;
    position_components[1] = pg->y;
    position_components[2] = pg->z;
    status = write_vector_record(species_group_id, "position", position_components,
                                  pg->num_particles, 1.0, &position_dim, 0.0);
    if (status != PMD_SUCCESS) goto cleanup;

    /* Write positionOffset - use zeros if not provided */
    offset_x = pg->x_offset;
    offset_y = pg->y_offset;
    offset_z = pg->z_offset;

    if (!offset_x) {
        offset_x = (double*)calloc(pg->num_particles, sizeof(double));
        if (!offset_x) {
            status = PMD_ERROR_OUT_OF_MEMORY;
            goto cleanup;
        }
        allocated_offset_x = 1;
    }
    if (!offset_y) {
        offset_y = (double*)calloc(pg->num_particles, sizeof(double));
        if (!offset_y) {
            if (allocated_offset_x) free(offset_x);
            status = PMD_ERROR_OUT_OF_MEMORY;
            goto cleanup;
        }
        allocated_offset_y = 1;
    }
    if (!offset_z) {
        offset_z = (double*)calloc(pg->num_particles, sizeof(double));
        if (!offset_z) {
            if (allocated_offset_x) free(offset_x);
            if (allocated_offset_y) free(offset_y);
            status = PMD_ERROR_OUT_OF_MEMORY;
            goto cleanup;
        }
        allocated_offset_z = 1;
    }

    position_components[0] = offset_x;
    position_components[1] = offset_y;
    position_components[2] = offset_z;
    status = write_vector_record(species_group_id, "positionOffset", position_components,
                                  pg->num_particles, 1.0, &position_dim, 0.0);

    if (allocated_offset_x) free(offset_x);
    if (allocated_offset_y) free(offset_y);
    if (allocated_offset_z) free(offset_z);

    if (status != PMD_SUCCESS) goto cleanup;

    /* Write momentum record if present */
    if (pg->px || pg->py || pg->pz) {
        momentum_components[0] = pg->px;
        momentum_components[1] = pg->py;
        momentum_components[2] = pg->pz;
        status = write_vector_record(species_group_id, "momentum", momentum_components,
                                      pg->num_particles, EV_C_TO_SI, &momentum_dim, 0.0);
        if (status != PMD_SUCCESS) goto cleanup;
    }

    /* Write time as scalar record if present (at species level) */
    if (pg->t) {
        status = write_double_dataset(species_group_id, "time", pg->t, pg->num_particles, 1.0, &time_dim, 0.0);
        if (status != PMD_SUCCESS) goto cleanup;
    }

    /* Write optional scalar records (dimensionless) */
    if (pg->weight) {
        status = write_double_dataset(species_group_id, "weight", pg->weight, pg->num_particles, 1.0, &dimensionless, 0.0);
        if (status != PMD_SUCCESS) goto cleanup;
    }
    if (pg->status) {
        status = write_int64_dataset(species_group_id, "particleStatus", pg->status, pg->num_particles, &dimensionless, 0.0);
        if (status != PMD_SUCCESS) goto cleanup;
    }
    if (pg->id) {
        status = write_int64_dataset(species_group_id, "id", pg->id, pg->num_particles, &dimensionless, 0.0);
        if (status != PMD_SUCCESS) goto cleanup;
    }

cleanup:
    if (species_group_id >= 0) H5Gclose(species_group_id);
    if (particles_group_id >= 0) H5Gclose(particles_group_id);
    free(particles_path);
    return status;
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