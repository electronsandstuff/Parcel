from pmd_beamphysics import ParticleGroup
import numpy as np
from pathlib import Path

def make_attr_count(fname: str, num_particles: int):
    """
    Create a BeamPhysics file where each required array is set to the same value, but
    increasing for each attributes. Ie x->0, y->1, z->2, ...
    """
    # Create the particle group
    pg = ParticleGroup(data={
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
    })

    # Write it
    pg.write(fname)

    # Write message
    print(f"make_attr_count: Wrote {num_particles} to {fname}")


if __name__ == "__main__":
    # Get the path to the test data location
    test_data_dir = Path(__file__).parent.parent / "tests" / "data" 
 
    # Generate test file with 32 particles
    make_attr_count(str(test_data_dir / "attr_count_32.h5"), 32)
