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
 * Particle Data Structures
 * ========================================================================= */

/**
 * ParticleGroup - Represents a collection of particles
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

} ParticleGroup;

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

    /* OpenPMD metadata */
    char *base_path;                  /* e.g., "/data/%T/" */
    char *particles_path;             /* e.g., "particles/" */
    char *iteration_format;           /* e.g., "sim_%T.h5" or "/data/%T/" */

    /* Iteration encoding type */
    pmd_iteration_encoding iteration_encoding;

    /* For FILE_BASED: filename pattern */
    char *filename_pattern;           /* e.g., "simulation_%T.h5" */
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

/* --- Particle Data Operations --- */

/**
 * Allocate memory for a ParticleGroup
 *
 * Allocates all arrays based on the number of particles for the species.
 * User must call pmd_free_particle_group() when done.
 *
 * @param iter Iteration handle
 * @param species Species name
 * @param pg_out Output pointer to allocated ParticleGroup
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_allocate_particle_group(pmd_iteration *iter, const char *species,
                                        ParticleGroup **pg_out);

/**
 * Read particle group data
 *
 * Reads particle data into a pre-allocated ParticleGroup.
 * The ParticleGroup arrays must be allocated before calling (e.g., via pmd_allocate_particle_group).
 *
 * @param iter Iteration handle
 * @param species Species name
 * @param pg Pre-allocated ParticleGroup to fill
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_read_particle_group(pmd_iteration *iter, const char *species,
                                    ParticleGroup *pg);

/**
 * Free a ParticleGroup and its arrays
 *
 * @param pg ParticleGroup to free
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_free_particle_group(ParticleGroup *pg);

/**
 * Write particle group to iteration (not yet implemented)
 *
 * @param iter Iteration handle
 * @param species Species name
 * @param pg ParticleGroup to write
 * @return PMD_ERROR (not implemented)
 */
pmd_status pmd_write_particle_group(pmd_iteration *iter, const char *species,
                                     const ParticleGroup *pg);

/* --- Utility Functions --- */

/**
 * Check if a filename matches a pattern where %T can be any sequence of digits
 *
 * @param filename Filename to check
 * @param pattern Pattern with %T placeholder
 * @return 1 if match, 0 otherwise
 */
int matches_pattern(const char *filename, const char *pattern);

/* =========================================================================
 * Implementation
 * ========================================================================= */

#ifdef PARCEL_IMPLEMENTATION

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <libgen.h>  /* For dirname() */
#include <dirent.h>  /* For directory scanning */
#include <ctype.h>   /* For isdigit() */

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

    /* Open attribute */
    attr_id = H5Aopen(loc_id, attr_name, H5P_DEFAULT);
    if (attr_id < 0) {
        return PMD_ERROR_HDF5;
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

/* =========================================================================
 * Series Operations Implementation
 * ========================================================================= */

/**
 * Check if a filename matches a pattern where %T can be any sequence of digits
 * All %T placeholders must match the same digit sequence
 * Returns 1 if match, 0 otherwise
 */
int matches_pattern(const char *filename, const char *pattern) {
    const char *p = pattern;
    const char *f = filename;
    char matched_number[64] = {0};  /* Store the first %T match */
    int first_match = 1;

    while (*p && *f) {
        if (*p == '%' && *(p + 1) == 'T') {
            /* %T should match one or more digits */
            if (!isdigit(*f)) {
                return 0;
            }

            /* Extract the digit sequence */
            const char *digit_start = f;
            while (isdigit(*f)) {
                f++;
            }
            size_t digit_len = f - digit_start;

            if (first_match) {
                /* First %T - store the matched digits */
                if (digit_len >= sizeof(matched_number)) {
                    return 0;  /* Number too long */
                }
                strncpy(matched_number, digit_start, digit_len);
                matched_number[digit_len] = '\0';
                first_match = 0;
            } else {
                /* Subsequent %T - must match the same digits */
                if (strlen(matched_number) != digit_len ||
                    strncmp(matched_number, digit_start, digit_len) != 0) {
                    return 0;
                }
            }

            p += 2; /* Skip %T */
        } else if (*p == *f) {
            p++;
            f++;
        } else {
            return 0;
        }
    }

    /* Both should be at end for a complete match */
    return (*p == '\0' && *f == '\0');
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

        /* Extract directory and pattern basename */
        char *filename_copy = strdup(filename);
        char *dir_path = dirname(filename_copy);
        char *filename_copy2 = strdup(filename);
        char *pattern_basename = basename(filename_copy2);

        /* Open directory */
        DIR *dir = opendir(dir_path);
        if (!dir) {
            free(filename_copy);
            free(filename_copy2);
            free(series);
            return PMD_ERROR_FILE_NOT_FOUND;
        }

        /* Find first file matching the pattern */
        int found = 0;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && !found) {
            if (matches_pattern(entry->d_name, pattern_basename)) {
                /* Found a matching file, construct full path */
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
                file_id = H5Fopen(full_path, H5F_ACC_RDONLY, H5P_DEFAULT);
                if (file_id >= 0) {
                    actual_filename = strdup(full_path);
                    found = 1;
                }
            }
        }

        closedir(dir);
        free(filename_copy);
        free(filename_copy2);

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

    /* Read required basePath attribute */
    status = read_string_attribute(file_id, "basePath", &series->base_path);
    if (status != PMD_SUCCESS) {
        fprintf(stderr, "Error: Missing required 'basePath' attribute in '%s'\n", filename);
        goto cleanup;
    }

    /* Read iterationFormat (use basePath as default if missing) */
    if (attribute_exists(file_id, "iterationFormat") > 0) {
        status = read_string_attribute(file_id, "iterationFormat", &series->iteration_format);
        if (status != PMD_SUCCESS) goto cleanup;
    } else {
        fprintf(stderr, "Warning: Missing 'iterationFormat' attribute in '%s', using basePath as default\n", filename);
        series->iteration_format = strdup(series->base_path);
    }

    /* Read iterationEncoding (default to groupBased if missing) */
    if (attribute_exists(file_id, "iterationEncoding") > 0) {
        status = read_string_attribute(file_id, "iterationEncoding", &iter_encoding_str);
        if (status != PMD_SUCCESS) goto cleanup;
    } else {
        fprintf(stderr, "Warning: Missing 'iterationEncoding' attribute in '%s', defaulting to 'groupBased'\n", filename);
        iter_encoding_str = strdup("groupBased");
    }

    /* Read optional particlesPath attribute */
    if (attribute_exists(file_id, "particlesPath") > 0) {
        status = read_string_attribute(file_id, "particlesPath", &series->particles_path);
        if (status != PMD_SUCCESS) goto cleanup;
    } else {
        /* Use default if attribute doesn't exist */
        series->particles_path = strdup("particles/");
    }

    /* Parse iteration encoding and handle file lifecycle */
    if (strcmp(iter_encoding_str, "fileBased") == 0) {
        series->iteration_encoding = PMD_FILE_BASED;
        /* For fileBased, extract filename pattern and directory */
        series->filename_pattern = strdup(series->iteration_format);
        char *filename_copy = strdup(filename);
        series->directory = strdup(dirname(filename_copy));
        free(filename_copy);
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
    free(series->particles_path);
    free(series->iteration_format);
    free(series->filename_pattern);
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
 * Extract parent group path and iteration pattern from basePath template
 * E.g., "/data/%T/" -> parent="/data", prefix="", suffix=""
 * E.g., "/data/step_%T_final/" -> parent="/data", prefix="step_", suffix="_final"
 */
static void parse_base_path(const char *base_path, char **parent_out,
                             char **prefix_out, char **suffix_out) {
    char *path = strdup(base_path);
    char *percent_t = strstr(path, "%T");

    if (!percent_t) {
        *parent_out = path;
        *prefix_out = strdup("");
        *suffix_out = strdup("");
        return;
    }

    /* Find the last '/' before %T to get parent path */
    char *last_slash = percent_t;
    while (last_slash > path && *last_slash != '/') {
        last_slash--;
    }

    /* Extract prefix (between last '/' and %T) */
    size_t prefix_len = percent_t - (last_slash + 1);
    *prefix_out = (char *)malloc(prefix_len + 1);
    strncpy(*prefix_out, last_slash + 1, prefix_len);
    (*prefix_out)[prefix_len] = '\0';

    /* Extract suffix (after %T until next '/' or end) */
    char *suffix_start = percent_t + 2;  /* Skip %T */
    char *next_slash = strchr(suffix_start, '/');
    if (next_slash) {
        size_t suffix_len = next_slash - suffix_start;
        *suffix_out = (char *)malloc(suffix_len + 1);
        strncpy(*suffix_out, suffix_start, suffix_len);
        (*suffix_out)[suffix_len] = '\0';
    } else {
        *suffix_out = strdup(suffix_start);
    }

    /* Extract parent path */
    *last_slash = '\0';
    *parent_out = path;
}

/**
 * Helper struct for collecting iterations during HDF5 group iteration
 */
typedef struct {
    int64_t *indices;
    int count;
    int capacity;
    char *prefix;
    char *suffix;
} IterationCollector;

/**
 * Callback for H5Literate to collect iteration group names
 */
static herr_t collect_iterations_callback(hid_t loc_id, const char *name,
                                           const H5L_info_t *info, void *op_data) {
    IterationCollector *collector = (IterationCollector *)op_data;

    /* Check if name matches pattern */
    size_t prefix_len = strlen(collector->prefix);
    size_t suffix_len = strlen(collector->suffix);
    size_t name_len = strlen(name);

    if (name_len >= prefix_len + suffix_len &&
        strncmp(name, collector->prefix, prefix_len) == 0 &&
        strcmp(name + name_len - suffix_len, collector->suffix) == 0) {

        /* Extract iteration number from middle */
        size_t iter_str_len = name_len - prefix_len - suffix_len;
        char *iter_str = (char *)malloc(iter_str_len + 1);
        strncpy(iter_str, name + prefix_len, iter_str_len);
        iter_str[iter_str_len] = '\0';

        /* Try to parse as integer */
        char *endptr;
        int64_t iteration = strtoll(iter_str, &endptr, 10);

        /* Check if entire extracted string was parsed successfully */
        if (*endptr == '\0' && endptr != iter_str) {
            /* Grow array if needed */
            if (collector->count >= collector->capacity) {
                collector->capacity = collector->capacity * 2 + 10;
                collector->indices = (int64_t *)realloc(collector->indices,
                                                         collector->capacity * sizeof(int64_t));
            }
            collector->indices[collector->count++] = iteration;
        }

        free(iter_str);
    }

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

    if (series->iteration_encoding == PMD_GROUP_BASED) {
        /* Check if this is a single-snapshot file (no %T in basePath) */
        if (!strstr(series->base_path, "%T")) {
            /* Single iteration at index 0 */
            series->iteration_indices = (int64_t *)malloc(sizeof(int64_t));
            series->iteration_indices[0] = 0;
            series->num_iterations = 1;
            *iterations = series->iteration_indices;
            *count = 1;
            return PMD_SUCCESS;
        }

        /* GROUP_BASED: enumerate groups in the data directory */
        char *parent_path, *prefix, *suffix;
        parse_base_path(series->base_path, &parent_path, &prefix, &suffix);

        hid_t group_id;
        IterationCollector collector = {NULL, 0, 0, prefix, suffix};

        /* Open parent group */
        group_id = H5Gopen(series->file_id, parent_path, H5P_DEFAULT);
        if (group_id < 0) {
            free(parent_path);
            free(prefix);
            free(suffix);
            return PMD_ERROR_HDF5;
        }

        /* Iterate through groups to find iterations */
        H5Literate(group_id, H5_INDEX_NAME, H5_ITER_NATIVE, NULL,
                   collect_iterations_callback, &collector);
        H5Gclose(group_id);

        free(parent_path);
        free(prefix);
        free(suffix);

        /* Sort the iterations */
        if (collector.count > 0) {
            qsort(collector.indices, collector.count, sizeof(int64_t), compare_int64);
        }

        /* Cache results */
        series->iteration_indices = collector.indices;
        series->num_iterations = collector.count;
        *iterations = collector.indices;
        *count = collector.count;

    } else {  /* PMD_FILE_BASED */
        /* Check if this is a single-snapshot file (no %T in filename pattern) */
        if (!strstr(series->filename_pattern, "%T")) {
            /* Single iteration at index 0 */
            series->iteration_indices = (int64_t *)malloc(sizeof(int64_t));
            series->iteration_indices[0] = 0;
            series->num_iterations = 1;
            *iterations = series->iteration_indices;
            *count = 1;
            return PMD_SUCCESS;
        }

        /* FILE_BASED: scan directory for matching files */
        DIR *dir;
        struct dirent *entry;
        IterationCollector collector = {NULL, 0, 0, NULL, NULL};

        dir = opendir(series->directory);
        if (!dir) {
            return PMD_ERROR_FILE_NOT_FOUND;
        }

        /* Find position of first %T to extract iteration number */
        const char *first_percent_t = strstr(series->filename_pattern, "%T");
        if (!first_percent_t) {
            closedir(dir);
            return PMD_ERROR_FILE_FORMAT;
        }
        size_t prefix_len = first_percent_t - series->filename_pattern;

        /* Scan directory for matching files */
        while ((entry = readdir(dir)) != NULL) {
            /* Check if filename matches pattern (handles multiple %T correctly) */
            if (matches_pattern(entry->d_name, series->filename_pattern)) {
                /* Extract iteration number at position of first %T */
                const char *iter_start = entry->d_name + prefix_len;

                /* Parse the digit sequence */
                char *endptr;
                int64_t iteration = strtoll(iter_start, &endptr, 10);

                /* Verify we parsed at least one digit */
                if (endptr != iter_start && isdigit(*iter_start)) {
                    /* Grow array if needed */
                    if (collector.count >= collector.capacity) {
                        collector.capacity = collector.capacity * 2 + 10;
                        collector.indices = (int64_t *)realloc(collector.indices,
                                                               collector.capacity * sizeof(int64_t));
                    }
                    collector.indices[collector.count++] = iteration;
                }
            }
        }

        closedir(dir);

        /* Sort the iterations */
        if (collector.count > 0) {
            qsort(collector.indices, collector.count, sizeof(int64_t), compare_int64);
        }

        /* Cache results */
        series->iteration_indices = collector.indices;
        series->num_iterations = collector.count;
        *iterations = collector.indices;
        *count = collector.count;
    }

    return PMD_SUCCESS;
}

/* =========================================================================
 * Iteration Operations Implementation
 * ========================================================================= */

/**
 * Replace %T in a template string with an iteration number
 * Returns newly allocated string (caller must free)
 */
static char* replace_iteration(const char *template, int64_t iteration) {
    char *result;
    char iter_str[32];
    snprintf(iter_str, sizeof(iter_str), "%lld", (long long)iteration);

    const char *percent_t = strstr(template, "%T");
    if (!percent_t) {
        return strdup(template);
    }

    size_t prefix_len = percent_t - template;
    size_t iter_len = strlen(iter_str);
    size_t suffix_len = strlen(percent_t + 2);

    result = (char *)malloc(prefix_len + iter_len + suffix_len + 1);
    strncpy(result, template, prefix_len);
    strcpy(result + prefix_len, iter_str);
    strcpy(result + prefix_len + iter_len, percent_t + 2);

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
 * Callback to count species groups
 */
static herr_t count_species_iteration_callback(hid_t loc_id, const char *name,
                                                const H5L_info_t *info, void *op_data) {
    int *count = (int *)op_data;
    (*count)++;
    return 0;
}

/**
 * Helper struct for collecting species during iteration
 */
typedef struct {
    char **names;
    int64_t *num_particles;
    int count;
} SpeciesCollector;

/**
 * Callback to collect species names and particle counts
 */
static herr_t collect_species_iteration_callback(hid_t loc_id, const char *name,
                                                   const H5L_info_t *info, void *op_data) {
    SpeciesCollector *collector = (SpeciesCollector *)op_data;
    hid_t species_group_id, attr_id;
    int64_t num_particles;

    /* Open species group */
    species_group_id = H5Gopen(loc_id, name, H5P_DEFAULT);
    if (species_group_id < 0) return 0;

    /* Read numParticles attribute */
    attr_id = H5Aopen(species_group_id, "numParticles", H5P_DEFAULT);
    if (attr_id >= 0) {
        if (H5Aread(attr_id, H5T_NATIVE_INT64, &num_particles) >= 0) {
            collector->names[collector->count] = strdup(name);
            collector->num_particles[collector->count] = num_particles;
            collector->count++;
        }
        H5Aclose(attr_id);
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
        char *filename = replace_iteration(series->filename_pattern, index);
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", series->directory, filename);
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
        fprintf(stderr, "Warning: Missing 'time' attribute in iteration %lld, defaulting to 0.0\n",
                (long long)index);
        iter->time = 0.0;
    }

    if (attribute_exists(iter->iteration_group_id, "dt") > 0) {
        status = read_double_attribute(iter->iteration_group_id, "dt", &iter->dt);
        if (status != PMD_SUCCESS) goto cleanup;
    } else {
        fprintf(stderr, "Warning: Missing 'dt' attribute in iteration %lld, defaulting to 0.0\n",
                (long long)index);
        iter->dt = 0.0;
    }

    /* timeUnitSI is optional */
    if (attribute_exists(iter->iteration_group_id, "timeUnitSI") > 0) {
        read_double_attribute(iter->iteration_group_id, "timeUnitSI", &iter->time_unit_si);
    } else {
        iter->time_unit_si = 1.0;  /* Default: already in SI */
    }

    /* Construct particles path and open particles group */
    particles_full_path = (char *)malloc(strlen(series->particles_path) + 1);
    strcpy(particles_full_path, series->particles_path);

    /* Remove trailing slash if present */
    size_t len = strlen(particles_full_path);
    if (len > 0 && particles_full_path[len-1] == '/') {
        particles_full_path[len-1] = '\0';
    }

    /* Open particles group relative to iteration group */
    particles_group_id = H5Gopen(iter->iteration_group_id, particles_full_path, H5P_DEFAULT);
    if (particles_group_id < 0) {
        status = PMD_ERROR_HDF5;
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
    SpeciesCollector collector = {iter->species_names, iter->num_particles, 0};
    H5Literate(particles_group_id, H5_INDEX_NAME, H5_ITER_NATIVE, NULL,
               collect_species_iteration_callback, &collector);

    H5Gclose(particles_group_id);
    free(iteration_path);
    free(particles_full_path);

    *iter_out = iter;
    return PMD_SUCCESS;

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
    free(iter->num_particles);

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
 * Constants
 * ========================================================================= */

 /* Exact speed of light (m/s) */
 #define CLIGHT 299792456

 /* Conversion from eV/c^2 to kg (CODATA recommended value 2022) */
 #define EV_C2_TO_SI 1.782661921e-36

/* Conversion factor from eV/c to SI (kg⋅m/s) */
#define EV_C_TO_SI (EV_C2_TO_SI*CLIGHT)

/* =========================================================================
 * Helper Functions - Forward Declarations
 * ========================================================================= */

static int record_exists(hid_t group_id, const char *name);
static pmd_status read_record_generic(hid_t group_id, const char *name,
                                       void *array, hid_t h5_type, size_t elem_size,
                                       int64_t num_particles, double *unit_si_out);
static pmd_status read_double_record(hid_t group_id, const char *name, double *array,
                                      int64_t num_particles, double unit_multiplier);
static pmd_status read_int64_record(hid_t group_id, const char *name, int64_t *array,
                                     int64_t num_particles);

/* =========================================================================
 * Particle Data Operations Implementation
 * ========================================================================= */

pmd_status pmd_allocate_particle_group(pmd_iteration *iter, const char *species,
                                        ParticleGroup **pg_out) {
    ParticleGroup *pg = NULL;
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

    /* Allocate ParticleGroup */
    pg = (ParticleGroup *)calloc(1, sizeof(ParticleGroup));
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
                                    ParticleGroup *pg) {
    hid_t particles_group_id = -1;
    hid_t species_group_id = -1;
    pmd_status status = PMD_SUCCESS;
    char *particles_path = NULL;

    if (!iter || !species || !pg) {
        return PMD_ERROR_NULL_POINTER;
    }

    /* Verify species exists and get particle count */
    int64_t num_particles;
    status = pmd_get_num_particles(iter, species, &num_particles);
    if (status != PMD_SUCCESS) {
        return status;
    }

    /* Suppress HDF5 error messages for expected failures */
    H5Eset_auto(H5E_DEFAULT, NULL, NULL);

    /* Construct path to particles group */
    particles_path = strdup(iter->series->particles_path);
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
        read_double_record(species_group_id, "time", pg->t, num_particles, 1.0);
    } else if (pg->t) {
        for (int64_t i = 0; i < num_particles; i++) pg->t[i] = NAN;
    }

    /* Read optional momentum components (convert from SI to eV/c) */
    if (pg->px && record_exists(species_group_id, "momentum/x")) {
        read_double_record(species_group_id, "momentum/x", pg->px, num_particles, 1.0 / EV_C_TO_SI);
    } else if (pg->px) {
        for (int64_t i = 0; i < num_particles; i++) pg->px[i] = NAN;
    }

    if (pg->py && record_exists(species_group_id, "momentum/y")) {
        read_double_record(species_group_id, "momentum/y", pg->py, num_particles, 1.0 / EV_C_TO_SI);
    } else if (pg->py) {
        for (int64_t i = 0; i < num_particles; i++) pg->py[i] = NAN;
    }

    if (pg->pz && record_exists(species_group_id, "momentum/z")) {
        read_double_record(species_group_id, "momentum/z", pg->pz, num_particles, 1.0 / EV_C_TO_SI);
    } else if (pg->pz) {
        for (int64_t i = 0; i < num_particles; i++) pg->pz[i] = NAN;
    }

    /* Read optional weight (dimensionless) */
    if (pg->weight && record_exists(species_group_id, "weight")) {
        read_double_record(species_group_id, "weight", pg->weight, num_particles, 1.0);
    } else if (pg->weight) {
        for (int64_t i = 0; i < num_particles; i++) pg->weight[i] = 1.0;
    }

    /* Read optional status */
    if (pg->status && record_exists(species_group_id, "particleStatus")) {
        read_int64_record(species_group_id, "particleStatus", pg->status, num_particles);
    } else if (pg->status) {
        for (int64_t i = 0; i < num_particles; i++) pg->status[i] = 1;
    }

    /* Read optional id */
    if (pg->id && record_exists(species_group_id, "id")) {
        read_int64_record(species_group_id, "id", pg->id, num_particles);
    } else if (pg->id) {
        for (int64_t i = 0; i < num_particles; i++) pg->id[i] = i;
    }

cleanup:
    H5Gclose(species_group_id);
    return status;
}

pmd_status pmd_free_particle_group(ParticleGroup *pg) {
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
    memset(pg, 0, sizeof(ParticleGroup));
    free(pg);

    return PMD_SUCCESS;
}

pmd_status pmd_write_particle_group(pmd_iteration *iter, const char *species,
                                     const ParticleGroup *pg) {
    /* Not yet implemented */
    fprintf(stderr, "Error: pmd_write_particle_group not yet implemented\n");
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
 * Read unitSI attribute from a record component
 * Returns the conversion factor, or 1.0 if not present
 */
static double read_unit_si(hid_t loc_id) {
    double unit_si = 1.0;
    hid_t attr_id;

    if (attribute_exists(loc_id, "unitSI") > 0) {
        attr_id = H5Aopen(loc_id, "unitSI", H5P_DEFAULT);
        if (attr_id >= 0) {
            H5Aread(attr_id, H5T_NATIVE_DOUBLE, &unit_si);
            H5Aclose(attr_id);
        }
    }

    return unit_si;
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
        /* Read the array */
        if (H5Dread(dataset_id, h5_type, H5S_ALL, H5S_ALL, H5P_DEFAULT, array) < 0) {
            H5Dclose(dataset_id);
            return PMD_ERROR_HDF5;
        }

        /* Read unitSI if requested */
        if (unit_si_out) {
            unit_si = read_unit_si(dataset_id);
            *unit_si_out = unit_si;
        }

        H5Dclose(dataset_id);
        return PMD_SUCCESS;
    }

    /* Try as constant record (group with 'value' attribute) */
    group_id_local = H5Gopen(group_id, name, H5P_DEFAULT);
    if (group_id_local >= 0) {
        attr_id = H5Aopen(group_id_local, "value", H5P_DEFAULT);
        if (attr_id >= 0) {
            char constant_buffer[16];  /* Large enough for double or int64_t */
            if (H5Aread(attr_id, h5_type, constant_buffer) >= 0) {
                /* Read unitSI if requested */
                if (unit_si_out) {
                    unit_si = read_unit_si(group_id_local);
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