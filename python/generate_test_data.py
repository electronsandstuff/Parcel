"""
This tool generates a variety of test files for confirming the OpenPMD-BeamPhysics API
works correctly. When valid files are generated, the contents of each attribute is
`sum(ord(x) for x in name)` for the attributes name. When each particle in the particle group
has a different value, the first particle has a value of `sum(ord(x) for x in name)` and each
following particle has a value incremented by one. This allows tests to confirm the data was
loaded correctly.

 The following files should be created in `tests/data` within this repo.
- File that is missing the group `particles` in base
- `particles` is a dataset instead of a group
- File that has `particles` in base, but no particle species contained inside
- Species inside `particles` is a dataset, not a group
- Dataset has wrong type for position, momentum
- Datasets length for attributes is less than `num_particles` (ie to confirm it is safely handled)
- Datasets have inconsistent lengths inside particle group
- Particle Group where array attribute has constant value (ie it is written as attribute in group)
- Particle Group where array attribute has constant value (ie it is written as attribute in group)
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
