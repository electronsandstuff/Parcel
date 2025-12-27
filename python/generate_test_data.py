"""
This tool generates a variety of test files for confirming the OpenPMD-BeamPhysics API
works correctly. When valid files are generated, the contents of each attribute is
`sum(ord(x) for x in name)` for the attributes name. When each particle in the particle group
has a different value, the first particle has a value of `sum(ord(x) for x in name)` and each
following particle has a value incremented by one. This allows tests to confirm the data was
loaded correctly.

The following files should be created in `tests/data` within this repo.

## OpenPMD Root-Level Metadata Issues
- Missing `openPMD` attribute at root level
- `openPMD` attribute has wrong version format (e.g., "1.1" instead of "1.1.0")
- `openPMD` attribute has unsupported version
- Missing `basePath` attribute at root level
- `basePath` has wrong format (e.g., doesn't contain %T when it should)
- `basePath` has wrong type (numeric instead of string)
- Group corresponding to basePath with %T replaced doesn't exist (e.g., basePath="/data/%T/" but "/data/0/" missing)
- Missing `openPMDextension` attribute (should have "BeamPhysics;SpeciesType")
- `openPMDextension` has wrong value (missing BeamPhysics or SpeciesType)
- Missing `particlesPath` attribute at root level
- `particlesPath` attribute exists but the path doesn't exist in file
- Missing `iterationEncoding` attribute
- Missing `iterationFormat` attribute
- `iterationEncoding` has invalid value (not "fileBased" or "groupBased")

## HDF5 Structure Issues
- File that is missing the group `particles` in base
- `particles` is a dataset instead of a group
- File is valid HDF5 but completely empty (no groups at all)
- `/particles` contains both groups AND datasets

## Species Group Issues
- File that has `particles` in base, but no particle species contained inside
- Species inside `particles` is a dataset, not a group
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

## Unit Conversion (unitSI)
- Valid file where position data stored in non-SI units (e.g., cm) with unitSI=0.01
- Valid file where momentum data stored in non-SI units (e.g., eV/c) with appropriate unitSI
- Valid file where time stored in non-SI units (e.g., ps) with unitSI=1e-12
- File missing unitSI attribute (should default to 1.0)
- File with unitSI of wrong type (e.g., string instead of float64)

## Valid Files
- Particle Group where array attribute has constant value (ie written as attribute in group)
- Valid file where each attribute in particle group has unique value
  - Constant value across all particles (eg set as attribute of group)
  - Different (incrementing) values for each particle (eg using datasets to store the values)
- Valid file with non-default particlesPath (e.g., "beams/" instead of "particles/")
- Valid file with non-default basePath
- Valid file with multiple iterations (groupBased encoding with %T in basePath)
"""

from pmd_beamphysics import ParticleGroup
import numpy as np
from pathlib import Path
import h5py


# ============================================================================
# Helper functions
# ============================================================================


def get_test_value(name: str, particle_idx: int = 0, constant: bool = True) -> float:
    """
    Calculate test value for a field based on naming convention.

    Parameters
    ----------
    name : str
        Field name (e.g., "position/x")
    particle_idx : int
        Particle index (0-based)
    constant : bool
        If True, all particles have same value. If False, increment per particle.

    Returns
    -------
    float
        Test value = sum(ord(c) for c in name) + (particle_idx if not constant else 0)
    """
    base_value = sum(ord(c) for c in name)
    return float(base_value + (particle_idx if not constant else 0))


def write_openpmd_header(
    f: h5py.File,
    iteration: int = 0,
    base_path: str = "/data/%T/",
    particles_path: str = "particles/",
    iteration_encoding: str = "groupBased",
):
    """
    Write valid OpenPMD root-level attributes to an HDF5 file.

    Parameters
    ----------
    f : h5py.File
        Open h5py.File object
    iteration : int
        Iteration number for %T substitution
    base_path : str
        Base path template (must contain %T for groupBased)
    particles_path : str
        Relative path to particles from basePath
    iteration_encoding : str
        "groupBased" or "fileBased"
    """
    f.attrs["openPMD"] = "2.0.0"
    f.attrs["openPMDextension"] = "BeamPhysics;SpeciesType"
    f.attrs["basePath"] = base_path
    f.attrs["particlesPath"] = particles_path
    f.attrs["iterationEncoding"] = iteration_encoding

    if iteration_encoding == "groupBased":
        f.attrs["iterationFormat"] = base_path
    else:
        f.attrs["iterationFormat"] = "data_%T.h5"

    f.attrs["software"] = "generate_test_data.py"
    f.attrs["softwareVersion"] = "1.0.0"


def write_iteration_attributes(grp: h5py.Group, time: float = 0.0, dt: float = 1.0e-15):
    """
    Write iteration-level attributes (time, dt, timeUnitSI).

    Parameters
    ----------
    grp : h5py.Group
        h5py.Group for the iteration (e.g., /data/0/)
    time : float
        Time for this iteration
    dt : float
        Time step
    """
    grp.attrs["time"] = time
    grp.attrs["dt"] = dt
    grp.attrs["timeUnitSI"] = 1.0


def write_record(
    parent_grp: h5py.Group,
    record_path: str,
    data: np.ndarray,
    unit_si: float = 1.0,
    unit_dimension: np.ndarray = None,
    time_offset: float = 0.0,
    constant: bool = False,
):
    """
    Write a single particle record component with OpenPMD metadata.

    Parameters
    ----------
    parent_grp : h5py.Group
        Parent group (e.g., species group)
    record_path : str
        Path to record component (e.g., "position/x", "weight")
    data : np.ndarray
        Data array to write
    unit_si : float
        Unit conversion factor to SI
    unit_dimension : np.ndarray
        7-element array of dimension powers [L,M,T,I,theta,N,J]
    time_offset : float
        Time offset for this record
    constant : bool
        If True, write as constant record (group with value attribute)
    """
    if unit_dimension is None:
        unit_dimension = np.array([0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)

    if "/" in record_path:
        record_name, component = record_path.rsplit("/", 1)

        if record_name not in parent_grp:
            rec_grp = parent_grp.create_group(record_name)
            rec_grp.attrs["unitDimension"] = unit_dimension
            rec_grp.attrs["timeOffset"] = time_offset
        else:
            rec_grp = parent_grp[record_name]

        if constant:
            comp_grp = rec_grp.create_group(component)
            comp_grp.attrs["value"] = data[0] if len(data) > 0 else 0.0
            comp_grp.attrs["shape"] = np.array(data.shape, dtype=np.uint64)
            comp_grp.attrs["unitSI"] = unit_si
        else:
            dset = rec_grp.create_dataset(component, data=data)
            dset.attrs["unitSI"] = unit_si
    else:
        if constant:
            comp_grp = parent_grp.create_group(record_path)
            comp_grp.attrs["value"] = data[0] if len(data) > 0 else 0.0
            comp_grp.attrs["shape"] = np.array(data.shape, dtype=np.uint64)
            comp_grp.attrs["unitSI"] = unit_si
            comp_grp.attrs["unitDimension"] = unit_dimension
            comp_grp.attrs["timeOffset"] = time_offset
        else:
            dset = parent_grp.create_dataset(record_path, data=data)
            dset.attrs["unitSI"] = unit_si
            dset.attrs["unitDimension"] = unit_dimension
            dset.attrs["timeOffset"] = time_offset


def write_particle_group(
    iteration_grp: h5py.Group,
    species_name: str,
    num_particles: int,
    particles_path: str = "particles/",
    constant_records: bool = False,
):
    """
    Write a complete particle group with all required records.

    Parameters
    ----------
    iteration_grp : h5py.Group
        The iteration group (e.g., /data/0/)
    species_name : str
        Name of the species (e.g., "electron")
    num_particles : int
        Number of particles
    particles_path : str
        Relative path to particles
    constant_records : bool
        If True, write records as constant (group with value attribute)

    Returns
    -------
    h5py.Group
        The species group
    """
    if particles_path.rstrip("/") not in iteration_grp:
        particles_grp = iteration_grp.create_group(particles_path.rstrip("/"))
    else:
        particles_grp = iteration_grp[particles_path.rstrip("/")]

    species_grp = particles_grp.create_group(species_name)
    species_grp.attrs["numParticles"] = np.int64(num_particles)
    species_grp.attrs["speciesType"] = species_name

    pos_dim = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
    for comp in ["x", "y", "z"]:
        path = f"position/{comp}"
        data = np.array(
            [
                get_test_value(path, i, constant=constant_records)
                for i in range(num_particles)
            ],
            dtype=np.float64,
        )
        write_record(
            species_grp,
            path,
            data,
            unit_si=1.0,
            unit_dimension=pos_dim,
            constant=constant_records,
        )

    mom_dim = np.array([1.0, 1.0, -1.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
    for comp in ["x", "y", "z"]:
        path = f"momentum/{comp}"
        data = np.array(
            [
                get_test_value(path, i, constant=constant_records)
                for i in range(num_particles)
            ],
            dtype=np.float64,
        )
        write_record(
            species_grp,
            path,
            data,
            unit_si=1.0,
            unit_dimension=mom_dim,
            constant=constant_records,
        )

    time_dim = np.array([0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
    data = np.array(
        [
            get_test_value("time", i, constant=constant_records)
            for i in range(num_particles)
        ],
        dtype=np.float64,
    )
    write_record(
        species_grp,
        "time",
        data,
        unit_si=1.0,
        unit_dimension=time_dim,
        constant=constant_records,
    )

    dimless = np.array([0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
    data = np.array(
        [
            get_test_value("weight", i, constant=constant_records)
            for i in range(num_particles)
        ],
        dtype=np.float64,
    )
    write_record(
        species_grp,
        "weight",
        data,
        unit_si=1.0,
        unit_dimension=dimless,
        constant=constant_records,
    )

    data = np.array(
        [
            int(get_test_value("id", i, constant=constant_records))
            for i in range(num_particles)
        ],
        dtype=np.int64,
    )
    write_record(
        species_grp,
        "id",
        data,
        unit_si=1.0,
        unit_dimension=dimless,
        constant=constant_records,
    )

    data = np.ones(num_particles, dtype=np.int64)
    write_record(
        species_grp,
        "particleStatus",
        data,
        unit_si=1.0,
        unit_dimension=dimless,
        constant=constant_records,
    )

    return species_grp


# ============================================================================
# Test file generators - OpenPMD Root-Level Metadata Issues
# ============================================================================


def make_missing_openpmd_attr(fname: str):
    """Missing `openPMD` attribute at root level"""
    with h5py.File(fname, "w") as f:
        f.attrs["basePath"] = "/data/%T/"
        f.attrs["openPMDextension"] = "BeamPhysics;SpeciesType"
        f.attrs["particlesPath"] = "particles/"
        f.attrs["iterationEncoding"] = "groupBased"
        f.attrs["iterationFormat"] = "/data/%T/"
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
    print(f"make_missing_openpmd_attr: Created {fname}")


def make_wrong_version_format(fname: str):
    """`openPMD` attribute has wrong version format"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        f.attrs["openPMD"] = "2.0"
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
    print(f"make_wrong_version_format: Created {fname}")


def make_unsupported_version(fname: str):
    """`openPMD` attribute has unsupported version"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        f.attrs["openPMD"] = "99.0.0"
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
    print(f"make_unsupported_version: Created {fname}")


def make_missing_basepath(fname: str):
    """Missing `basePath` attribute at root level"""
    with h5py.File(fname, "w") as f:
        f.attrs["openPMD"] = "2.0.0"
        f.attrs["openPMDextension"] = "BeamPhysics;SpeciesType"
        f.attrs["particlesPath"] = "particles/"
        f.attrs["iterationEncoding"] = "groupBased"
        f.attrs["iterationFormat"] = "/data/%T/"
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
    print(f"make_missing_basepath: Created {fname}")


def make_basepath_wrong_format(fname: str):
    """`basePath` has wrong format"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        f.attrs["basePath"] = "/data/"
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
    print(f"make_basepath_wrong_format: Created {fname}")


def make_basepath_wrong_type(fname: str):
    """`basePath` has wrong type"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        f.attrs["basePath"] = 123
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
    print(f"make_basepath_wrong_type: Created {fname}")


def make_basepath_group_missing(fname: str):
    """basePath points to non-existent group"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
    print(f"make_basepath_group_missing: Created {fname}")


def make_missing_extension(fname: str):
    """Missing `openPMDextension` attribute"""
    with h5py.File(fname, "w") as f:
        f.attrs["openPMD"] = "2.0.0"
        f.attrs["basePath"] = "/data/%T/"
        f.attrs["particlesPath"] = "particles/"
        f.attrs["iterationEncoding"] = "groupBased"
        f.attrs["iterationFormat"] = "/data/%T/"
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
    print(f"make_missing_extension: Created {fname}")


def make_wrong_extension(fname: str):
    """`openPMDextension` has wrong value"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        f.attrs["openPMDextension"] = "ED-PIC"
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
    print(f"make_wrong_extension: Created {fname}")


def make_missing_particles_path(fname: str):
    """Missing `particlesPath` attribute at root level"""
    with h5py.File(fname, "w") as f:
        f.attrs["openPMD"] = "2.0.0"
        f.attrs["openPMDextension"] = "BeamPhysics;SpeciesType"
        f.attrs["basePath"] = "/data/%T/"
        f.attrs["iterationEncoding"] = "groupBased"
        f.attrs["iterationFormat"] = "/data/%T/"
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
    print(f"make_missing_particles_path: Created {fname}")


def make_particles_path_doesnt_exist(fname: str):
    """`particlesPath` attribute exists but the path doesn't exist in file"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
    print(f"make_particles_path_doesnt_exist: Created {fname}")


def make_missing_iteration_encoding(fname: str):
    """Missing `iterationEncoding` attribute"""
    with h5py.File(fname, "w") as f:
        f.attrs["openPMD"] = "2.0.0"
        f.attrs["openPMDextension"] = "BeamPhysics;SpeciesType"
        f.attrs["basePath"] = "/data/%T/"
        f.attrs["particlesPath"] = "particles/"
        f.attrs["iterationFormat"] = "/data/%T/"
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
    print(f"make_missing_iteration_encoding: Created {fname}")


def make_missing_iteration_format(fname: str):
    """Missing `iterationFormat` attribute"""
    with h5py.File(fname, "w") as f:
        f.attrs["openPMD"] = "2.0.0"
        f.attrs["openPMDextension"] = "BeamPhysics;SpeciesType"
        f.attrs["basePath"] = "/data/%T/"
        f.attrs["particlesPath"] = "particles/"
        f.attrs["iterationEncoding"] = "groupBased"
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
    print(f"make_missing_iteration_format: Created {fname}")


def make_invalid_iteration_encoding(fname: str):
    """`iterationEncoding` has invalid value"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        f.attrs["iterationEncoding"] = "streamBased"
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
    print(f"make_invalid_iteration_encoding: Created {fname}")


# ============================================================================
# Test file generators - HDF5 Structure Issues
# ============================================================================


def make_missing_particles_group(fname: str):
    """File that is missing the group `particles` in base"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
    print(f"make_missing_particles_group: Created {fname}")


def make_particles_is_dataset(fname: str):
    """`particles` is a dataset instead of a group"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        iter_grp.create_dataset("particles", data=np.array([1, 2, 3]))
    print(f"make_particles_is_dataset: Created {fname}")


def make_completely_empty_file(fname: str):
    """File is valid HDF5 but completely empty (no groups at all)"""
    with h5py.File(fname, "w"):
        pass
    print(f"make_completely_empty_file: Created {fname}")


def make_particles_mixed_content(fname: str):
    """`/particles` contains both groups AND datasets"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        particles_grp = iter_grp.create_group("particles")
        write_particle_group(iter_grp, "electron", 10)
        particles_grp.create_dataset("invalid_dataset", data=np.array([1, 2, 3]))
    print(f"make_particles_mixed_content: Created {fname}")


# ============================================================================
# Test file generators - Species Group Issues
# ============================================================================


def make_empty_particles_group(fname: str):
    """File that has `particles` in base, but no particle species contained inside"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        iter_grp.create_group("particles")
    print(f"make_empty_particles_group: Created {fname}")


def make_species_is_dataset(fname: str):
    """Species inside `particles` is a dataset, not a group"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        particles_grp = iter_grp.create_group("particles")
        particles_grp.create_dataset("electron", data=np.array([1, 2, 3]))
    print(f"make_species_is_dataset: Created {fname}")


def make_species_very_long_name(fname: str):
    """Very long species name (>255 chars) - could overflow buffers"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        long_name = "electron_" + "x" * 300
        write_particle_group(iter_grp, long_name, 10)
    print(f"make_species_very_long_name: Created {fname}")


# ============================================================================
# Valid files using pmd_beamphysics library
# ============================================================================


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
    test_data_dir = Path(__file__).parent.parent / "tests" / "data"
    test_data_dir.mkdir(parents=True, exist_ok=True)

    print("\n" + "=" * 80)
    print("OpenPMD Root-Level Metadata Issues")
    print("=" * 80)

    make_missing_openpmd_attr(str(test_data_dir / "missing_openpmd_attr.h5"))
    make_wrong_version_format(str(test_data_dir / "wrong_version_format.h5"))
    make_unsupported_version(str(test_data_dir / "unsupported_version.h5"))
    make_missing_basepath(str(test_data_dir / "missing_basepath.h5"))
    make_basepath_wrong_format(str(test_data_dir / "basepath_wrong_format.h5"))
    make_basepath_wrong_type(str(test_data_dir / "basepath_wrong_type.h5"))
    make_basepath_group_missing(str(test_data_dir / "basepath_group_missing.h5"))
    make_missing_extension(str(test_data_dir / "missing_extension.h5"))
    make_wrong_extension(str(test_data_dir / "wrong_extension.h5"))
    make_missing_particles_path(str(test_data_dir / "missing_particles_path.h5"))
    make_particles_path_doesnt_exist(
        str(test_data_dir / "particles_path_doesnt_exist.h5")
    )
    make_missing_iteration_encoding(
        str(test_data_dir / "missing_iteration_encoding.h5")
    )
    make_missing_iteration_format(str(test_data_dir / "missing_iteration_format.h5"))
    make_invalid_iteration_encoding(
        str(test_data_dir / "invalid_iteration_encoding.h5")
    )

    print("\n" + "=" * 80)
    print("HDF5 Structure Issues")
    print("=" * 80)

    make_missing_particles_group(str(test_data_dir / "missing_particles_group.h5"))
    make_particles_is_dataset(str(test_data_dir / "particles_is_dataset.h5"))
    make_completely_empty_file(str(test_data_dir / "completely_empty_file.h5"))
    make_particles_mixed_content(str(test_data_dir / "particles_mixed_content.h5"))

    print("\n" + "=" * 80)
    print("Species Group Issues")
    print("=" * 80)

    make_empty_particles_group(str(test_data_dir / "empty_particles_group.h5"))
    make_species_is_dataset(str(test_data_dir / "species_is_dataset.h5"))
    make_species_very_long_name(str(test_data_dir / "species_very_long_name.h5"))

    print("\n" + "=" * 80)
    print("Valid files using pmd_beamphysics library")
    print("=" * 80)

    make_attr_count(str(test_data_dir / "attr_count_32.h5"), 32)

    print("\nAll test files created successfully!")
