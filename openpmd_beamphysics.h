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

#endif /* OPENPMD_BeamPhysics_C_IMPLEMENTATION */

#endif