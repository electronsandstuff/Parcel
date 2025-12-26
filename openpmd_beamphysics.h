/* =========================================================================
    OpenPMD-BeamPhysics-C A C API to the OpenPMD BeamPhysics data format
    Copyright (c) 2025 Christopher M. Pierce
    SPDX-License-Identifier: BSD-3-Clause
========================================================================= */

#ifndef OPENPMD_BeamPhysics_C
#define OPENPMD_BeamPhysics_C

#include <stddef.h>
#include <stdint.h>

/**
 * BeamPhysicsMD - Metadata for an OpenPMD BeamPhysics file
 */
typedef struct {
    int64_t num_particles;       /* Total number of particles in file */
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

/* Implementation, to be included only once in a single user C file */
#ifdef OPENPMD_BeamPhysics_C_IMPLEMENTATION
#endif

#endif