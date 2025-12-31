/**
 * Tests for OpenPMD read/write round-trip functionality
 */

#include "../deps/Unity-2.6.1/unity.h"

#define PARCEL_IMPLEMENTATION
#include "../parcel.h"

#include "utils.h"

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

#define TEST_TEMP_DIR "tests/temp_read_write"

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
 * Helper: Write and read back particle group (round-trip)
 * Tests that data written can be read back correctly
 *
 * @param filename Path to the file to create (should include %T for file-based)
 */
static void test_write_and_read_particle_group_helper(const char *filename) {
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
    double *write_t = (double*)malloc(num_particles * sizeof(double));
    double *write_px = (double*)malloc(num_particles * sizeof(double));
    double *write_py = (double*)malloc(num_particles * sizeof(double));
    double *write_pz = (double*)malloc(num_particles * sizeof(double));
    double *write_weight = (double*)malloc(num_particles * sizeof(double));
    int64_t *write_status = (int64_t*)malloc(num_particles * sizeof(int64_t));
    int64_t *write_id = (int64_t*)malloc(num_particles * sizeof(int64_t));

    /* Initialize with known test values */
    for (int64_t i = 0; i < num_particles; i++) {
        write_x[i] = 0.1 + (double)i * 0.01;
        write_y[i] = 0.2 + (double)i * 0.02;
        write_z[i] = 0.3 + (double)i * 0.03;
        write_t[i] = 1e-9 * (double)i;
        write_px[i] = 1e6 + (double)i * 1e5;
        write_py[i] = 2e6 + (double)i * 2e5;
        write_pz[i] = 3e6 + (double)i * 3e5;
        write_weight[i] = 1e10 + (double)i * 1e9;
        write_status[i] = 1;
        write_id[i] = 100 + i;
    }

    /* Setup write particle group */
    memset(&write_pg, 0, sizeof(particle_group));
    write_pg.num_particles = num_particles;
    write_pg.species_type = "electron";
    write_pg.x = write_x;
    write_pg.y = write_y;
    write_pg.z = write_z;
    write_pg.t = write_t;
    write_pg.px = write_px;
    write_pg.py = write_py;
    write_pg.pz = write_pz;
    write_pg.weight = write_weight;
    write_pg.status = write_status;
    write_pg.id = write_id;

    /* Write particle group */
    result = pmd_open_series(filename, &series, PMD_TRUNC);
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
    result = pmd_open_series(filename, &series, PMD_RDONLY);
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
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(write_x[i], read_pg->x[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(write_y[i], read_pg->y[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(write_z[i], read_pg->z[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(write_t[i], read_pg->t[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(write_px[i], read_pg->px[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(write_py[i], read_pg->py[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(write_pz[i], read_pg->pz[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(write_weight[i], read_pg->weight[i]);
        TEST_ASSERT_EQUAL_INT64(write_status[i], read_pg->status[i]);
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
    free(write_t);
    free(write_px);
    free(write_py);
    free(write_pz);
    free(write_weight);
    free(write_status);
    free(write_id);
}

/**
 * Test: Write and read back particle group with group-based iteration encoding
 */
void test_write_and_read_particle_group_based(void) {
    test_write_and_read_particle_group_helper(TEST_TEMP_DIR "/roundtrip_group.h5");
}

/**
 * Test: Write and read back particle group with file-based iteration encoding
 */
void test_write_and_read_particle_group_file_based(void) {
    test_write_and_read_particle_group_helper(TEST_TEMP_DIR "/roundtrip_%T.h5");
}

int main(void) {
    /* Suppress HDF5 error messages during tests */
    H5Eset_auto2(H5E_DEFAULT, NULL, NULL);

    UNITY_BEGIN();

    RUN_TEST(test_write_and_read_particle_group_based);
    RUN_TEST(test_write_and_read_particle_group_file_based);

    return UNITY_END();
}
