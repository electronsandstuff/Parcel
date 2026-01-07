/* Test C++ API compatibility with custom user-defined particle group */

#include <vector>
#include <iostream>
#include <cmath>

extern "C" {
    #include "../deps/Unity-2.6.1/unity.h"
}

#define PARCEL_IMPLEMENTATION
#include "../parcel.h"

/* Custom C++ particle group using std::vector for storage */
struct cpp_particle_group {
    int64_t num_particles;
    std::string species_type;

    /* Position vectors */
    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> z;
    std::vector<double> t;

    /* Position offset vectors */
    std::vector<double> x_offset;
    std::vector<double> y_offset;
    std::vector<double> z_offset;

    /* Momentum vectors */
    std::vector<double> px;
    std::vector<double> py;
    std::vector<double> pz;

    /* Optional per-particle data */
    std::vector<double> weight;
    std::vector<int64_t> status;
    std::vector<int64_t> id;

    /* Constructor - allocate all arrays */
    explicit cpp_particle_group(int64_t count) : num_particles(count) {
        x.resize(count);
        y.resize(count);
        z.resize(count);
        t.resize(count);
        x_offset.resize(count);
        y_offset.resize(count);
        z_offset.resize(count);
        px.resize(count);
        py.resize(count);
        pz.resize(count);
        weight.resize(count);
        status.resize(count);
        id.resize(count);
    }

    /* Convert to C pmd_particle_group for passing to library functions */
    pmd_particle_group to_c_struct() {
        pmd_particle_group pg;
        pg.num_particles = num_particles;
        pg.species_type = nullptr;

        /* Get raw pointers from vectors */
        pg.x = x.data();
        pg.y = y.data();
        pg.z = z.data();
        pg.t = t.data();
        pg.x_offset = x_offset.data();
        pg.y_offset = y_offset.data();
        pg.z_offset = z_offset.data();
        pg.px = px.data();
        pg.py = py.data();
        pg.pz = pz.data();
        pg.weight = weight.data();
        pg.status = status.data();
        pg.id = id.data();

        return pg;
    }
};

void setUp() {
    /* This is run before each test */
}

void tearDown() {
    /* This is run after each test */
}

/* Test basic C++ API usage with std::vector-backed storage */
void test_cpp_vector_backed_particle_group() {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;
    int64_t num_particles;

    /* Open series */
    result = pmd_open_series("tests/data/valid_dataset_records.h5", &series, PMD_RDONLY);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(series);

    /* Get iterations */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_TRUE(num_iterations > 0);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(iter);

    /* Get number of particles */
    result = pmd_get_num_particles(iter, "electron", &num_particles);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_TRUE(num_particles > 0);

    /* Create C++ particle group with std::vector storage */
    cpp_particle_group cpp_pg(num_particles);
    cpp_pg.species_type = "electron";

    /* Convert to C struct for reading */
    pmd_particle_group c_pg = cpp_pg.to_c_struct();

    /* Read particle data into our C++ vectors via the C struct */
    result = pmd_read_particle_group(iter, "electron", &c_pg, nullptr);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify data was read correctly into std::vectors */
    TEST_ASSERT_TRUE(cpp_pg.num_particles > 0);

    /* Check first particle position - verify vectors were populated */
    TEST_ASSERT_FALSE(std::isnan(cpp_pg.x[0]));
    TEST_ASSERT_FALSE(std::isnan(cpp_pg.y[0]));
    TEST_ASSERT_FALSE(std::isnan(cpp_pg.z[0]));

    /* Check ID is valid */
    TEST_ASSERT_TRUE(cpp_pg.id[0] >= 0);

    /* Verify we can iterate over vectors with C++ range-based for */
    for (const auto& x_val : cpp_pg.x) {
        TEST_ASSERT_FALSE(std::isnan(x_val));
    }

    /* Clean up */
    pmd_close_iteration(iter);
    pmd_close_series(series);

    /* Note: std::vectors automatically clean up their memory */
}

/* Test selective reading with some NULL pointers */
void test_cpp_selective_reading() {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;
    int64_t num_particles;

    /* Open series */
    result = pmd_open_series("tests/data/valid_dataset_records.h5", &series, PMD_RDONLY);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get iterations */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get number of particles */
    result = pmd_get_num_particles(iter, "electron", &num_particles);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Create vectors for ONLY position and id */
    std::vector<double> x(num_particles);
    std::vector<double> y(num_particles);
    std::vector<double> z(num_particles);
    std::vector<double> x_offset(num_particles);
    std::vector<double> y_offset(num_particles);
    std::vector<double> z_offset(num_particles);
    std::vector<int64_t> id(num_particles);

    /* Create C struct with selective pointers - initialize to zero */
    pmd_particle_group pg = {};
    pg.num_particles = num_particles;
    pg.species_type = nullptr;

    /* Only provide position, offset, and id arrays */
    pg.x = x.data();
    pg.y = y.data();
    pg.z = z.data();
    pg.x_offset = x_offset.data();
    pg.y_offset = y_offset.data();
    pg.z_offset = z_offset.data();
    pg.id = id.data();

    /* Set others to nullptr - they won't be read */
    pg.t = nullptr;
    pg.px = nullptr;
    pg.py = nullptr;
    pg.pz = nullptr;
    pg.weight = nullptr;
    pg.status = nullptr;

    /* Read - should only populate x, y, z, id */
    result = pmd_read_particle_group(iter, "electron", &pg, nullptr);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify position data was read */
    TEST_ASSERT_FALSE(std::isnan(x[0]));
    TEST_ASSERT_FALSE(std::isnan(y[0]));
    TEST_ASSERT_FALSE(std::isnan(z[0]));
    TEST_ASSERT_TRUE(id[0] >= 0);

    /* Clean up */
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Main test runner */
int main() {
    // Turn off logging for tests
    pmd_set_log_level(PMD_LOG_NONE);

    UNITY_BEGIN();
    RUN_TEST(test_cpp_vector_backed_particle_group);
    RUN_TEST(test_cpp_selective_reading);

    /* Clean up HDF5 library internal resources */
    H5close();

    return UNITY_END();
}
