import pytest
from mlc import config as cfg
from mlc._cython import SYSTEM


def test_includedir() -> None:
    (include_dir,) = cfg.includedir()
    assert include_dir.exists()
    assert (include_dir / "mlc").exists()
    assert (include_dir / "dlpack").exists()
    assert (include_dir / "mlc" / "backtrace").exists()


def test_libdir() -> None:
    libdir = cfg.libdir()
    assert libdir.exists()
    if SYSTEM == "Windows":
        assert (libdir / "libmlc.dll").exists()
        assert (libdir / "libmlc-static.lib").exists()
    elif SYSTEM == "Darwin":
        assert (libdir / "libmlc.dylib").exists()
        assert (libdir / "libmlc-static.a").exists()
    else:
        assert (libdir / "libmlc.so").exists()
        assert (libdir / "libmlc-static.a").exists()


@pytest.mark.xfail(
    condition=SYSTEM == "Windows",
    reason="`vcvarsall.bat` not found for some reason",
)
def test_probe_compiler() -> None:
    compilers = cfg.probe_compiler()
    for compiler in compilers:
        assert compiler.exists()
    if SYSTEM == "Windows":
        print("Compilers found: ", ";".join(str(i) for i in compilers))
        print("vcvarsall.bat found: ", cfg.probe_vcvarsall())
