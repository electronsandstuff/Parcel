/**
 * Test to generate fully featured OpenPMD files with particle groups
 * Creates both group-based and file-based iteration encoding examples
 */

#include "../deps/Unity-2.6.1/unity.h"

#define PARCEL_IMPLEMENTATION
#include "../parcel.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <unistd.h>
#endif

#define TEST_OUTPUT_DIR "tests/generated_openpmd"

void setUp(void) {
    /* Create output directory if it doesn't exist */
    mkdir(TEST_OUTPUT_DIR, 0755);
}

void tearDown(void) {
    /* Leave generated files for inspection */
}

/**
 * Generate particle data for a given iteration
 * Values are iteration-dependent to make each iteration unique
 */
static void generate_particle_data(int64_t iter_idx, int64_t num_particles,
                                     particle_group *pg) {
    for (int64_t i = 0; i < num_particles; i++) {
        /* Position in meters - varies by particle and iteration */
        pg->x[i] = 0.001 * (double)i + 0.0001 * (double)iter_idx;
        pg->y[i] = 0.002 * (double)i + 0.0002 * (double)iter_idx;
        pg->z[i] = 0.01 * (double)i + 0.001 * (double)iter_idx;

        /* Time in seconds - varies by particle and iteration */
        pg->t[i] = 1e-9 * (double)i + 1e-8 * (double)iter_idx;

        /* Momentum in eV/c - typical electron beam energies */
        pg->px[i] = 1e6 + (double)i * 1e5 + (double)iter_idx * 1e4;
        pg->py[i] = 5e5 + (double)i * 5e4 + (double)iter_idx * 5e3;
        pg->pz[i] = 1e8 + (double)i * 1e6 + (double)iter_idx * 1e5;

        /* Weight - number of real particles per macroparticle */
        pg->weight[i] = 1e10 + (double)i * 1e9 + (double)iter_idx * 1e8;

        /* Status - 1 for alive particles */
        pg->status[i] = 1;

        /* Unique particle ID */
        pg->id[i] = 1000 * iter_idx + i;
    }
}

/**
 * Test: Generate group-based OpenPMD file with multiple iterations
 */
void test_generate_group_based_openpmd(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;
    particle_group pg;
    const int64_t NUM_PARTICLES = 10;
    const int64_t NUM_ITERATIONS = 3;

    /* Allocate particle arrays */
    double *x = (double*)malloc(NUM_PARTICLES * sizeof(double));
    double *y = (double*)malloc(NUM_PARTICLES * sizeof(double));
    double *z = (double*)malloc(NUM_PARTICLES * sizeof(double));
    double *t = (double*)malloc(NUM_PARTICLES * sizeof(double));
    double *px = (double*)malloc(NUM_PARTICLES * sizeof(double));
    double *py = (double*)malloc(NUM_PARTICLES * sizeof(double));
    double *pz = (double*)malloc(NUM_PARTICLES * sizeof(double));
    double *weight = (double*)malloc(NUM_PARTICLES * sizeof(double));
    int64_t *status = (int64_t*)malloc(NUM_PARTICLES * sizeof(int64_t));
    int64_t *id = (int64_t*)malloc(NUM_PARTICLES * sizeof(int64_t));

    /* Setup particle group structure */
    memset(&pg, 0, sizeof(particle_group));
    pg.num_particles = NUM_PARTICLES;
    pg.species_type = "electron";
    pg.x = x;
    pg.y = y;
    pg.z = z;
    pg.t = t;
    pg.px = px;
    pg.py = py;
    pg.pz = pz;
    pg.weight = weight;
    pg.status = status;
    pg.id = id;

    /* Create group-based series */
    result = pmd_open_series(TEST_OUTPUT_DIR "/example_group_based.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Write multiple iterations with non-consecutive indices (0, 5, 10) */
    for (int64_t iter_loop = 0; iter_loop < NUM_ITERATIONS; iter_loop++) {
        int64_t iter_idx = iter_loop * 5;

        /* Generate iteration-specific particle data */
        generate_particle_data(iter_idx, NUM_PARTICLES, &pg);

        result = pmd_open_iteration(series, iter_idx, &iter);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

        result = pmd_write_particle_group(iter, &pg);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

        result = pmd_close_iteration(iter);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    }

    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Cleanup */
    free(x);
    free(y);
    free(z);
    free(t);
    free(px);
    free(py);
    free(pz);
    free(weight);
    free(status);
    free(id);
}

/**
 * Test: Generate file-based OpenPMD files with multiple iterations
 * Files are stored in a subdirectory
 */
void test_generate_file_based_openpmd(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;
    particle_group pg;
    const int64_t NUM_PARTICLES = 10;
    const int64_t NUM_ITERATIONS = 3;
    const char *output_dir = TEST_OUTPUT_DIR "/file_based_example";

    /* Create subdirectory for file-based iterations */
    mkdir(output_dir, 0755);

    /* Allocate particle arrays */
    double *x = (double*)malloc(NUM_PARTICLES * sizeof(double));
    double *y = (double*)malloc(NUM_PARTICLES * sizeof(double));
    double *z = (double*)malloc(NUM_PARTICLES * sizeof(double));
    double *t = (double*)malloc(NUM_PARTICLES * sizeof(double));
    double *px = (double*)malloc(NUM_PARTICLES * sizeof(double));
    double *py = (double*)malloc(NUM_PARTICLES * sizeof(double));
    double *pz = (double*)malloc(NUM_PARTICLES * sizeof(double));
    double *weight = (double*)malloc(NUM_PARTICLES * sizeof(double));
    int64_t *status = (int64_t*)malloc(NUM_PARTICLES * sizeof(int64_t));
    int64_t *id = (int64_t*)malloc(NUM_PARTICLES * sizeof(int64_t));

    /* Setup particle group structure */
    memset(&pg, 0, sizeof(particle_group));
    pg.num_particles = NUM_PARTICLES;
    pg.species_type = "electron";
    pg.x = x;
    pg.y = y;
    pg.z = z;
    pg.t = t;
    pg.px = px;
    pg.py = py;
    pg.pz = pz;
    pg.weight = weight;
    pg.status = status;
    pg.id = id;

    /* Create file-based series with %T pattern in directory */
    result = pmd_open_series(TEST_OUTPUT_DIR "/file_based_example/data_%T.h5",
                              &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Write multiple iterations with non-consecutive indices (0, 5, 10) */
    for (int64_t iter_loop = 0; iter_loop < NUM_ITERATIONS; iter_loop++) {
        int64_t iter_idx = iter_loop * 5;

        /* Generate iteration-specific particle data */
        generate_particle_data(iter_idx, NUM_PARTICLES, &pg);

        result = pmd_open_iteration(series, iter_idx, &iter);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

        result = pmd_write_particle_group(iter, &pg);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

        result = pmd_close_iteration(iter);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    }

    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Cleanup */
    free(x);
    free(y);
    free(z);
    free(t);
    free(px);
    free(py);
    free(pz);
    free(weight);
    free(status);
    free(id);
}

int main(void) {
    /* Suppress HDF5 error messages during tests */
    H5Eset_auto2(H5E_DEFAULT, NULL, NULL);

    UNITY_BEGIN();

    RUN_TEST(test_generate_group_based_openpmd);
    RUN_TEST(test_generate_file_based_openpmd);

    return UNITY_END();
}
