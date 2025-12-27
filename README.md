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

## License
This code was written by Christopher M. Pierce and is released under the BSD 3-Clause License.
