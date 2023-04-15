from mlc import config as cfg
from mlc._cython import SYSTEM


def test_includedir() -> None:
    mlc_include, dlpack_include = cfg.includedir()
    assert mlc_include.exists() and (mlc_include / "mlc").exists()
    assert dlpack_include.exists() and (dlpack_include / "dlpack").exists()


def test_libdir() -> None:
    libdir = cfg.libdir()
    assert libdir.exists()
    if SYSTEM == "Windows":
        assert (libdir / "libmlc_registry.dll").exists()
        assert (libdir / "libmlc_registry_static.lib").exists()
    elif SYSTEM == "Darwin":
        assert (libdir / "libmlc_registry.dylib").exists()
        assert (libdir / "libmlc_registry_static.a").exists()
    else:
        assert (libdir / "libmlc_registry.so").exists()
        assert (libdir / "libmlc_registry_static.a").exists()


def test_probe_compiler() -> None:
    compilers = cfg.probe_compiler()
    for compiler in compilers:
        assert compiler.exists()
    if SYSTEM == "Windows":
        print("Compilers found: ", ";".join(str(i) for i in compilers))
        print("vcvarsall.bat found: ", cfg.probe_vcvarsall())
