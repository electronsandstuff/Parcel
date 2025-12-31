# Parcel

**Par**cel: a package of **par**ticle and mesh data.
A single header C implementation of the [OpenPMD standard](https://github.com/openPMD/openPMD-standard) with the BeamPhysics extension.


## Usage

This project uses the same convention as the [stb libraries](https://github.com/nothings/stb).
Simply copy the header file `parcel.h` into your project and include in everything that requires its interface.
Then, in one and only one of your `.c` files, use the following line to include the library's implementations.
```c
#define PARCEL_IMPLEMENTATION
#include "../parcel.h"
```

## Implementation Notes

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

### Cache Invalidation

The library caches certain values for performance:

1. **Iteration list cache** (`series->num_iterations` and `series->iteration_indices`): Stores the list of available iterations
2. **Series metadata cache** (`series->_author`, `series->_software`, etc.): Stores metadata attributes in the series handle

These caches must be invalidated when the underlying data changes:

- When creating a new iteration with `pmd_open_iteration()`, the iteration list cache must be invalidated:
  ```c
  /* Invalidate iteration cache since we just created a new iteration */
  series->num_iterations = -1;
  free(series->iteration_indices);
  series->iteration_indices = NULL;
  ```

- When setting series metadata (e.g., `pmd_set_author()`), the new value must be:
  1. Stored in the series handle cache
  2. Written to all existing iteration files for FILE_BASED series
  3. Written to the series file for GROUP_BASED series

Failing to invalidate caches can cause stale data to be returned and metadata updates to not propagate to all files.

## License
This code was written by Christopher M. Pierce and is released under the BSD 3-Clause License.
