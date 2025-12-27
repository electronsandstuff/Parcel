"""
This tool generates a variety of test files for confirming the OpenPMD-BeamPhysics API
works correctly. When valid files are generated, the contents of each attribute is
`sum(ord(x) for x in name)` for the attributes name. When each particle in the particle group
has a different value, the first particle has a value of `sum(ord(x) for x in name)` and each
following particle has a value incremented by one. This allows tests to confirm the data was
loaded correctly.

The following files should be created in `tests/data` within this repo.

## HDF5 Structure Issues
- File that is missing the group `particles` in base
- `particles` is a dataset instead of a group
- File is valid HDF5 but completely empty (no groups at all)
- `/particles` contains both groups AND datasets

## Species Group Issues
- File that has `particles` in base, but no particle species contained inside
- Species inside `particles` is a dataset, not a group
- Empty string for species name
- Species name with special characters (/, null bytes, unicode)
- Very long species name (>255 chars) - could overflow buffers

## Missing/Invalid Attributes
- Missing `numParticles` attribute
- `numParticles` has wrong type (e.g., float instead of int64)
- `numParticles` is 0 (edge case)
- `numParticles` is negative
- `numParticles` is 1 (single particle edge case)
- Missing `speciesType` attribute
- `speciesType` has wrong type (e.g., numeric instead of string)
- `position/` is a dataset instead of a group
- `momentum/` group exists but is empty

## Array Size Mismatches
- Dataset size larger than `numParticles`
- Dataset size smaller than `numParticles`
- Dataset size is 0 when `numParticles` > 0
- Different sizes for position/x, position/y, position/z
- Datasets for optional fields have different lengths than position
- Datasets have inconsistent lengths inside particle group

## Invalid Constant Records
- Constant record (group) exists but missing `value` attribute
- Constant record with wrong-typed `value` attribute
- Both dataset AND constant record exist for same field
- Constant record with `value` that is an array instead of scalar

## Valid Files
- Particle Group where array attribute has constant value (ie written as attribute in group)
- Valid file where each attribute in particle group has unique value
  - Constant value across all particles (eg set as attribute of group)
  - Different (incrementing) values for each particle (eg using datasets to store the values)
"""

from pmd_beamphysics import ParticleGroup
import numpy as np
from pathlib import Path


def make_attr_count(fname: str, num_particles: int):
    """
    Create a BeamPhysics file where each required array is set to the same value, but
    increasing for each attributes. Ie x->0, y->1, z->2, ...
    """
    # Create the particle group
    pg = ParticleGroup(
        data={
            "x": np.full(num_particles, 0.0),
            "y": np.full(num_particles, 1.0),
            "z": np.full(num_particles, 2.0),
            "t": np.full(num_particles, 3.0),
            "px": np.full(num_particles, 4.0),
            "py": np.full(num_particles, 5.0),
            "pz": np.full(num_particles, 6.0),
            "weight": np.full(num_particles, 7.0),
            "id": np.full(num_particles, 8, dtype=int),
            "status": np.full(num_particles, 9, dtype=int),
            "species": "electron",
        }
    )

    # Write it
    pg.write(fname)

    # Write message
    print(f"make_attr_count: Wrote {num_particles} to {fname}")


if __name__ == "__main__":
    # Get the path to the test data location
    test_data_dir = Path(__file__).parent.parent / "tests" / "data"

    # Generate test file with 32 particles
    make_attr_count(str(test_data_dir / "attr_count_32.h5"), 32)
