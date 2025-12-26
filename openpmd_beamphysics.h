/* =========================================================================
    OpenPMD-BeamPhysics-C A C API to the OpenPMD BeamPhysics data format
    Copyright (c) 2025 Christopher M. Pierce
    SPDX-License-Identifier: BSD-3-Clause
========================================================================= */

#ifndef OPENPMD_BeamPhysics_C
#define OPENPMD_BeamPhysics_C

#include <stddef.h>
#include <stdint.h>
#include <hdf5.h>

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
 * Function Declarations
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

#ifdef OPENPMD_BeamPhysics_C_IMPLEMENTATION

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Helper struct for counting species */
typedef struct {
    int count;
    char **names;
    int64_t *num_particles;
} SpeciesIterData;

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
    hid_t dataset_id = -1;
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

    /* Helper macro to read dataset or constant record if pointer is non-NULL */
    #define READ_RECORD(name, ptr, type, ctype, required) \
        if (ptr != NULL) { \
            int success = 0; \
            /* Try to open as dataset first */ \
            dataset_id = H5Dopen(species_group_id, name, H5P_DEFAULT); \
            if (dataset_id >= 0) { \
                /* It's a dataset - read the array */ \
                if (H5Dread(dataset_id, type, H5S_ALL, H5S_ALL, H5P_DEFAULT, ptr) >= 0) { \
                    success = 1; \
                } \
                H5Dclose(dataset_id); \
            } else { \
                /* Try as constant record (group with 'value' attribute) */ \
                hid_t group_id = H5Gopen(species_group_id, name, H5P_DEFAULT); \
                if (group_id >= 0) { \
                    hid_t attr_id = H5Aopen(group_id, "value", H5P_DEFAULT); \
                    if (attr_id >= 0) { \
                        ctype constant_value; \
                        if (H5Aread(attr_id, type, &constant_value) >= 0) { \
                            /* Fill array with constant value */ \
                            for (int64_t i = 0; i < pg->num_particles; i++) { \
                                ptr[i] = constant_value; \
                            } \
                            success = 1; \
                        } \
                        H5Aclose(attr_id); \
                    } \
                    H5Gclose(group_id); \
                } \
            } \
            if (!success) { \
                if (required) { \
                    fprintf(stderr, "Error: Failed to read required field '%s' from %s\n", \
                            name, group_path); \
                    read_failed = 1; \
                } \
            } \
        }

    /* Read position vector components (REQUIRED) */
    READ_RECORD("position/x", pg->x, H5T_NATIVE_DOUBLE, double, 1);
    READ_RECORD("position/y", pg->y, H5T_NATIVE_DOUBLE, double, 1);
    READ_RECORD("position/z", pg->z, H5T_NATIVE_DOUBLE, double, 1);

    /* Read time (optional) */
    READ_RECORD("time", pg->t, H5T_NATIVE_DOUBLE, double, 0);

    /* Read momentum vector components (optional) */
    READ_RECORD("momentum/x", pg->px, H5T_NATIVE_DOUBLE, double, 0);
    READ_RECORD("momentum/y", pg->py, H5T_NATIVE_DOUBLE, double, 0);
    READ_RECORD("momentum/z", pg->pz, H5T_NATIVE_DOUBLE, double, 0);

    /* Read optional arrays */
    READ_RECORD("weight", pg->weight, H5T_NATIVE_DOUBLE, double, 0);
    READ_RECORD("particleStatus", pg->status, H5T_NATIVE_INT64, int64_t, 0);
    READ_RECORD("id", pg->id, H5T_NATIVE_INT64, int64_t, 0);

    #undef READ_RECORD

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

#endif /* OPENPMD_BeamPhysics_C_IMPLEMENTATION */

#endif