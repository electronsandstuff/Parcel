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

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_create_group_based_series);
    RUN_TEST(test_create_file_based_series);
    RUN_TEST(test_open_existing_group_based_rdwr);
    RUN_TEST(test_open_existing_file_based_rdwr);
    RUN_TEST(test_create_excl_fails_if_exists);
    RUN_TEST(test_rdwr_fails_if_not_exists);

    return UNITY_END();
}
