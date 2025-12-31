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
 * Test: Cannot write new iterations in read-only mode
 */
void test_cannot_write_iteration_readonly(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;

    /* Create a file first */
    result = pmd_open_series(TEST_TEMP_DIR "/readonly_test.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open in read-only mode */
    result = pmd_open_series(TEST_TEMP_DIR "/readonly_test.h5", &series, PMD_RDONLY);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(PMD_RDONLY, series->access_mode);

    /* Try to create an iteration - should fail */
    result = pmd_open_iteration(series, 0, &iter);
    TEST_ASSERT_NOT_EQUAL(PMD_SUCCESS, result);

    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
}

/**
 * Test: Write non-consecutive iterations in group-based mode
 */
void test_write_nonconsecutive_iterations_group_based(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Create group-based series */
    result = pmd_open_series(TEST_TEMP_DIR "/nonconsec_group.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Create iterations 0, 5, 10 (non-consecutive) */
    int64_t test_iterations[] = {0, 5, 10};
    for (int i = 0; i < 3; i++) {
        result = pmd_open_iteration(series, test_iterations[i], &iter);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
        TEST_ASSERT_EQUAL_INT64(test_iterations[i], iter->iteration_index);

        result = pmd_close_iteration(iter);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    }

    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Reopen and verify all iterations exist */
    result = pmd_open_series(TEST_TEMP_DIR "/nonconsec_group.h5", &series, PMD_RDONLY);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(3, num_iterations);

    /* Verify iteration numbers */
    TEST_ASSERT_EQUAL_INT64(0, iterations[0]);
    TEST_ASSERT_EQUAL_INT64(5, iterations[1]);
    TEST_ASSERT_EQUAL_INT64(10, iterations[2]);

    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
}

/**
 * Test: Write non-consecutive iterations in file-based mode
 */
void test_write_nonconsecutive_iterations_file_based(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Create file-based series */
    result = pmd_open_series(TEST_TEMP_DIR "/nonconsec_%T.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Create iterations 1, 3, 7 (non-consecutive) */
    int64_t test_iterations[] = {1, 3, 7};
    for (int i = 0; i < 3; i++) {
        result = pmd_open_iteration(series, test_iterations[i], &iter);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
        TEST_ASSERT_EQUAL_INT64(test_iterations[i], iter->iteration_index);

        result = pmd_close_iteration(iter);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    }

    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify individual files were created */
    FILE *test1 = fopen(TEST_TEMP_DIR "/nonconsec_1.h5", "rb");
    TEST_ASSERT_NOT_NULL(test1);
    fclose(test1);

    FILE *test3 = fopen(TEST_TEMP_DIR "/nonconsec_3.h5", "rb");
    TEST_ASSERT_NOT_NULL(test3);
    fclose(test3);

    FILE *test7 = fopen(TEST_TEMP_DIR "/nonconsec_7.h5", "rb");
    TEST_ASSERT_NOT_NULL(test7);
    fclose(test7);

    /* Reopen with pattern and verify iterations */
    result = pmd_open_series(TEST_TEMP_DIR "/nonconsec_%T.h5", &series, PMD_RDONLY);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(3, num_iterations);

    /* Verify iteration numbers */
    TEST_ASSERT_EQUAL_INT64(1, iterations[0]);
    TEST_ASSERT_EQUAL_INT64(3, iterations[1]);
    TEST_ASSERT_EQUAL_INT64(7, iterations[2]);

    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
}

/**
 * Test: Writer fails if parent directory does not exist
 */
void test_write_fails_no_parent_directory(void) {
    pmd_series *series;
    pmd_status result;

    /* Try to create file in non-existent directory */
    result = pmd_open_series("nonexistent_dir/test.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_NOT_FOUND, result);

    /* Try with file-based pattern */
    result = pmd_open_series("nonexistent_dir/data_%T.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_NOT_FOUND, result);
}

/**
 * Test: Invalid patterns with ambiguous %T fail
 */
void test_invalid_pattern_ambiguous(void) {
    pmd_series *series;
    pmd_status result;

    /* Test: Ambiguous %T (adjacent with no separator) - should fail */
    result = pmd_open_series(TEST_TEMP_DIR "/data_%T%T.h5", &series, PMD_TRUNC);
    TEST_ASSERT_NOT_EQUAL(PMD_SUCCESS, result);

    /* Also test in RDWR mode */
    result = pmd_open_series(TEST_TEMP_DIR "/data_%T%T.h5", &series, PMD_RDWR);
    TEST_ASSERT_NOT_EQUAL(PMD_SUCCESS, result);

    /* Also test in EXCL mode */
    result = pmd_open_series(TEST_TEMP_DIR "/data_%T%T.h5", &series, PMD_EXCL);
    TEST_ASSERT_NOT_EQUAL(PMD_SUCCESS, result);
}

/**
 * Test: Various valid file-based iteration patterns
 * Tests multiple pattern formats in a parameterized style
 */
void test_valid_filebased_patterns(void) {
    typedef struct {
        const char *pattern;
        const char *expected_file;
        const char *description;
    } pattern_test_case;

    pattern_test_case test_cases[] = {
        {
            TEST_TEMP_DIR "/data_%T_suffix.h5",
            TEST_TEMP_DIR "/data_7_suffix.h5",
            "Suffix after %T"
        },
        {
            TEST_TEMP_DIR "/data_%T/data.h5",
            TEST_TEMP_DIR "/data_7/data.h5",
            "Directory after %T"
        },
        {
            TEST_TEMP_DIR "/data_%T_%T.h5",
            TEST_TEMP_DIR "/data_7_7.h5",
            "Multiple %T in filename"
        },
        {
            TEST_TEMP_DIR "/data_%T/file_%T.h5",
            TEST_TEMP_DIR "/data_7/file_7.h5",
            "Multiple %T across directory boundary"
        },
        {
            TEST_TEMP_DIR "/iter_%T/step_%T/data_%T.h5",
            TEST_TEMP_DIR "/iter_7/step_7/data_7.h5",
            "Multiple nested directories with %T"
        }
    };

    int num_cases = sizeof(test_cases) / sizeof(test_cases[0]);

    for (int i = 0; i < num_cases; i++) {
        pmd_series *series;
        pmd_iteration *iter;
        pmd_status result;

        /* Create series with pattern */
        result = pmd_open_series(test_cases[i].pattern, &series, PMD_TRUNC);
        TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, test_cases[i].description);
        TEST_ASSERT_NOT_NULL_MESSAGE(series, test_cases[i].description);
        TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_FILE_BASED, series->iteration_encoding, test_cases[i].description);

        /* Create iteration 7 */
        result = pmd_open_iteration(series, 7, &iter);
        TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, test_cases[i].description);

        result = pmd_close_iteration(iter);
        TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, test_cases[i].description);

        result = pmd_close_series(series);
        TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, test_cases[i].description);

        /* Verify file was created at expected location */
        FILE *test = fopen(test_cases[i].expected_file, "rb");
        TEST_ASSERT_NOT_NULL_MESSAGE(test, test_cases[i].description);
        if (test) fclose(test);

        /* Reopen series and verify we can read it back */
        result = pmd_open_series(test_cases[i].pattern, &series, PMD_RDONLY);
        TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, test_cases[i].description);

        result = pmd_open_iteration(series, 7, &iter);
        TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, test_cases[i].description);

        result = pmd_close_iteration(iter);
        TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, test_cases[i].description);

        result = pmd_close_series(series);
        TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, test_cases[i].description);
    }
}

/**
 * Test: Verify all required OpenPMD attributes are written
 * Parameterized test for both file-based and group-based series
 */
void test_openpmd_required_attributes(void) {
    typedef struct {
        const char *pattern;
        const char *description;
        pmd_iteration_encoding expected_encoding;
    } test_case;

    test_case cases[] = {
        {TEST_TEMP_DIR "/attr_test_group.h5", "Group-based series", PMD_GROUP_BASED},
        {TEST_TEMP_DIR "/attr_test_%T.h5", "File-based series", PMD_FILE_BASED},
        {TEST_TEMP_DIR "/attr_test_%T_data_%T.h5", "File-based series; Complex Pattern", PMD_FILE_BASED},
        {TEST_TEMP_DIR "/attr_test_%T/data.h5", "File-based series; Directory", PMD_FILE_BASED}
    };

    for (int case_idx = 0; case_idx < 4; case_idx++) {
        pmd_series *series;
        pmd_iteration *iter;
        pmd_status result;
        particle_group pg;
        int64_t test_iterations[] = {0, 5, 10, 14, 23, 45, 90, 100, 1024, 10920};
        int num_test_iters = 10;
        const int64_t num_particles = 5;

        /* Allocate particle data */
        double *x = (double*)malloc(num_particles * sizeof(double));
        double *y = (double*)malloc(num_particles * sizeof(double));
        double *z = (double*)malloc(num_particles * sizeof(double));
        double *px = (double*)malloc(num_particles * sizeof(double));
        double *py = (double*)malloc(num_particles * sizeof(double));
        double *pz = (double*)malloc(num_particles * sizeof(double));

        for (int64_t i = 0; i < num_particles; i++) {
            x[i] = (double)i * 0.001;
            y[i] = (double)i * 0.002;
            z[i] = (double)i * 0.003;
            px[i] = (double)i * 1e6;
            py[i] = (double)i * 2e6;
            pz[i] = (double)i * 3e6;
        }

        /* Setup particle group */
        memset(&pg, 0, sizeof(particle_group));
        pg.num_particles = num_particles;
        pg.species_type = "electron";
        pg.x = x;
        pg.y = y;
        pg.z = z;
        pg.px = px;
        pg.py = py;
        pg.pz = pz;

        /* Create series and write non-consecutive iterations */
        result = pmd_open_series(cases[case_idx].pattern, &series, PMD_TRUNC);
        TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, cases[case_idx].description);

        for (int i = 0; i < num_test_iters; i++) {
            result = pmd_open_iteration(series, test_iterations[i], &iter);
            TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, cases[case_idx].description);

            result = pmd_write_particle_group(iter, &pg);
            TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, cases[case_idx].description);

            result = pmd_close_iteration(iter);
            TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, cases[case_idx].description);
        }

        result = pmd_close_series(series);
        TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, cases[case_idx].description);

        /* Now reopen and verify attributes */
        result = pmd_open_series(cases[case_idx].pattern, &series, PMD_RDONLY);
        TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, cases[case_idx].description);

        /* Check root-level attributes */
        hid_t file_id;
        if (cases[case_idx].expected_encoding == PMD_GROUP_BASED) {
            file_id = series->file_id;
        } else {
            /* For file-based, open first file to check root attributes */
            char *filename = replace_iteration(cases[case_idx].pattern, test_iterations[0]);
            file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
            free(filename);
        }
        TEST_ASSERT_MESSAGE(file_id >= 0, cases[case_idx].description);

        /* Required: openPMD version */
        TEST_ASSERT_MESSAGE(H5Aexists(file_id, "openPMD") > 0, cases[case_idx].description);

        /* Required: basePath */
        TEST_ASSERT_MESSAGE(H5Aexists(file_id, "basePath") > 0, cases[case_idx].description);

        /* Required: particlesPath (since we wrote particles) */
        TEST_ASSERT_MESSAGE(H5Aexists(file_id, "particlesPath") > 0, cases[case_idx].description);

        /* Required: iterationEncoding */
        TEST_ASSERT_MESSAGE(H5Aexists(file_id, "iterationEncoding") > 0, cases[case_idx].description);

        /* Required: iterationFormat */
        TEST_ASSERT_MESSAGE(H5Aexists(file_id, "iterationFormat") > 0, cases[case_idx].description);

        if (cases[case_idx].expected_encoding == PMD_FILE_BASED) {
            H5Fclose(file_id);
        }

        /* Check iteration-level attributes for each iteration */
        int64_t *iterations;
        int num_iterations;
        result = pmd_get_iterations(series, &iterations, &num_iterations);
        TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, cases[case_idx].description);
        TEST_ASSERT_EQUAL_INT_MESSAGE(num_test_iters, num_iterations, cases[case_idx].description);

        for (int i = 0; i < num_iterations; i++) {
            result = pmd_open_iteration(series, iterations[i], &iter);
            TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, cases[case_idx].description);

            /* Check basePath attributes: time, dt, timeUnitSI */
            hid_t iter_group = iter->iteration_group_id;
            TEST_ASSERT_MESSAGE(iter_group >= 0, cases[case_idx].description);

            TEST_ASSERT_MESSAGE(H5Aexists(iter_group, "time") > 0, cases[case_idx].description);
            TEST_ASSERT_MESSAGE(H5Aexists(iter_group, "dt") > 0, cases[case_idx].description);
            TEST_ASSERT_MESSAGE(H5Aexists(iter_group, "timeUnitSI") > 0, cases[case_idx].description);

            /* Check particle group attributes */
            hid_t particles_group = H5Gopen(iter_group, "particles", H5P_DEFAULT);
            TEST_ASSERT_MESSAGE(particles_group >= 0, cases[case_idx].description);

            hid_t species_group = H5Gopen(particles_group, "electron", H5P_DEFAULT);
            TEST_ASSERT_MESSAGE(species_group >= 0, cases[case_idx].description);

            /* For each record, check unitDimension and timeOffset */
            const char *records[] = {"position", "momentum"};
            const char *components[] = {"x", "y", "z"};

            for (int r = 0; r < 2; r++) {
                hid_t record_group = H5Gopen(species_group, records[r], H5P_DEFAULT);
                TEST_ASSERT_MESSAGE(record_group >= 0, cases[case_idx].description);

                /* Check record-level attributes: unitDimension, timeOffset */
                TEST_ASSERT_MESSAGE(H5Aexists(record_group, "unitDimension") > 0,
                                   cases[case_idx].description);
                TEST_ASSERT_MESSAGE(H5Aexists(record_group, "timeOffset") > 0,
                                   cases[case_idx].description);

                /* Check component-level unitSI */
                for (int c = 0; c < 3; c++) {
                    hid_t component = H5Dopen(record_group, components[c], H5P_DEFAULT);
                    TEST_ASSERT_MESSAGE(component >= 0, cases[case_idx].description);
                    TEST_ASSERT_MESSAGE(H5Aexists(component, "unitSI") > 0,
                                       cases[case_idx].description);
                    H5Dclose(component);
                }

                H5Gclose(record_group);
            }

            H5Gclose(species_group);
            H5Gclose(particles_group);

            result = pmd_close_iteration(iter);
            TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, cases[case_idx].description);
        }

        result = pmd_close_series(series);
        TEST_ASSERT_EQUAL_INT_MESSAGE(PMD_SUCCESS, result, cases[case_idx].description);

        /* Clean up */
        free(x);
        free(y);
        free(z);
        free(px);
        free(py);
        free(pz);
    }
}

/**
 * Test: File-based pattern fails if parent directory before %T doesn't exist
 * Parcel should only create directories it's responsible for (containing %T)
 */
void test_filebased_fails_parent_before_T_missing(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;

    /* Try to create with pattern where parent before %T doesn't exist */
    result = pmd_open_series("nonexistent_parent/data_%T.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_NOT_FOUND, result);

    /* Also test with nested pattern */
    result = pmd_open_series("nonexistent_parent/iter_%T/data.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_NOT_FOUND, result);
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

#ifdef _WIN32
/**
 * Test: Windows-style path works for creating GROUP_BASED series
 * Tests: Backslash path separators work on Windows for writing
 */
void test_windows_path_group_based_write(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;

    /* Create new group-based series with Windows path */
    result = pmd_open_series("tests\\temp_writer\\win_group.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(series);
    TEST_ASSERT_EQUAL_INT(PMD_GROUP_BASED, series->iteration_encoding);

    /* Create an iteration to verify full functionality */
    result = pmd_open_iteration(series, 0, &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_close_iteration(iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify file was created */
    FILE *test = fopen("tests/temp_writer/win_group.h5", "rb");
    TEST_ASSERT_NOT_NULL(test);
    fclose(test);
}

/**
 * Test: Windows-style path with pattern works for FILE_BASED series
 * Tests: Backslash path separators work with pattern-based creation on Windows
 */
void test_windows_path_file_based_pattern_write(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;

    /* Create new file-based series with Windows path and pattern */
    result = pmd_open_series("tests\\temp_writer\\win_data_%T.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(series);
    TEST_ASSERT_EQUAL_INT(PMD_FILE_BASED, series->iteration_encoding);

    /* Create iteration 0 - should create file */
    result = pmd_open_iteration(series, 0, &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_close_iteration(iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Create iteration 1 */
    result = pmd_open_iteration(series, 1, &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_close_iteration(iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify files were created */
    FILE *test0 = fopen("tests/temp_writer/win_data_0.h5", "rb");
    TEST_ASSERT_NOT_NULL(test0);
    fclose(test0);

    FILE *test1 = fopen("tests/temp_writer/win_data_1.h5", "rb");
    TEST_ASSERT_NOT_NULL(test1);
    fclose(test1);
}

/**
 * Test: Windows-style path works for opening existing series in RDWR mode
 * Tests: Backslash paths work when opening existing files for modification
 */
void test_windows_path_rdwr(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;

    /* First create a file with forward slashes */
    result = pmd_open_series(TEST_TEMP_DIR "/win_existing.h5", &series, PMD_TRUNC);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Now open it with backslashes in RDWR mode */
    result = pmd_open_series("tests\\temp_writer\\win_existing.h5", &series, PMD_RDWR);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(series);
    TEST_ASSERT_EQUAL_INT(PMD_RDWR, series->access_mode);

    /* Create an iteration to verify write functionality */
    result = pmd_open_iteration(series, 0, &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_close_iteration(iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_close_series(series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
}
#endif /* _WIN32 */

int main(void) {
    /* Suppress error messages during tests */
    H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
    pmd_set_log_level(PMD_LOG_NONE);
    
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
    RUN_TEST(test_cannot_write_iteration_readonly);
    RUN_TEST(test_write_nonconsecutive_iterations_group_based);
    RUN_TEST(test_write_nonconsecutive_iterations_file_based);
    RUN_TEST(test_write_fails_no_parent_directory);
    RUN_TEST(test_invalid_pattern_ambiguous);
    RUN_TEST(test_valid_filebased_patterns);
    RUN_TEST(test_filebased_fails_parent_before_T_missing);
    RUN_TEST(test_openpmd_required_attributes);
    RUN_TEST(test_write_particle_group_minimal);
    RUN_TEST(test_write_particle_group_complete);
    RUN_TEST(test_write_particle_group_errors);

#ifdef _WIN32
    /* Windows-specific path tests */
    RUN_TEST(test_windows_path_group_based_write);
    RUN_TEST(test_windows_path_file_based_pattern_write);
    RUN_TEST(test_windows_path_rdwr);
#endif

    return UNITY_END();
}
