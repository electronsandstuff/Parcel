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

## Series Tests
- Valid file-based series with multiple files matching pattern (e.g., data_%T.h5 with data_0.h5, data_1.h5, data_2.h5)
- File-based series with other non-matching files in directory
- Files with multiple %T present in pattern (e.g., data_%T_iter_%T.h5)
- Group-based series with multiple iterations
- Multiple groups in HDF5 that don't match iteration pattern
- Test iterationFormats with prefix and suffix (e.g., step_%T_final or data_%T_test)
"""

import numpy as np
from pathlib import Path
import h5py
import argparse
from pmd_beamphysics import ParticleGroup


# SI conversion: eV/c -> kg*m/s
eV_c_to_SI = 299792456 * 1.782661921e-36

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
    """Missing `particlesPath` attribute - should succeed with zero species"""
    with h5py.File(fname, "w") as f:
        f.attrs["openPMD"] = "2.0.0"
        f.attrs["openPMDextension"] = "BeamPhysics;SpeciesType"
        f.attrs["basePath"] = "/data/%T/"
        f.attrs["iterationEncoding"] = "groupBased"
        f.attrs["iterationFormat"] = "/data/%T/"
        # Note: NO particlesPath attribute
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        # Note: NO particles created
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
# Test file generators - Missing/Invalid Attributes
# ============================================================================


def make_missing_num_particles(fname: str):
    """Missing `numParticles` attribute"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        particles_grp = iter_grp.create_group("particles")
        species_grp = particles_grp.create_group("electron")
        species_grp.attrs["speciesType"] = "electron"

        pos_dim = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        for comp in ["x", "y", "z"]:
            path = f"position/{comp}"
            data = np.array(
                [get_test_value(path, i, constant=False) for i in range(10)],
                dtype=np.float64,
            )
            write_record(species_grp, path, data, unit_si=1.0, unit_dimension=pos_dim)
    print(f"make_missing_num_particles: Created {fname}")


def make_num_particles_wrong_type(fname: str):
    """`numParticles` has wrong type (e.g., float instead of int64)"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
        species_grp = iter_grp["particles/electron"]
        species_grp.attrs["numParticles"] = 10.5
    print(f"make_num_particles_wrong_type: Created {fname}")


def make_num_particles_zero(fname: str):
    """`numParticles` is 0 (edge case)"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 0)
    print(f"make_num_particles_zero: Created {fname}")


def make_num_particles_negative(fname: str):
    """`numParticles` is negative"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
        species_grp = iter_grp["particles/electron"]
        species_grp.attrs["numParticles"] = np.int64(-5)
    print(f"make_num_particles_negative: Created {fname}")


def make_num_particles_one(fname: str):
    """`numParticles` is 1 (single particle edge case)"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 1)
    print(f"make_num_particles_one: Created {fname}")


def make_missing_species_type(fname: str):
    """Missing `speciesType` attribute"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        particles_grp = iter_grp.create_group("particles")
        species_grp = particles_grp.create_group("electron")
        species_grp.attrs["numParticles"] = np.int64(10)

        pos_dim = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        for comp in ["x", "y", "z"]:
            path = f"position/{comp}"
            data = np.array(
                [get_test_value(path, i, constant=False) for i in range(10)],
                dtype=np.float64,
            )
            write_record(species_grp, path, data, unit_si=1.0, unit_dimension=pos_dim)
    print(f"make_missing_species_type: Created {fname}")


def make_species_type_wrong_type(fname: str):
    """`speciesType` has wrong type (e.g., numeric instead of string)"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
        species_grp = iter_grp["particles/electron"]
        species_grp.attrs["speciesType"] = 42
    print(f"make_species_type_wrong_type: Created {fname}")


def make_position_is_dataset(fname: str):
    """`position/` is a dataset instead of a group"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        particles_grp = iter_grp.create_group("particles")
        species_grp = particles_grp.create_group("electron")
        species_grp.attrs["numParticles"] = np.int64(10)
        species_grp.attrs["speciesType"] = "electron"

        species_grp.create_dataset("position", data=np.array([1, 2, 3]))
    print(f"make_position_is_dataset: Created {fname}")


def make_momentum_group_empty(fname: str):
    """`momentum/` group exists but is empty"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
        species_grp = iter_grp["particles/electron"]

        mom_grp = species_grp.create_group("momentum_empty")
        mom_dim = np.array([1.0, 1.0, -1.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        mom_grp.attrs["unitDimension"] = mom_dim
        mom_grp.attrs["timeOffset"] = 0.0
    print(f"make_momentum_group_empty: Created {fname}")


def make_position_x_wrong_rank(fname: str):
    """position/x is 2D array instead of 1D (wrong rank/dimensions)"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        particles_grp = iter_grp.create_group("particles")
        species_grp = particles_grp.create_group("electron")
        species_grp.attrs["numParticles"] = np.int64(10)
        species_grp.attrs["speciesType"] = "electron"

        # Create position group
        pos_grp = species_grp.create_group("position")
        pos_dim = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        pos_grp.attrs["unitDimension"] = pos_dim
        pos_grp.attrs["timeOffset"] = 0.0

        # Create position/x as 2D array [10, 3] instead of 1D [10]
        data_2d = np.array(
            [
                [get_test_value("position/x", i, constant=False) for _ in range(3)]
                for i in range(10)
            ],
            dtype=np.float64,
        )
        pos_x = pos_grp.create_dataset("x", data=data_2d)
        pos_x.attrs["unitSI"] = 1.0

        # Create position/y and position/z as normal 1D arrays
        for comp in ["y", "z"]:
            path = f"position/{comp}"
            data = np.array(
                [get_test_value(path, i, constant=False) for i in range(10)],
                dtype=np.float64,
            )
            write_record(species_grp, path, data, unit_si=1.0, unit_dimension=pos_dim)
    print(f"make_position_x_wrong_rank: Created {fname}")


# ============================================================================
# Test file generators - Array Size Mismatches
# ============================================================================


def make_dataset_larger_than_num_particles(fname: str):
    """Dataset size larger than `numParticles`"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        particles_grp = iter_grp.create_group("particles")
        species_grp = particles_grp.create_group("electron")
        species_grp.attrs["numParticles"] = np.int64(10)
        species_grp.attrs["speciesType"] = "electron"

        pos_dim = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        for comp in ["x", "y", "z"]:
            path = f"position/{comp}"
            data = np.array(
                [get_test_value(path, i, constant=False) for i in range(20)],
                dtype=np.float64,
            )
            write_record(species_grp, path, data, unit_si=1.0, unit_dimension=pos_dim)
    print(f"make_dataset_larger_than_num_particles: Created {fname}")


def make_dataset_smaller_than_num_particles(fname: str):
    """Dataset size smaller than `numParticles`"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        particles_grp = iter_grp.create_group("particles")
        species_grp = particles_grp.create_group("electron")
        species_grp.attrs["numParticles"] = np.int64(20)
        species_grp.attrs["speciesType"] = "electron"

        pos_dim = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        for comp in ["x", "y", "z"]:
            path = f"position/{comp}"
            data = np.array(
                [get_test_value(path, i, constant=False) for i in range(10)],
                dtype=np.float64,
            )
            write_record(species_grp, path, data, unit_si=1.0, unit_dimension=pos_dim)
    print(f"make_dataset_smaller_than_num_particles: Created {fname}")


def make_dataset_size_zero(fname: str):
    """Dataset size is 0 when `numParticles` > 0"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        particles_grp = iter_grp.create_group("particles")
        species_grp = particles_grp.create_group("electron")
        species_grp.attrs["numParticles"] = np.int64(10)
        species_grp.attrs["speciesType"] = "electron"

        pos_dim = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        for comp in ["x", "y", "z"]:
            path = f"position/{comp}"
            data = np.array([], dtype=np.float64)
            write_record(species_grp, path, data, unit_si=1.0, unit_dimension=pos_dim)
    print(f"make_dataset_size_zero: Created {fname}")


def make_position_components_different_sizes(fname: str):
    """Different sizes for position/x, position/y, position/z"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        particles_grp = iter_grp.create_group("particles")
        species_grp = particles_grp.create_group("electron")
        species_grp.attrs["numParticles"] = np.int64(10)
        species_grp.attrs["speciesType"] = "electron"

        pos_dim = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)

        data_x = np.array(
            [get_test_value("position/x", i, constant=False) for i in range(10)],
            dtype=np.float64,
        )
        write_record(
            species_grp, "position/x", data_x, unit_si=1.0, unit_dimension=pos_dim
        )

        data_y = np.array(
            [get_test_value("position/y", i, constant=False) for i in range(8)],
            dtype=np.float64,
        )
        write_record(
            species_grp, "position/y", data_y, unit_si=1.0, unit_dimension=pos_dim
        )

        data_z = np.array(
            [get_test_value("position/z", i, constant=False) for i in range(12)],
            dtype=np.float64,
        )
        write_record(
            species_grp, "position/z", data_z, unit_si=1.0, unit_dimension=pos_dim
        )
    print(f"make_position_components_different_sizes: Created {fname}")


def make_optional_fields_different_length(fname: str):
    """Datasets for optional fields have different lengths than position"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        particles_grp = iter_grp.create_group("particles")
        species_grp = particles_grp.create_group("electron")
        species_grp.attrs["numParticles"] = np.int64(10)
        species_grp.attrs["speciesType"] = "electron"

        pos_dim = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        for comp in ["x", "y", "z"]:
            path = f"position/{comp}"
            data = np.array(
                [get_test_value(path, i, constant=False) for i in range(10)],
                dtype=np.float64,
            )
            write_record(species_grp, path, data, unit_si=1.0, unit_dimension=pos_dim)

        dimless = np.array([0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        data = np.array(
            [get_test_value("weight", i, constant=False) for i in range(15)],
            dtype=np.float64,
        )
        write_record(species_grp, "weight", data, unit_si=1.0, unit_dimension=dimless)
    print(f"make_optional_fields_different_length: Created {fname}")


# ============================================================================
# Test file generators - Invalid Constant Records
# ============================================================================


def make_constant_record_missing_value(fname: str):
    """Constant record (group) exists but missing `value` attribute"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
        species_grp = iter_grp["particles/electron"]

        # Replace the weight dataset with a group that's missing the 'value' attribute
        del species_grp["weight"]
        weight_grp = species_grp.create_group("weight")
        weight_grp.attrs["shape"] = np.array([10], dtype=np.uint64)
        dimless = np.array([0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        weight_grp.attrs["unitSI"] = 1.0
        weight_grp.attrs["unitDimension"] = dimless
        weight_grp.attrs["timeOffset"] = 0.0
    print(f"make_constant_record_missing_value: Created {fname}")


def make_constant_record_wrong_type_value(fname: str):
    """Constant record with wrong-typed `value` attribute"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
        species_grp = iter_grp["particles/electron"]

        # Replace the weight dataset with a group that has a string 'value' attribute
        del species_grp["weight"]
        weight_grp = species_grp.create_group("weight")
        weight_grp.attrs["value"] = "not_a_number"
        weight_grp.attrs["shape"] = np.array([10], dtype=np.uint64)
        dimless = np.array([0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        weight_grp.attrs["unitSI"] = 1.0
        weight_grp.attrs["unitDimension"] = dimless
        weight_grp.attrs["timeOffset"] = 0.0
    print(f"make_constant_record_wrong_type_value: Created {fname}")


def make_both_dataset_and_constant(fname: str):
    """Both dataset AND constant record exist for same field"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        particles_grp = iter_grp.create_group("particles")
        species_grp = particles_grp.create_group("electron")
        species_grp.attrs["numParticles"] = np.int64(10)
        species_grp.attrs["speciesType"] = "electron"

        pos_dim = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        pos_grp = species_grp.create_group("position")
        pos_grp.attrs["unitDimension"] = pos_dim
        pos_grp.attrs["timeOffset"] = 0.0

        data_x = np.array(
            [get_test_value("position/x", i, constant=False) for i in range(10)],
            dtype=np.float64,
        )
        pos_grp.create_dataset("x", data=data_x).attrs["unitSI"] = 1.0

        y_const = pos_grp.create_group("y")
        y_const.attrs["value"] = get_test_value("position/y", 0, constant=True)
        y_const.attrs["shape"] = np.array([10], dtype=np.uint64)
        y_const.attrs["unitSI"] = 1.0

        data_z = np.array(
            [get_test_value("position/z", i, constant=False) for i in range(10)],
            dtype=np.float64,
        )
        pos_grp.create_dataset("z", data=data_z).attrs["unitSI"] = 1.0
    print(f"make_both_dataset_and_constant: Created {fname}")


def make_constant_value_is_array(fname: str):
    """Constant record with `value` that is an array instead of scalar"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
        species_grp = iter_grp["particles/electron"]

        # Replace the weight dataset with a group that has an array 'value' attribute
        del species_grp["weight"]
        weight_grp = species_grp.create_group("weight")
        weight_grp.attrs["value"] = np.array([1.0, 2.0, 3.0])
        weight_grp.attrs["shape"] = np.array([10], dtype=np.uint64)
        dimless = np.array([0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        weight_grp.attrs["unitSI"] = 1.0
        weight_grp.attrs["unitDimension"] = dimless
        weight_grp.attrs["timeOffset"] = 0.0
    print(f"make_constant_value_is_array: Created {fname}")


# ============================================================================
# Test file generators - Unit Conversion (unitSI)
# ============================================================================


def make_position_non_si_units(fname: str):
    """Valid file where position data stored in non-SI units (e.g., cm) with unitSI=0.01"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        particles_grp = iter_grp.create_group("particles")
        species_grp = particles_grp.create_group("electron")
        species_grp.attrs["numParticles"] = np.int64(10)
        species_grp.attrs["speciesType"] = "electron"

        pos_dim = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        for comp in ["x", "y", "z"]:
            path = f"position/{comp}"
            data = np.array(
                [get_test_value(path, i, constant=False) * 100.0 for i in range(10)],
                dtype=np.float64,
            )
            write_record(species_grp, path, data, unit_si=0.01, unit_dimension=pos_dim)
    print(f"make_position_non_si_units: Created {fname}")


def make_momentum_non_si_units(fname: str):
    """Valid file where momentum data stored in non-SI units (e.g., eV/c)"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        particles_grp = iter_grp.create_group("particles")
        species_grp = particles_grp.create_group("electron")
        species_grp.attrs["numParticles"] = np.int64(10)
        species_grp.attrs["speciesType"] = "electron"

        pos_dim = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        for comp in ["x", "y", "z"]:
            path = f"position/{comp}"
            data = np.array(
                [get_test_value(path, i, constant=False) for i in range(10)],
                dtype=np.float64,
            )
            write_record(species_grp, path, data, unit_si=1.0, unit_dimension=pos_dim)

        mom_dim = np.array([1.0, 1.0, -1.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        for comp in ["x", "y", "z"]:
            path = f"momentum/{comp}"
            data = np.array(
                [get_test_value(path, i, constant=False) for i in range(10)],
                dtype=np.float64,
            )
            write_record(
                species_grp, path, data, unit_si=eV_c_to_SI, unit_dimension=mom_dim
            )
    print(f"make_momentum_non_si_units: Created {fname}")


def make_time_non_si_units(fname: str):
    """Valid file where time stored in non-SI units (e.g., ps) with unitSI=1e-12"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        particles_grp = iter_grp.create_group("particles")
        species_grp = particles_grp.create_group("electron")
        species_grp.attrs["numParticles"] = np.int64(10)
        species_grp.attrs["speciesType"] = "electron"

        pos_dim = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        for comp in ["x", "y", "z"]:
            path = f"position/{comp}"
            data = np.array(
                [get_test_value(path, i, constant=False) for i in range(10)],
                dtype=np.float64,
            )
            write_record(species_grp, path, data, unit_si=1.0, unit_dimension=pos_dim)

        time_dim = np.array([0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        data = np.array(
            [get_test_value("time", i, constant=False) * 1e12 for i in range(10)],
            dtype=np.float64,
        )
        write_record(species_grp, "time", data, unit_si=1e-12, unit_dimension=time_dim)
    print(f"make_time_non_si_units: Created {fname}")


def make_missing_unitsi(fname: str):
    """File missing unitSI attribute (should default to 1.0)"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        particles_grp = iter_grp.create_group("particles")
        species_grp = particles_grp.create_group("electron")
        species_grp.attrs["numParticles"] = np.int64(10)
        species_grp.attrs["speciesType"] = "electron"

        pos_dim = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        pos_grp = species_grp.create_group("position")
        pos_grp.attrs["unitDimension"] = pos_dim
        pos_grp.attrs["timeOffset"] = 0.0

        for comp in ["x", "y", "z"]:
            data = np.array(
                [
                    get_test_value(f"position/{comp}", i, constant=False)
                    for i in range(10)
                ],
                dtype=np.float64,
            )
            pos_grp.create_dataset(comp, data=data)
    print(f"make_missing_unitsi: Created {fname}")


def make_unitsi_wrong_type(fname: str):
    """File with unitSI of wrong type (e.g., string instead of float64)"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
        species_grp = iter_grp["particles/electron"]

        pos_grp = species_grp["position"]
        x_dset = pos_grp["x"]
        x_dset.attrs["unitSI"] = "1.0"
    print(f"make_unitsi_wrong_type: Created {fname}")


# ============================================================================
# Test file generators - Valid Files
# ============================================================================


def make_valid_constant_records(fname: str):
    """Valid file with constant records (stored as group attributes)"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10, constant_records=True)
    print(f"make_valid_constant_records: Created {fname}")


def make_valid_dataset_records(fname: str):
    """Valid file with datasets (unique incrementing values per particle)"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f)
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10, constant_records=False)
    print(f"make_valid_dataset_records: Created {fname}")


def make_valid_non_default_particles_path(fname: str):
    """Valid file with non-default particlesPath"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f, particles_path="beams/")
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10, particles_path="beams/")
    print(f"make_valid_non_default_particles_path: Created {fname}")


def make_valid_non_default_base_path(fname: str):
    """Valid file with non-default basePath"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f, base_path="/simulations/%T/")
        iter_grp = f.create_group("simulations/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)
    print(f"make_valid_non_default_base_path: Created {fname}")


def make_valid_multiple_iterations(fname: str):
    """Valid file with multiple iterations (groupBased encoding)"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f, iteration_encoding="groupBased")

        for iteration in [0, 1, 2]:
            iter_grp = f.create_group(f"data/{iteration}")
            write_iteration_attributes(iter_grp)
            write_particle_group(iter_grp, "electron", 10)
    print(f"make_valid_multiple_iterations: Created {fname}")


def make_attr_count(fname: str, num_particles: int):
    """
    Create a BeamPhysics file where each required array is set to the same value, but
    increasing for each attributes. Ie x->0, y->1, z->2, ...
    Momentum is stored in eV/c units (4, 5, 6 eV/c) with appropriate unitSI conversion.
    Uses basePath="/" (iteration at root level, no %T).
    """
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f, base_path="/", iteration_encoding="groupBased")
        # basePath="/" means iteration is at root level
        iter_grp = f  # Use file root as iteration group
        write_iteration_attributes(iter_grp)

        particles_grp = iter_grp.create_group("particles")
        species_grp = particles_grp.create_group("electron")
        species_grp.attrs["numParticles"] = np.int64(num_particles)
        species_grp.attrs["speciesType"] = "electron"

        # Position in SI units (meters)
        pos_dim = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        write_record(
            species_grp,
            "position/x",
            np.full(num_particles, 0.0),
            unit_si=1.0,
            unit_dimension=pos_dim,
        )
        write_record(
            species_grp,
            "position/y",
            np.full(num_particles, 1.0),
            unit_si=1.0,
            unit_dimension=pos_dim,
        )
        write_record(
            species_grp,
            "position/z",
            np.full(num_particles, 2.0),
            unit_si=1.0,
            unit_dimension=pos_dim,
        )

        # Time in SI units (seconds)
        time_dim = np.array([0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        write_record(
            species_grp,
            "time",
            np.full(num_particles, 3.0),
            unit_si=1.0,
            unit_dimension=time_dim,
        )

        # Momentum in eV/c units (4, 5, 6 eV/c with conversion to SI)
        mom_dim = np.array([1.0, 1.0, -1.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        write_record(
            species_grp,
            "momentum/x",
            np.full(num_particles, 4.0),
            unit_si=eV_c_to_SI,
            unit_dimension=mom_dim,
        )
        write_record(
            species_grp,
            "momentum/y",
            np.full(num_particles, 5.0),
            unit_si=eV_c_to_SI,
            unit_dimension=mom_dim,
        )
        write_record(
            species_grp,
            "momentum/z",
            np.full(num_particles, 6.0),
            unit_si=eV_c_to_SI,
            unit_dimension=mom_dim,
        )

        # Other dimensionless quantities
        dimless = np.array([0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        write_record(
            species_grp,
            "weight",
            np.full(num_particles, 7.0),
            unit_si=1.0,
            unit_dimension=dimless,
        )
        write_record(
            species_grp,
            "id",
            np.full(num_particles, 8, dtype=np.int64),
            unit_si=1.0,
            unit_dimension=dimless,
        )
        write_record(
            species_grp,
            "particleStatus",
            np.full(num_particles, 9, dtype=np.int64),
            unit_si=1.0,
            unit_dimension=dimless,
        )

    # Write message
    print(f"make_attr_count: Wrote {num_particles} particles to {fname}")


# ============================================================================
# Valid files using pmd_beamphysics library
# ============================================================================


def make_pmd_beamphysics_constant(fname: str):
    pg = ParticleGroup(
        data={
            "x": np.full(10, get_test_value("position/x")),
            "y": np.full(10, get_test_value("position/y")),
            "z": np.full(10, get_test_value("position/z")),
            "px": np.full(10, get_test_value("momentum/x")),
            "py": np.full(10, get_test_value("momentum/y")),
            "pz": np.full(10, get_test_value("momentum/z")),
            "t": np.full(10, get_test_value("time")),
            "status": np.ones(10, dtype=int),
            "weight": np.full(10, get_test_value("weight")),
            "species": "electron",
        }
    )
    pg.write(fname)
    print(f"make_pmd_beamphysics_constant: Wrote particles to {fname}")


def make_pmd_beamphysics_dataset(fname: str):
    """Create ParticleGroup with dataset (incrementing) values"""
    num_particles = 10
    pg = ParticleGroup(
        data={
            "x": np.array(
                [
                    get_test_value("position/x", i, constant=False)
                    for i in range(num_particles)
                ]
            ),
            "y": np.array(
                [
                    get_test_value("position/y", i, constant=False)
                    for i in range(num_particles)
                ]
            ),
            "z": np.array(
                [
                    get_test_value("position/z", i, constant=False)
                    for i in range(num_particles)
                ]
            ),
            "px": np.array(
                [
                    get_test_value("momentum/x", i, constant=False)
                    for i in range(num_particles)
                ]
            ),
            "py": np.array(
                [
                    get_test_value("momentum/y", i, constant=False)
                    for i in range(num_particles)
                ]
            ),
            "pz": np.array(
                [
                    get_test_value("momentum/z", i, constant=False)
                    for i in range(num_particles)
                ]
            ),
            "t": np.array(
                [
                    get_test_value("time", i, constant=False)
                    for i in range(num_particles)
                ]
            ),
            "status": np.ones(num_particles, dtype=int),
            "weight": np.array(
                [
                    get_test_value("weight", i, constant=False)
                    for i in range(num_particles)
                ]
            ),
            "species": "electron",
        }
    )
    pg.write(fname)
    print(f"make_pmd_beamphysics_dataset: Wrote particles to {fname}")


# ============================================================================
# Test file generators - Series Tests
# ============================================================================


def make_valid_file_based_series(dirname: str):
    """Valid file-based series with multiple files matching pattern"""
    dir_path = Path(dirname)
    dir_path.mkdir(parents=True, exist_ok=True)

    # Create 3 files: data_0.h5, data_1.h5, data_2.h5
    for iteration in [0, 1, 2]:
        fname = dir_path / f"data_{iteration}.h5"
        with h5py.File(fname, "w") as f:
            write_openpmd_header(
                f, base_path="/data/0/", iteration_encoding="fileBased"
            )
            f.attrs["iterationFormat"] = "data_%T.h5"

            iter_grp = f.create_group("data/0")
            write_iteration_attributes(iter_grp, time=iteration * 1.0e-15)
            write_particle_group(iter_grp, "electron", 10)

    print(f"make_valid_file_based_series: Created 3 files in {dirname}")


def make_file_based_series_with_other_files(dirname: str):
    """File-based series with other non-matching files in directory"""
    dir_path = Path(dirname)
    dir_path.mkdir(parents=True, exist_ok=True)

    # Create matching files
    for iteration in [0, 1, 2]:
        fname = dir_path / f"sim_{iteration}.h5"
        with h5py.File(fname, "w") as f:
            write_openpmd_header(
                f, base_path="/data/0/", iteration_encoding="fileBased"
            )
            f.attrs["iterationFormat"] = "sim_%T.h5"

            iter_grp = f.create_group("data/0")
            write_iteration_attributes(iter_grp)
            write_particle_group(iter_grp, "electron", 10)

    # Create non-matching files that should be ignored
    for fname in ["other_data.h5", "sim_backup.h5", "sim_final.h5", "readme.txt"]:
        fpath = dir_path / fname
        if fname.endswith(".h5"):
            with h5py.File(fpath, "w") as f:
                f.attrs["note"] = "This file should be ignored"
        else:
            fpath.write_text("This is not an HDF5 file")

    print(f"make_file_based_series_with_other_files: Created files in {dirname}")


def make_file_based_multiple_percent_t(dirname: str):
    """Files with multiple %T present in pattern"""
    dir_path = Path(dirname)
    dir_path.mkdir(parents=True, exist_ok=True)

    # Create files with pattern data_%T_iter_%T.h5
    # This is ambiguous - only the first %T should be used for iteration
    for iteration in [0, 1]:
        fname = dir_path / f"data_{iteration}_iter_{iteration}.h5"
        with h5py.File(fname, "w") as f:
            write_openpmd_header(
                f, base_path="/data/0/", iteration_encoding="fileBased"
            )
            f.attrs["iterationFormat"] = "data_%T_iter_%T.h5"

            iter_grp = f.create_group("data/0")
            write_iteration_attributes(iter_grp)
            write_particle_group(iter_grp, "electron", 10)

    print(f"make_file_based_multiple_percent_t: Created files in {dirname}")


def make_group_based_non_matching_groups(fname: str):
    """Multiple groups in HDF5 that don't match iteration pattern"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(f, base_path="/data/%T/", iteration_encoding="groupBased")

        # Create matching iteration groups
        for iteration in [0, 1, 2]:
            iter_grp = f.create_group(f"data/{iteration}")
            write_iteration_attributes(iter_grp)
            write_particle_group(iter_grp, "electron", 10)

        # Create non-matching groups that should be ignored
        f.create_group("data/metadata")
        f.create_group("data/config")
        f.create_group("data/not_a_number")
        f.create_group("other")

    print(f"make_group_based_non_matching_groups: Created {fname}")


def make_iteration_format_with_prefix_suffix(fname: str):
    """Test iterationFormats with prefix and suffix around %T"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(
            f, base_path="/data/step_%T_final/", iteration_encoding="groupBased"
        )

        # Create iterations with prefix and suffix
        for iteration in [0, 1, 2]:
            iter_grp = f.create_group(f"data/step_{iteration}_final")
            write_iteration_attributes(iter_grp)
            write_particle_group(iter_grp, "electron", 10)

    print(f"make_iteration_format_with_prefix_suffix: Created {fname}")


def make_group_based_complex_pattern(fname: str):
    """Group-based series with complex pattern (multiple parts)"""
    with h5py.File(fname, "w") as f:
        write_openpmd_header(
            f, base_path="/simulations/run_%T_data/", iteration_encoding="groupBased"
        )

        # Create iterations with complex pattern
        for iteration in [0, 10, 100]:
            iter_grp = f.create_group(f"simulations/run_{iteration}_data")
            write_iteration_attributes(iter_grp, time=iteration * 1.0e-14)
            write_particle_group(iter_grp, "electron", 10)

    print(f"make_group_based_complex_pattern: Created {fname}")


# ============================================================================
# Test file generators - All Optional Metadata
# ============================================================================


def make_valid_all_metadata(fname: str):
    """Valid file with all optional metadata attributes defined with known values"""
    with h5py.File(fname, "w") as f:
        # Write base header
        write_openpmd_header(f)

        # Add all optional metadata with known test values
        f.attrs["meshesPath"] = "meshes/"
        f.attrs["author"] = "Test Author <test@example.com>"
        f.attrs["date"] = "2025-12-29 10:30:00 +0000"
        f.attrs["softwareDependencies"] = "gcc@11.2.0;boost@1.76.0;hdf5@1.10.7"
        f.attrs["machine"] = "test-cluster-node42"
        f.attrs["comment"] = "Test file with all optional metadata"

        # Create iteration and particle data
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)
        write_particle_group(iter_grp, "electron", 10)

    print(f"make_valid_all_metadata: Created {fname}")


def make_valid_partial_optional_fields(fname: str):
    """Valid file with only some optional fields present for testing ParticleGroupReadInfo"""
    with h5py.File(fname, "w") as f:
        # Write base header
        write_openpmd_header(f)

        # Create iteration
        iter_grp = f.create_group("data/0")
        write_iteration_attributes(iter_grp)

        # Manually write particle group with only some optional fields
        particles_grp = iter_grp.create_group("particles")
        species_grp = particles_grp.create_group("electron")

        num_particles = 10
        species_grp.attrs["numParticles"] = np.int64(num_particles)
        species_grp.attrs["speciesType"] = "electron"

        # Write required position components
        pos_dim = np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        for comp in ["x", "y", "z"]:
            path = f"position/{comp}"
            data = np.array(
                [get_test_value(path, i, constant=False) for i in range(num_particles)],
                dtype=np.float64,
            )
            write_record(species_grp, path, data, unit_si=1.0, unit_dimension=pos_dim)

        # Write momentum (PRESENT)
        mom_dim = np.array([1.0, 1.0, -1.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        for comp in ["x", "y", "z"]:
            path = f"momentum/{comp}"
            data = np.array(
                [get_test_value(path, i, constant=False) for i in range(num_particles)],
                dtype=np.float64,
            )
            write_record(
                species_grp, path, data, unit_si=eV_c_to_SI, unit_dimension=mom_dim
            )

        # Write time (PRESENT)
        time_dim = np.array([0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float64)
        data = np.array(
            [get_test_value("time", i, constant=False) for i in range(num_particles)],
            dtype=np.float64,
        )
        write_record(species_grp, "time", data, unit_si=1.0, unit_dimension=time_dim)

        # Write id (PRESENT)
        data = np.array(
            [
                int(get_test_value("id", i, constant=False))
                for i in range(num_particles)
            ],
            dtype=np.int64,
        )
        write_record(species_grp, "id", data, unit_si=1.0, unit_dimension=None)

        # DO NOT write weight (ABSENT)
        # DO NOT write particleStatus (ABSENT)

    print(f"make_valid_partial_optional_fields: Created {fname}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate test data files for Parcel library"
    )
    parser.add_argument(
        "data_dir",
        type=str,
        nargs="?",
        default=str(Path(__file__).parent.parent / "tests" / "data"),
        help="Directory to write test data files (default: tests/data)",
    )
    args = parser.parse_args()

    test_data_dir = Path(args.data_dir)
    test_data_dir.mkdir(parents=True, exist_ok=True)

    print(f"Writing test data to: {test_data_dir.absolute()}")
    print("\n" + "=" * 80)
    print("OpenPMD Root-Level Metadata Issues")
    print("=" * 80)

    make_missing_openpmd_attr(str(test_data_dir / "missing_openpmd_attr.h5"))
    make_wrong_version_format(str(test_data_dir / "wrong_version_format.h5"))
    make_unsupported_version(str(test_data_dir / "unsupported_version.h5"))
    make_missing_basepath(str(test_data_dir / "missing_basepath.h5"))
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
    print("Missing/Invalid Attributes")
    print("=" * 80)

    make_missing_num_particles(str(test_data_dir / "missing_num_particles.h5"))
    make_num_particles_wrong_type(str(test_data_dir / "num_particles_wrong_type.h5"))
    make_num_particles_zero(str(test_data_dir / "num_particles_zero.h5"))
    make_num_particles_negative(str(test_data_dir / "num_particles_negative.h5"))
    make_num_particles_one(str(test_data_dir / "num_particles_one.h5"))
    make_missing_species_type(str(test_data_dir / "missing_species_type.h5"))
    make_species_type_wrong_type(str(test_data_dir / "species_type_wrong_type.h5"))
    make_position_is_dataset(str(test_data_dir / "position_is_dataset.h5"))
    make_momentum_group_empty(str(test_data_dir / "momentum_group_empty.h5"))
    make_position_x_wrong_rank(str(test_data_dir / "position_x_wrong_rank.h5"))

    print("\n" + "=" * 80)
    print("Array Size Mismatches")
    print("=" * 80)

    make_dataset_larger_than_num_particles(
        str(test_data_dir / "dataset_larger_than_num_particles.h5")
    )
    make_dataset_smaller_than_num_particles(
        str(test_data_dir / "dataset_smaller_than_num_particles.h5")
    )
    make_dataset_size_zero(str(test_data_dir / "dataset_size_zero.h5"))
    make_position_components_different_sizes(
        str(test_data_dir / "position_components_different_sizes.h5")
    )
    make_optional_fields_different_length(
        str(test_data_dir / "optional_fields_different_length.h5")
    )

    print("\n" + "=" * 80)
    print("Invalid Constant Records")
    print("=" * 80)

    make_constant_record_missing_value(
        str(test_data_dir / "constant_record_missing_value.h5")
    )
    make_constant_record_wrong_type_value(
        str(test_data_dir / "constant_record_wrong_type_value.h5")
    )
    make_both_dataset_and_constant(str(test_data_dir / "both_dataset_and_constant.h5"))
    make_constant_value_is_array(str(test_data_dir / "constant_value_is_array.h5"))

    print("\n" + "=" * 80)
    print("Unit Conversion (unitSI)")
    print("=" * 80)

    make_position_non_si_units(str(test_data_dir / "position_non_si_units.h5"))
    make_momentum_non_si_units(str(test_data_dir / "momentum_non_si_units.h5"))
    make_time_non_si_units(str(test_data_dir / "time_non_si_units.h5"))
    make_missing_unitsi(str(test_data_dir / "missing_unitsi.h5"))
    make_unitsi_wrong_type(str(test_data_dir / "unitsi_wrong_type.h5"))

    print("\n" + "=" * 80)
    print("Valid Files")
    print("=" * 80)

    make_valid_constant_records(str(test_data_dir / "valid_constant_records.h5"))
    make_valid_dataset_records(str(test_data_dir / "valid_dataset_records.h5"))
    make_valid_non_default_particles_path(
        str(test_data_dir / "valid_non_default_particles_path.h5")
    )
    make_valid_non_default_base_path(
        str(test_data_dir / "valid_non_default_base_path.h5")
    )
    make_valid_multiple_iterations(str(test_data_dir / "valid_multiple_iterations.h5"))
    make_attr_count(str(test_data_dir / "attr_count_32.h5"), 32)
    make_valid_partial_optional_fields(
        str(test_data_dir / "valid_partial_optional_fields.h5")
    )

    print("\n" + "=" * 80)
    print("Series Tests")
    print("=" * 80)

    make_valid_file_based_series(str(test_data_dir / "file_based_series"))
    make_file_based_series_with_other_files(
        str(test_data_dir / "file_based_series_with_other")
    )
    make_file_based_multiple_percent_t(
        str(test_data_dir / "file_based_multiple_percent_t")
    )
    make_group_based_non_matching_groups(
        str(test_data_dir / "group_based_non_matching_groups.h5")
    )
    make_iteration_format_with_prefix_suffix(
        str(test_data_dir / "iteration_format_prefix_suffix.h5")
    )
    make_group_based_complex_pattern(
        str(test_data_dir / "group_based_complex_pattern.h5")
    )

    print("\n" + "=" * 80)
    print("Valid file with all optional metadata")
    print("=" * 80)

    make_valid_all_metadata(str(test_data_dir / "valid_all_metadata.h5"))

    print("\n" + "=" * 80)
    print("Valid files using pmd_beamphysics library")
    print("=" * 80)

    make_pmd_beamphysics_constant(str(test_data_dir / "pmd_beamphysics_constant.h5"))
    make_pmd_beamphysics_dataset(str(test_data_dir / "pmd_beamphysics_dataset.h5"))

    print("\nAll test files created successfully!")
