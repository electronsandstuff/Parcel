/**
 * Tests for OpenPMD writer functionality
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
    #define rmdir(path) _rmdir(path)
#else
    #include <unistd.h>
    #include <dirent.h>
#endif

#define TEST_TEMP_DIR "tests/temp_writer"

/* Helper function to copy a file */
static int copy_file(const char *src, const char *dst) {
    FILE *src_file = fopen(src, "rb");
    FILE *dst_file;
    char buffer[4096];
    size_t bytes;

    if (!src_file) return -1;

    dst_file = fopen(dst, "wb");
    if (!dst_file) {
        fclose(src_file);
        return -1;
    }

    while ((bytes = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        fwrite(buffer, 1, bytes, dst_file);
    }

    fclose(src_file);
    fclose(dst_file);
    return 0;
}

/* Helper function to recursively remove directory */
static void remove_directory(const char *path) {
#ifdef _WIN32
    WIN32_FIND_DATAA find_data;
    HANDLE handle;
    char search_path[512];
    char file_path[512];

    snprintf(search_path, sizeof(search_path), "%s\\*", path);
    handle = FindFirstFileA(search_path, &find_data);

    if (handle != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0) {
                snprintf(file_path, sizeof(file_path), "%s\\%s", path, find_data.cFileName);
                if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    remove_directory(file_path);
                } else {
                    DeleteFileA(file_path);
                }
            }
        } while (FindNextFileA(handle, &find_data));
        FindClose(handle);
    }
    RemoveDirectoryA(path);
#else
    DIR *dir = opendir(path);
    struct dirent *entry;
    char file_path[512];

    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                snprintf(file_path, sizeof(file_path), "%s/%s", path, entry->d_name);
                struct stat st;
                if (stat(file_path, &st) == 0) {
                    if (S_ISDIR(st.st_mode)) {
                        remove_directory(file_path);
                    } else {
                        unlink(file_path);
                    }
                }
            }
        }
        closedir(dir);
    }
    rmdir(path);
#endif
}

void setUp(void) {
    /* Create temporary directory for test files */
    mkdir(TEST_TEMP_DIR, 0755);
}

void tearDown(void) {
    /* Clean up temporary directory */
    remove_directory(TEST_TEMP_DIR);
}

/**
 * Test: Create new group-based series
 */
void test_create_group_based_series(void) {
    pmd_series *series;
    pmd_status result;

    /* Create new group-based series */
    result = pmd_open_series(TEST_TEMP_DIR "/test_group.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(series);

    /* Verify series handle properties */
    TEST_ASSERT_EQUAL_INT(PMD_TRUNC, series->access_mode);
    TEST_ASSERT_EQUAL_INT(PMD_GROUP_BASED, series->iteration_encoding);
    TEST_ASSERT_EQUAL_INT(2, series->openpmd_version_major);
    TEST_ASSERT_EQUAL_INT(0, series->openpmd_version_minor);
    TEST_ASSERT_EQUAL_INT(0, series->openpmd_version_revision);

    /* Verify file handle is open for group-based */
    TEST_ASSERT(series->file_id >= 0);

    /* Verify directory is NULL for group-based */
    TEST_ASSERT_NULL(series->directory);

    /* Close series */
    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify file was created */
    FILE *test = fopen(TEST_TEMP_DIR "/test_group.h5", "rb");
    TEST_ASSERT_NOT_NULL(test);
    fclose(test);
}

/**
 * Test: Create new file-based series
 */
void test_create_file_based_series(void) {
    pmd_series *series;
    pmd_status result;

    /* Create new file-based series with %T pattern */
    result = pmd_open_series(TEST_TEMP_DIR "/data_%T.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(series);

    /* Verify series handle properties */
    TEST_ASSERT_EQUAL_INT(PMD_TRUNC, series->access_mode);
    TEST_ASSERT_EQUAL_INT(PMD_FILE_BASED, series->iteration_encoding);
    TEST_ASSERT_EQUAL_INT(2, series->openpmd_version_major);
    TEST_ASSERT_EQUAL_INT(0, series->openpmd_version_minor);
    TEST_ASSERT_EQUAL_INT(0, series->openpmd_version_revision);

    /* Verify directory is set for file-based */
    TEST_ASSERT_NOT_NULL(series->directory);
    TEST_ASSERT_EQUAL_STRING(TEST_TEMP_DIR, series->directory);

    /* Verify file handle is not open (file-based) */
    TEST_ASSERT_EQUAL_INT(-1, series->file_id);

    /* Close series */
    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* No files should be created yet (no iterations added) */
    FILE *test = fopen(TEST_TEMP_DIR "/data_0.h5", "rb");
    TEST_ASSERT_NULL(test);
}

/**
 * Test: Open existing group-based file in write mode
 */
void test_open_existing_group_based_rdwr(void) {
    pmd_series *series;
    pmd_status result;

    /* First create a file */
    result = pmd_open_series(TEST_TEMP_DIR "/existing.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Now open it in RDWR mode */
    result = pmd_open_series(TEST_TEMP_DIR "/existing.h5", &series, PMD_RDWR);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(series);

    /* Verify handle properties */
    TEST_ASSERT_EQUAL_INT(PMD_RDWR, series->access_mode);
    TEST_ASSERT_EQUAL_INT(PMD_GROUP_BASED, series->iteration_encoding);
    TEST_ASSERT_EQUAL_INT(2, series->openpmd_version_major);

    /* File should be open */
    TEST_ASSERT(series->file_id >= 0);

    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
}

/**
 * Test: Open existing file-based series in write mode
 */
void test_open_existing_file_based_rdwr(void) {
    pmd_series *series;
    pmd_status result;

    /* Copy test files to temp directory */
    copy_file("tests/data/file_based_series/data_0.h5", TEST_TEMP_DIR "/data_0.h5");
    copy_file("tests/data/file_based_series/data_1.h5", TEST_TEMP_DIR "/data_1.h5");
    copy_file("tests/data/file_based_series/data_2.h5", TEST_TEMP_DIR "/data_2.h5");

    /* Open existing file-based series in write mode */
    result = pmd_open_series(TEST_TEMP_DIR "/data_%T.h5", &series, PMD_RDWR);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify handle properties */
    TEST_ASSERT_EQUAL_INT(PMD_RDWR, series->access_mode);
    TEST_ASSERT_EQUAL_INT(PMD_FILE_BASED, series->iteration_encoding);
    TEST_ASSERT_NOT_NULL(series->directory);
    TEST_ASSERT_EQUAL_STRING(TEST_TEMP_DIR, series->directory);
    TEST_ASSERT_EQUAL_INT(-1, series->file_id);

    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
}

/**
 * Test: Create with PMD_EXCL fails if file exists
 */
void test_create_excl_fails_if_exists(void) {
    pmd_series *series;
    pmd_status result;

    /* Create a file first */
    result = pmd_open_series(TEST_TEMP_DIR "/excl_test.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Try to create again with PMD_EXCL - should fail */
    result = pmd_open_series(TEST_TEMP_DIR "/excl_test.h5", &series, PMD_EXCL);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_HDF5, result);
}

/**
 * Test: RDWR mode fails if file doesn't exist
 */
void test_rdwr_fails_if_not_exists(void) {
    pmd_series *series;
    pmd_status result;

    /* Try to open non-existent file in RDWR mode */
    result = pmd_open_series(TEST_TEMP_DIR "/nonexistent.h5", &series, PMD_RDWR);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_NOT_FOUND, result);
}

/**
 * Test: Create iteration in group-based series
 */
void test_create_iteration_group_based(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;

    /* Create new group-based series */
    result = pmd_open_series(TEST_TEMP_DIR "/iter_test.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Create iteration 0 */
    result = pmd_open_iteration(series, 0, &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(iter);

    /* Verify iteration properties */
    TEST_ASSERT_EQUAL_INT64(0, iter->iteration_index);
    TEST_ASSERT(iter->iteration_group_id >= 0);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, iter->time);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, iter->dt);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, iter->time_unit_si);

    /* Close iteration */
    result = pmd_close_iteration(iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Close series */
    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
}

/**
 * Test: Create iteration in file-based series
 */
void test_create_iteration_file_based(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;

    /* Create new file-based series */
    result = pmd_open_series(TEST_TEMP_DIR "/fb_%T.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Create iteration 0 */
    result = pmd_open_iteration(series, 0, &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(iter);

    /* Verify iteration properties */
    TEST_ASSERT_EQUAL_INT64(0, iter->iteration_index);
    TEST_ASSERT(iter->file_id >= 0);
    TEST_ASSERT(iter->iteration_group_id >= 0);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, iter->time);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, iter->dt);

    /* Close iteration */
    result = pmd_close_iteration(iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify file was created */
    FILE *test = fopen(TEST_TEMP_DIR "/fb_0.h5", "rb");
    TEST_ASSERT_NOT_NULL(test);
    fclose(test);

    /* Close series */
    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
}

/**
 * Test: Create multiple iterations
 */
void test_create_multiple_iterations(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;

    /* Create new group-based series */
    result = pmd_open_series(TEST_TEMP_DIR "/multi_iter.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Create iterations 0, 1, 2 */
    for (int64_t i = 0; i < 3; i++) {
        result = pmd_open_iteration(series, i, &iter);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
        TEST_ASSERT_EQUAL_INT64(i, iter->iteration_index);

        result = pmd_close_iteration(iter);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    }

    /* Close series */
    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Reopen in read mode and verify iterations exist */
    result = pmd_open_series(TEST_TEMP_DIR "/multi_iter.h5", &series, PMD_RDONLY);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    for (int64_t i = 0; i < 3; i++) {
        result = pmd_open_iteration(series, i, &iter);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

        result = pmd_close_iteration(iter);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    }

    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
}

/**
 * Test: Write particle group with minimal fields (position only)
 */
void test_write_particle_group_minimal(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;
    particle_group pg;
    const int64_t num_particles = 10;

    /* Allocate position arrays */
    double *x = (double*)malloc(num_particles * sizeof(double));
    double *y = (double*)malloc(num_particles * sizeof(double));
    double *z = (double*)malloc(num_particles * sizeof(double));

    /* Initialize with test data */
    for (int64_t i = 0; i < num_particles; i++) {
        x[i] = (double)i * 0.001;
        y[i] = (double)i * 0.002;
        z[i] = (double)i * 0.003;
    }

    /* Setup particle group with minimal fields */
    memset(&pg, 0, sizeof(particle_group));
    pg.num_particles = num_particles;
    pg.species_type = "electron";
    pg.x = x;
    pg.y = y;
    pg.z = z;
    /* All other fields NULL */

    /* Create series and iteration */
    result = pmd_open_series(TEST_TEMP_DIR "/minimal_pg.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_open_iteration(series, 0, &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Write particle group */
    result = pmd_write_particle_group(iter, &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Close */
    result = pmd_close_iteration(iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Clean up */
    free(x);
    free(y);
    free(z);
}

/**
 * Test: Write particle group with all fields
 */
void test_write_particle_group_complete(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;
    particle_group pg;
    const int64_t num_particles = 5;

    /* Allocate all arrays */
    double *x = (double*)malloc(num_particles * sizeof(double));
    double *y = (double*)malloc(num_particles * sizeof(double));
    double *z = (double*)malloc(num_particles * sizeof(double));
    double *t = (double*)malloc(num_particles * sizeof(double));
    double *px = (double*)malloc(num_particles * sizeof(double));
    double *py = (double*)malloc(num_particles * sizeof(double));
    double *pz = (double*)malloc(num_particles * sizeof(double));
    double *weight = (double*)malloc(num_particles * sizeof(double));
    int64_t *status = (int64_t*)malloc(num_particles * sizeof(int64_t));
    int64_t *id = (int64_t*)malloc(num_particles * sizeof(int64_t));

    /* Initialize with test data */
    for (int64_t i = 0; i < num_particles; i++) {
        x[i] = (double)i * 0.001;
        y[i] = (double)i * 0.002;
        z[i] = (double)i * 0.003;
        t[i] = (double)i * 1e-9;
        px[i] = (double)i * 1e6;  /* eV/c */
        py[i] = (double)i * 2e6;
        pz[i] = (double)i * 3e6;
        weight[i] = 1e10;
        status[i] = 1;
        id[i] = i + 1;
    }

    /* Setup particle group with all fields */
    memset(&pg, 0, sizeof(particle_group));
    pg.num_particles = num_particles;
    pg.species_type = "proton";
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

    /* Create series and iteration */
    result = pmd_open_series(TEST_TEMP_DIR "/complete_pg.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_open_iteration(series, 0, &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Write particle group */
    result = pmd_write_particle_group(iter, &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Close */
    result = pmd_close_iteration(iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Clean up */
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
 * Test: Write and read back particle group (round-trip)
 */
void test_write_and_read_particle_group(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;
    particle_group write_pg;
    particle_group *read_pg = NULL;
    const int64_t num_particles = 3;

    /* Allocate write arrays */
    double *write_x = (double*)malloc(num_particles * sizeof(double));
    double *write_y = (double*)malloc(num_particles * sizeof(double));
    double *write_z = (double*)malloc(num_particles * sizeof(double));
    double *write_px = (double*)malloc(num_particles * sizeof(double));
    double *write_py = (double*)malloc(num_particles * sizeof(double));
    double *write_pz = (double*)malloc(num_particles * sizeof(double));
    int64_t *write_id = (int64_t*)malloc(num_particles * sizeof(int64_t));

    /* Initialize with known test values */
    for (int64_t i = 0; i < num_particles; i++) {
        write_x[i] = 0.1 + (double)i * 0.01;
        write_y[i] = 0.2 + (double)i * 0.02;
        write_z[i] = 0.3 + (double)i * 0.03;
        write_px[i] = 1e6 + (double)i * 1e5;
        write_py[i] = 2e6 + (double)i * 2e5;
        write_pz[i] = 3e6 + (double)i * 3e5;
        write_id[i] = 100 + i;
    }

    /* Setup write particle group */
    memset(&write_pg, 0, sizeof(particle_group));
    write_pg.num_particles = num_particles;
    write_pg.species_type = "electron";
    write_pg.x = write_x;
    write_pg.y = write_y;
    write_pg.z = write_z;
    write_pg.px = write_px;
    write_pg.py = write_py;
    write_pg.pz = write_pz;
    write_pg.id = write_id;

    /* Write particle group */
    result = pmd_open_series(TEST_TEMP_DIR "/roundtrip.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_open_iteration(series, 0, &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_write_particle_group(iter, &write_pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_close_iteration(iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Read back particle group */
    result = pmd_open_series(TEST_TEMP_DIR "/roundtrip.h5", &series, PMD_RDONLY);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_open_iteration(series, 0, &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate read particle group */
    result = pmd_allocate_particle_group(iter, "electron", &read_pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_read_particle_group(iter, "electron", read_pg, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify data matches */
    TEST_ASSERT_EQUAL_INT64(num_particles, read_pg->num_particles);

    for (int64_t i = 0; i < num_particles; i++) {
        TEST_ASSERT_DOUBLE_WITHIN(1e-10, write_x[i], read_pg->x[i]);
        TEST_ASSERT_DOUBLE_WITHIN(1e-10, write_y[i], read_pg->y[i]);
        TEST_ASSERT_DOUBLE_WITHIN(1e-10, write_z[i], read_pg->z[i]);
        TEST_ASSERT_DOUBLE_WITHIN(1e-3, write_px[i], read_pg->px[i]);  /* Momentum may have rounding */
        TEST_ASSERT_DOUBLE_WITHIN(1e-3, write_py[i], read_pg->py[i]);
        TEST_ASSERT_DOUBLE_WITHIN(1e-3, write_pz[i], read_pg->pz[i]);
        TEST_ASSERT_EQUAL_INT64(write_id[i], read_pg->id[i]);
    }

    /* Clean up */
    pmd_free_particle_group(read_pg);
    result = pmd_close_iteration(iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    free(write_x);
    free(write_y);
    free(write_z);
    free(write_px);
    free(write_py);
    free(write_pz);
    free(write_id);
}

/**
 * Test: Particle group write error cases
 */
void test_write_particle_group_errors(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;
    particle_group pg;
    const int64_t num_particles = 2;

    double *x = (double*)malloc(num_particles * sizeof(double));
    double *y = (double*)malloc(num_particles * sizeof(double));
    double *z = (double*)malloc(num_particles * sizeof(double));

    /* Initialize */
    for (int64_t i = 0; i < num_particles; i++) {
        x[i] = (double)i;
        y[i] = (double)i;
        z[i] = (double)i;
    }

    /* Test: NULL species_type */
    memset(&pg, 0, sizeof(particle_group));
    pg.num_particles = num_particles;
    pg.species_type = NULL;  /* Missing */
    pg.x = x;
    pg.y = y;
    pg.z = z;

    result = pmd_open_series(TEST_TEMP_DIR "/error_test.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    result = pmd_open_iteration(series, 0, &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_write_particle_group(iter, &pg);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_NULL_POINTER, result);  /* Should fail */

    /* Test: NULL position field */
    pg.species_type = "electron";
    pg.x = NULL;  /* Missing required field */

    result = pmd_write_particle_group(iter, &pg);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_NULL_POINTER, result);  /* Should fail */

    /* Restore x */
    pg.x = x;

    /* Test: Zero particles */
    pg.num_particles = 0;
    result = pmd_write_particle_group(iter, &pg);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR, result);  /* Should fail */

    result = pmd_close_iteration(iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Test: Write in read-only mode */
    result = pmd_open_series(TEST_TEMP_DIR "/error_test.h5", &series, PMD_RDONLY);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    result = pmd_open_iteration(series, 0, &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    pg.num_particles = num_particles;
    result = pmd_write_particle_group(iter, &pg);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR, result);  /* Should fail - read-only */

    result = pmd_close_iteration(iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Clean up */
    free(x);
    free(y);
    free(z);
}

int main(void) {
    /* Suppress HDF5 error messages during tests */
    H5Eset_auto2(H5E_DEFAULT, NULL, NULL);

    UNITY_BEGIN();

    RUN_TEST(test_create_group_based_series);
    RUN_TEST(test_create_file_based_series);
    RUN_TEST(test_open_existing_group_based_rdwr);
    RUN_TEST(test_open_existing_file_based_rdwr);
    RUN_TEST(test_create_excl_fails_if_exists);
    RUN_TEST(test_rdwr_fails_if_not_exists);
    RUN_TEST(test_create_iteration_group_based);
    RUN_TEST(test_create_iteration_file_based);
    RUN_TEST(test_create_multiple_iterations);
    RUN_TEST(test_write_particle_group_minimal);
    RUN_TEST(test_write_particle_group_complete);
    RUN_TEST(test_write_and_read_particle_group);
    RUN_TEST(test_write_particle_group_errors);

    return UNITY_END();
}
