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
    PMD_ERROR_INVALID_FORMAT = -7,   /* Invalid OpenPMD format */
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
 * Legacy Structures (deprecated - use new API)
 * ========================================================================= */

/**
 * BeamPhysicsMD - Metadata for an OpenPMD BeamPhysics file
 *
 * Stores particle counts per species (like a dict[str, int])
 */
typedef struct {
    int num_species;             /* Number of species in file */
    char **species_names;        /* Array of species names */
    int64_t *num_particles;      /* Array of particle counts per species */
} BeamPhysicsMD;

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
    double *px;                  /* x momentum (kg⋅m/s) */
    double *py;                  /* y momentum (kg⋅m/s) */
    double *pz;                  /* z momentum (kg⋅m/s) */

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
 * Read particle group (library-allocated)
 *
 * Allocates a new ParticleGroup and reads data into it.
 * User must call pmd_free_particle_group() when done.
 *
 * @param iter Iteration handle
 * @param species Species name
 * @param pg_out Output pointer to allocated ParticleGroup
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_read_particle_group(pmd_iteration *iter, const char *species,
                                    ParticleGroup **pg_out);

/**
 * Read particle group into user-provided buffer
 *
 * Reads data into a user-allocated ParticleGroup.
 * User must pre-allocate arrays in the ParticleGroup.
 *
 * @param iter Iteration handle
 * @param species Species name
 * @param pg Pre-allocated ParticleGroup to fill
 * @return PMD_SUCCESS or error code
 */
pmd_status pmd_read_particle_group_into(pmd_iteration *iter, const char *species,
                                         ParticleGroup *pg);

/**
 * Free a library-allocated ParticleGroup
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

/* =========================================================================
 * Legacy Function Declarations (deprecated)
 * ========================================================================= */

/**
 * Read metadata from a BeamPhysics HDF5 file
 *
 * @param filename Path to the HDF5 file
 * @param metadata Pointer to BeamPhysicsMD struct to populate
 * @return 0 on success, -1 on error
 */
int beamphysics_read_metadata(const char *filename, BeamPhysicsMD *metadata);

/**
 * Free memory allocated in BeamPhysicsMD struct
 *
 * @param metadata Pointer to BeamPhysicsMD struct to free
 */
void beamphysics_free_metadata(BeamPhysicsMD *metadata);

/**
 * Allocate memory for a ParticleGroup based on metadata
 *
 * @param pg Pointer to ParticleGroup struct to allocate
 * @param species_name Name of the species to allocate for
 * @param metadata Pointer to BeamPhysicsMD containing species info
 * @return 0 on success, -1 on error
 */
int beamphysics_allocate_particle_group(ParticleGroup *pg,
                                        const char *species_name,
                                        const BeamPhysicsMD *metadata);

/**
 * Read particle data from HDF5 file into a ParticleGroup
 * Only reads into non-NULL array pointers (allows selective reading)
 *
 * @param filename Path to the HDF5 file
 * @param species_name Name of the species to read
 * @param pg Pointer to ParticleGroup with pre-allocated arrays
 * @return 0 on success, -1 on error
 */
int beamphysics_read_particle_group(const char *filename,
                                    const char *species_name,
                                    ParticleGroup *pg);

/**
 * Free memory allocated in ParticleGroup struct
 *
 * @param pg Pointer to ParticleGroup struct to free
 */
void beamphysics_free_particle_group(ParticleGroup *pg);

/* =========================================================================
 * Implementation
 * ========================================================================= */

#ifdef PARCEL_IMPLEMENTATION

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <libgen.h>  /* For dirname() */

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
    hid_t attr_id, atype_id, aspace_id;
    size_t size;
    char *str_value;

    /* Open attribute */
    attr_id = H5Aopen(loc_id, attr_name, H5P_DEFAULT);
    if (attr_id < 0) {
        return PMD_ERROR_HDF5;
    }

    /* Get attribute type and size */
    atype_id = H5Aget_type(attr_id);
    size = H5Tget_size(atype_id);

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
    *value_out = str_value;

    H5Tclose(atype_id);
    H5Aclose(attr_id);
    return PMD_SUCCESS;
}

/* =========================================================================
 * Series Operations Implementation
 * ========================================================================= */

pmd_status pmd_open_series(const char *filename, pmd_series **series_out) {
    pmd_series *series = NULL;
    hid_t file_id = -1;
    char *iter_encoding_str = NULL;
    pmd_status status = PMD_SUCCESS;

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

    /* Open HDF5 file */
    file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file_id < 0) {
        free(series);
        return PMD_ERROR_FILE_NOT_FOUND;
    }

    /* Read required root attributes */
    status = read_string_attribute(file_id, "basePath", &series->base_path);
    if (status != PMD_SUCCESS) goto cleanup;

    status = read_string_attribute(file_id, "iterationFormat", &series->iteration_format);
    if (status != PMD_SUCCESS) goto cleanup;

    status = read_string_attribute(file_id, "iterationEncoding", &iter_encoding_str);
    if (status != PMD_SUCCESS) goto cleanup;

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
        status = PMD_ERROR_INVALID_FORMAT;
        goto cleanup;
    }

    free(iter_encoding_str);
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

/* =========================================================================
 * Legacy Helper Functions
 * ========================================================================= */

/* Helper struct for counting species */
typedef struct {
    int count;
    char **names;
    int64_t *num_particles;
} SpeciesIterData;

/* Helper function to read a record (dataset or constant) of any type
 * Returns: 1 = success, 0 = field not found, -1 = field found but read failed */
static int read_record(hid_t species_group_id, const char *name,
                       void *ptr, hid_t h5_type, size_t elem_size,
                       int64_t num_particles, int required,
                       const char *group_path) {
    if (ptr == NULL) return 0;

    int field_exists = 0;
    int read_success = 0;
    hid_t dataset_id, group_id, attr_id;

    /* Try to open as dataset first */
    dataset_id = H5Dopen(species_group_id, name, H5P_DEFAULT);
    if (dataset_id >= 0) {
        field_exists = 1;
        /* It's a dataset - read the array */
        if (H5Dread(dataset_id, h5_type, H5S_ALL, H5S_ALL, H5P_DEFAULT, ptr) >= 0) {
            read_success = 1;
        }
        H5Dclose(dataset_id);
    } else {
        /* Try as constant record (group with 'value' attribute) */
        group_id = H5Gopen(species_group_id, name, H5P_DEFAULT);
        if (group_id >= 0) {
            field_exists = 1;
            attr_id = H5Aopen(group_id, "value", H5P_DEFAULT);
            if (attr_id >= 0) {
                /* Read constant value into temporary buffer */
                char constant_buffer[16];  /* Large enough for double or int64_t */
                if (H5Aread(attr_id, h5_type, constant_buffer) >= 0) {
                    /* Fill array with constant value */
                    char *array_ptr = (char *)ptr;
                    for (int64_t i = 0; i < num_particles; i++) {
                        memcpy(array_ptr + i * elem_size, constant_buffer, elem_size);
                    }
                    read_success = 1;
                }
                H5Aclose(attr_id);
            }
            H5Gclose(group_id);
        }
    }

    /* Handle errors and warnings */
    if (!field_exists) {
        if (required) {
            fprintf(stderr, "Error: Required field '%s' not found in %s\n",
                    name, group_path);
        }
        return 0;  /* Field not found */
    } else if (!read_success) {
        fprintf(stderr, "Warning: Field '%s' exists but failed to read from %s\n",
                name, group_path);
        return -1;  /* Field found but read failed */
    }

    return 1;  /* Success */
}

/* Callback for H5Literate to count and collect species */
static herr_t count_species_callback(hid_t loc_id, const char *name,
                                     const H5L_info_t *info, void *op_data) {
    SpeciesIterData *data = (SpeciesIterData *)op_data;
    hid_t group_id, attr_id;
    int64_t num_particles;

    /* Open the species group */
    group_id = H5Gopen(loc_id, name, H5P_DEFAULT);
    if (group_id < 0) return 0;  /* Skip on error */

    /* Read numParticles attribute */
    attr_id = H5Aopen(group_id, "numParticles", H5P_DEFAULT);
    if (attr_id < 0) {
        H5Gclose(group_id);
        return 0;  /* Skip if no numParticles */
    }

    if (H5Aread(attr_id, H5T_NATIVE_INT64, &num_particles) < 0) {
        H5Aclose(attr_id);
        H5Gclose(group_id);
        return 0;  /* Skip on read error */
    }

    /* Store the species name and particle count */
    data->names[data->count] = strdup(name);
    data->num_particles[data->count] = num_particles;
    data->count++;

    H5Aclose(attr_id);
    H5Gclose(group_id);
    return 0;
}

int beamphysics_read_metadata(const char *filename, BeamPhysicsMD *metadata) {
    hid_t file_id = -1;
    hid_t particles_group_id = -1;
    hsize_t num_objs;
    SpeciesIterData iter_data;

    /* Initialize metadata */
    metadata->num_species = 0;
    metadata->species_names = NULL;
    metadata->num_particles = NULL;

    /* Open the file */
    file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file_id < 0) {
        fprintf(stderr, "Error: Failed to open file: %s\n", filename);
        return -1;
    }

    /* Open /particles group */
    particles_group_id = H5Gopen(file_id, "/particles", H5P_DEFAULT);
    if (particles_group_id < 0) {
        fprintf(stderr, "Error: Failed to open /particles group\n");
        H5Fclose(file_id);
        return -1;
    }

    /* Get number of objects in /particles */
    if (H5Gget_num_objs(particles_group_id, &num_objs) < 0) {
        fprintf(stderr, "Error: Failed to get number of species\n");
        H5Gclose(particles_group_id);
        H5Fclose(file_id);
        return -1;
    }

    /* Allocate temporary arrays for iteration */
    iter_data.count = 0;
    iter_data.names = (char **)calloc(num_objs, sizeof(char *));
    iter_data.num_particles = (int64_t *)calloc(num_objs, sizeof(int64_t));

    if (!iter_data.names || !iter_data.num_particles) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(iter_data.names);
        free(iter_data.num_particles);
        H5Gclose(particles_group_id);
        H5Fclose(file_id);
        return -1;
    }

    /* Iterate through species groups */
    H5Literate(particles_group_id, H5_INDEX_NAME, H5_ITER_NATIVE,
               NULL, count_species_callback, &iter_data);

    /* Copy results to metadata struct */
    metadata->num_species = iter_data.count;
    metadata->species_names = iter_data.names;
    metadata->num_particles = iter_data.num_particles;

    /* Clean up */
    H5Gclose(particles_group_id);
    H5Fclose(file_id);

    return 0;
}

void beamphysics_free_metadata(BeamPhysicsMD *metadata) {
    if (metadata) {
        if (metadata->species_names) {
            for (int i = 0; i < metadata->num_species; i++) {
                free(metadata->species_names[i]);
            }
            free(metadata->species_names);
        }
        free(metadata->num_particles);
        metadata->num_species = 0;
        metadata->species_names = NULL;
        metadata->num_particles = NULL;
    }
}

int beamphysics_allocate_particle_group(ParticleGroup *pg,
                                        const char *species_name,
                                        const BeamPhysicsMD *metadata) {
    int64_t num_particles = 0;

    /* Find the species in metadata */
    for (int i = 0; i < metadata->num_species; i++) {
        if (strcmp(metadata->species_names[i], species_name) == 0) {
            num_particles = metadata->num_particles[i];
            break;
        }
    }

    if (num_particles == 0) {
        fprintf(stderr, "Error: Species '%s' not found in metadata\n", species_name);
        return -1;
    }

    /* Initialize all pointers to NULL */
    memset(pg, 0, sizeof(ParticleGroup));

    /* Set metadata */
    pg->num_particles = num_particles;
    pg->species_type = strdup(species_name);

    /* Allocate position arrays */
    pg->x = (double *)calloc(num_particles, sizeof(double));
    pg->y = (double *)calloc(num_particles, sizeof(double));
    pg->z = (double *)calloc(num_particles, sizeof(double));
    pg->t = (double *)calloc(num_particles, sizeof(double));

    /* Allocate momentum arrays */
    pg->px = (double *)calloc(num_particles, sizeof(double));
    pg->py = (double *)calloc(num_particles, sizeof(double));
    pg->pz = (double *)calloc(num_particles, sizeof(double));

    /* Allocate optional arrays */
    pg->weight = (double *)calloc(num_particles, sizeof(double));
    pg->status = (int64_t *)calloc(num_particles, sizeof(int64_t));
    pg->id = (int64_t *)calloc(num_particles, sizeof(int64_t));

    /* Check allocation success */
    if (!pg->species_type || !pg->x || !pg->y || !pg->z || !pg->t ||
        !pg->px || !pg->py || !pg->pz || !pg->weight || !pg->status || !pg->id) {
        fprintf(stderr, "Error: Memory allocation failed for ParticleGroup\n");
        beamphysics_free_particle_group(pg);
        return -1;
    }

    return 0;
}

int beamphysics_read_particle_group(const char *filename,
                                    const char *species_name,
                                    ParticleGroup *pg) {
    hid_t file_id = -1;
    hid_t species_group_id = -1;
    char group_path[256];
    int read_failed = 0;  /* Sentinel for tracking read failures */

    /* Suppress HDF5 error messages for expected failures (trying dataset vs group) */
    H5Eset_auto(H5E_DEFAULT, NULL, NULL);

    /* Open the file */
    file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file_id < 0) {
        fprintf(stderr, "Error: Failed to open file: %s\n", filename);
        return -1;
    }

    /* Construct path to species group */
    snprintf(group_path, sizeof(group_path), "/particles/%s", species_name);

    /* Open the species group */
    species_group_id = H5Gopen(file_id, group_path, H5P_DEFAULT);
    if (species_group_id < 0) {
        fprintf(stderr, "Error: Failed to open species group: %s\n", group_path);
        H5Fclose(file_id);
        return -1;
    }

    /* Read position vector components (REQUIRED)
     * Return codes: 1=success, 0=not found, -1=read error */
    int result;
    result = read_record(species_group_id, "position/x", pg->x, H5T_NATIVE_DOUBLE,
                         sizeof(double), pg->num_particles, 1, group_path);
    read_failed |= (result != 1);

    result = read_record(species_group_id, "position/y", pg->y, H5T_NATIVE_DOUBLE,
                         sizeof(double), pg->num_particles, 1, group_path);
    read_failed |= (result != 1);

    result = read_record(species_group_id, "position/z", pg->z, H5T_NATIVE_DOUBLE,
                         sizeof(double), pg->num_particles, 1, group_path);
    read_failed |= (result != 1);

    /* Read time (optional - default to NaN if not found) */
    result = read_record(species_group_id, "time", pg->t, H5T_NATIVE_DOUBLE,
                         sizeof(double), pg->num_particles, 0, group_path);
    if (result != 1 && pg->t != NULL) {
        for (int64_t i = 0; i < pg->num_particles; i++) {
            pg->t[i] = NAN;
        }
    }

    /* Read momentum vector components (optional - default to NaN) */
    result = read_record(species_group_id, "momentum/x", pg->px, H5T_NATIVE_DOUBLE,
                         sizeof(double), pg->num_particles, 0, group_path);
    if (result != 1 && pg->px != NULL) {
        for (int64_t i = 0; i < pg->num_particles; i++) {
            pg->px[i] = NAN;
        }
    }

    result = read_record(species_group_id, "momentum/y", pg->py, H5T_NATIVE_DOUBLE,
                         sizeof(double), pg->num_particles, 0, group_path);
    if (result != 1 && pg->py != NULL) {
        for (int64_t i = 0; i < pg->num_particles; i++) {
            pg->py[i] = NAN;
        }
    }

    result = read_record(species_group_id, "momentum/z", pg->pz, H5T_NATIVE_DOUBLE,
                         sizeof(double), pg->num_particles, 0, group_path);
    if (result != 1 && pg->pz != NULL) {
        for (int64_t i = 0; i < pg->num_particles; i++) {
            pg->pz[i] = NAN;
        }
    }

    /* Read weight (optional - default to 1.0) */
    result = read_record(species_group_id, "weight", pg->weight, H5T_NATIVE_DOUBLE,
                         sizeof(double), pg->num_particles, 0, group_path);
    if (result != 1 && pg->weight != NULL) {
        for (int64_t i = 0; i < pg->num_particles; i++) {
            pg->weight[i] = 1.0;
        }
    }

    /* Read particleStatus (optional - default to 1 = alive) */
    result = read_record(species_group_id, "particleStatus", pg->status, H5T_NATIVE_INT64,
                         sizeof(int64_t), pg->num_particles, 0, group_path);
    if (result != 1 && pg->status != NULL) {
        for (int64_t i = 0; i < pg->num_particles; i++) {
            pg->status[i] = 1;
        }
    }

    /* Read id (optional - default to incrementing values starting at 0) */
    result = read_record(species_group_id, "id", pg->id, H5T_NATIVE_INT64,
                         sizeof(int64_t), pg->num_particles, 0, group_path);
    if (result != 1 && pg->id != NULL) {
        for (int64_t i = 0; i < pg->num_particles; i++) {
            pg->id[i] = i;
        }
    }

    /* Clean up */
    H5Gclose(species_group_id);
    H5Fclose(file_id);

    /* Return error if any required field failed */
    if (read_failed) {
        fprintf(stderr, "Error: Failed to read one or more required fields\n");
        return -1;
    }

    return 0;
}

void beamphysics_free_particle_group(ParticleGroup *pg) {
    if (pg) {
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
    }
}

#endif /* PARCEL_IMPLEMENTATION */

#endif /* PARCEL_H */