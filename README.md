<img src="assets/parcel_openpmd_logo.png" alt="Parcel logo showing a cardboard box holding a beam of electrons" width="600" >

----

Read and write particle-tracking code and particle-in-cell code data in the [OpenPMD standard](https://github.com/openPMD/openPMD-standard).

Parcel is a single header C implementation of the OpenPMD standard with the BeamPhysics extension using [HDF5](https://www.hdfgroup.org/solutions/hdf5/) as its backend.
It is designed for simple integration with existing physics simulation codes.
Drop the single file `parcel.h` into your codebase and call it to allow your tool to interchange particle data with the many other codes that use the OpenPMD format.


## Dependencies

Parcel is C99 compliant and its only dependency is HDF5.

## Usage

This project uses the same convention as the [stb libraries](https://github.com/nothings/stb).
Simply copy the header file `parcel.h` into your project and include in everything that requires its interface.
Then, in one and only one of your `.c` files, use the following line to include the library's implementations.
```c
#define PARCEL_IMPLEMENTATION
#include "../parcel.h"
```

The following examples illustrate typical use-cases of reading and writing data to an OpenPMD file.

### Basic Read Example

```c
#define PARCEL_IMPLEMENTATION
#include "parcel.h"

int main(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status status;
    int64_t *iterations = NULL;
    int64_t num_iterations;

    /* Open an OpenPMD data series */
    /* Use '%T' in the filename for a file-based series and parcel will autodetect */
    /* files of the form data_1.h5, data_2.h5, data_3.h5, etc. You may also use */
    /* the path of a single file in the file-based series, or the path to a */
    /* group-based series. */
    status = pmd_open_series("data_%T.h5", &series, PMD_RDONLY);
    if (status != PMD_SUCCESS) return 1;

    /* List available iteration indices */
    status = pmd_list_iterations(series, &iterations, &num_iterations);
    if (status != PMD_SUCCESS) return 1;

    printf("Found %lld iterations\n", (long long)num_iterations);
    for (int64_t i = 0; i < num_iterations; i++) {
        printf("  Iteration: %lld\n", (long long)iterations[i]);
    }

    /* Open the first iteration */
    status = pmd_open_iteration(series, iterations[0], &iter);
    if (status != PMD_SUCCESS) return 1;

    /* List available particle species */
    char **species_names = NULL;
    int num_species = 0;
    status = pmd_list_species(iter, &species_names, &num_species);
    if (status != PMD_SUCCESS) return 1;

    printf("Found %d species:\n", num_species);
    for (int i = 0; i < num_species; i++) {
        int64_t particle_count;
        pmd_get_num_particles(iter, species_names[i], &particle_count);
        printf("  %s: %lld particles\n", species_names[i], (long long)particle_count);
    }

    /* Allocate and read electron particle data */
    /* If integrating with existing physics code, you can set each pointer */
    /* in pmd_particle_group to an array inside an existing beam struct/class */
    /* to read data directly into it without having to copy. Set any pointer */
    /* to NULL to ignore reading. */
    status = pmd_allocate_particle_group(iter, "electron", &pg);
    if (status != PMD_SUCCESS) return 1;

    status = pmd_read_particle_group(iter, "electron", pg);
    if (status != PMD_SUCCESS) return 1;

    /* Use particle data - positions are in pg->x, pg->y, pg->z */
    printf("Read %lld particles\n", (long long)pg->num_particles);
    for (int64_t i = 0; i < pg->num_particles && i < 5; i++) {
        printf("  Particle %lld: x=%.3e, px=%.3e eV/c\n",
               (long long)i, pg->x[i], pg->px[i]);
    }

    /* Clean up */
    for (int i = 0; i < num_species; i++) {
        free(species_names[i]);
    }
    free(species_names);
    free(iterations);
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
    return 0;
}
```

### Basic Write Example

```c
#define PARCEL_IMPLEMENTATION
#include "parcel.h"

int main(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup pg = {0};
    pmd_status status;
    const int64_t N = 1000;

    /* Create a new OpenPMD data series */
    /* Using '%T' in filename will open series in file-based iteration mode */
    /* and embed iteration index in filename. Ommit '%T' in name to open in */
    /* group-based iteration mode and store iterations within HDF5 file */
    status = pmd_open_series("output_%T.h5", &series, PMD_TRUNC);
    if (status != PMD_SUCCESS) return 1;

    /* Create and open iteration 0 for writing */
    status = pmd_open_iteration(series, 0, &iter);
    if (status != PMD_SUCCESS) return 1;

    /* Allocate and populate particle data */
    pg.num_particles = N;
    pg.x = (double *)malloc(N * sizeof(double));
    pg.y = (double *)malloc(N * sizeof(double));
    pg.z = (double *)malloc(N * sizeof(double));
    pg.px = (double *)malloc(N * sizeof(double));
    pg.py = (double *)malloc(N * sizeof(double));
    pg.pz = (double *)malloc(N * sizeof(double));
    pg.weight = (double *)malloc(N * sizeof(double));

    /* Fill with sample data */
    for (int64_t i = 0; i < N; i++) {
        pg.x[i] = 0.001 * i;
        pg.y[i] = 0.0;
        pg.z[i] = 0.0;
        pg.px[i] = 0.0;  /* momentum in eV/c */
        pg.py[i] = 0.0;
        pg.pz[i] = 1e6;
        pg.weight[i] = 1.0;
    }

    /* Write particle data */
    /* Instead of allocating arrays in pmd_particle_group, you may set pointers to */
    /* user-supplied arrays in existing code to write directly from them. Set any */
    /* pointer to NULL to ignore writing. */
    status = pmd_write_particle_group(iter, "electron", &pg);
    if (status != PMD_SUCCESS) return 1;

    /* Clean up */
    free(pg.x); free(pg.y); free(pg.z);
    free(pg.px); free(pg.py); free(pg.pz);
    free(pg.weight);
    pmd_close_iteration(iter);
    pmd_close_series(series);
    return 0;
}
```

## Compiling and Running Tests

The tests in this project are built using CMake and have additional dependencies beyond HDF5 in order to generate test files.
Follow these steps to build the tests and then run them.
1) Install required dependencies through conda environment and activate. This will download and install HDF5 and the required python dependencies.
```
conda env create -f environment.yml
conda activate parcel-dev
```
2) Use CMake to build the tests.
```
mkdir build
cd build
cmake ..
make
```
3) Run the tests. This should run all C tests and check generated test files with the [OpenPMD Validator](https://github.com/openPMD/openPMD-validator).
```
# From inside of build/
ctest
```
4) [Optional] If any errors occur, running the individual test binaries may reveal more information.
```
./tests/test_read
./tests/test_write
./tests/test_read_write
./tests/test_utilitis
./tests/test_generate_openpmd
```

### Optional CMake Arguments
The following options may be used in the CMake command to enable additional debug features.
 - `-DDEBUG=ON`: Enable debug flags (ie for use with valgrind).
 - `-DCLANG_TIDY=ON`: Run `clang-tidy` during builds and error out on issues.


## Name

**Par**cel: a package containing **par**ticle data.

## License
This code was written by Christopher M. Pierce and is released under the BSD 3-Clause License.

## Internal Implementation Notes

### File Handle Management

When implementing internal functions that need to access iteration files, always use `pmd_open_iteration()` instead of directly calling `H5Fopen()`. This ensures that:

- Multiple file handles to the same HDF5 file are never opened simultaneously (which could cause corruption)
- File handles are properly reused when an iteration is already open
- All file access goes through the proper tracking mechanisms

Example:
```c
// DON'T do this:
hid_t file_id = H5Fopen(path, H5F_ACC_RDWR, H5P_DEFAULT);
// ... use file_id ...
H5Fclose(file_id);

// DO this instead:
pmd_iteration *iter;
pmd_open_iteration(series, index, &iter);
// ... use iter->file_id ...
pmd_close_iteration(iter);
```
