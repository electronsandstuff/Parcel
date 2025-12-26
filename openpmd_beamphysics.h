/* =========================================================================
    OpenPMD-Beamphysics-C A C API to the OpenPMD Beamphysics data format
    Copyright (c) 2025 Christopher M. Pierce
    SPDX-License-Identifier: BSD-3-Clause
========================================================================= */

#ifndef OPENPMD_BEAMPHYSICS_C
#define OPENPMD_BEAMPHYSICS_C

#include <stddef.h>
#include <stdint.h>

/**
 * ParticleGroup - Represents a collection of particles
 */
typedef struct {
    int64_t numParticles;        /* Number of particles in group */
    char *speciesType;           /* Species name (e.g., "electron") */

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
#ifdef OPENPMD_BEAMPHYSICS_C_IMPLEMENTATION
#endif

#endif