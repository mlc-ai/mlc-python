import argparse
import os
import platform
import shutil
import warnings
from pathlib import Path

from mlc._cython import LIB_PATH, SYSTEM


def includedir() -> tuple[Path, ...]:
    path = Path(__file__).parent
    if path.parent.name == "python":
        path = path.parent.parent
    else:
        path = path.parent
    mlc_include = path / "include"
    dlpack_include = path / "3rdparty" / "dlpack" / "include"
    return mlc_include.resolve(), dlpack_include.resolve()


def libdir() -> Path:
    return LIB_PATH.parent.resolve()


def probe_vcvarsall() -> Path:
    for path in probe_msvc():
        cur = path
        while cur.parent != cur:
            if cur.name == "VC":
                break
            cur = cur.parent
        else:
            continue
        vcvarsall = cur / "Auxiliary" / "Build" / "vcvarsall.bat"
        if vcvarsall.exists():
            return vcvarsall.resolve()
    raise RuntimeError("vcvarsall.bat not found")


def probe_msvc() -> tuple[Path, ...]:
    import setuptools  # type: ignore[import-not-found,import-untyped]

    results = []
    if (path := shutil.which("cl.exe", mode=os.X_OK)) is not None:
        results.append(Path(path).resolve())

    try:
        vctools = setuptools.msvc.EnvironmentInfo(platform.machine()).VCTools
    except Exception:
        pass
    else:
        for vctool in vctools:
            if (cl_exe := Path(vctool) / "cl.exe").exists():
                results.append(cl_exe.resolve())
    return tuple(dict.fromkeys(results))


def probe_compiler() -> tuple[Path, ...]:
    results = []
    if compiler := os.environ.get("CXX") or os.environ.get("CC"):
        results.append(Path(compiler).resolve())
    if SYSTEM == "Windows":
        return probe_msvc()
    else:
        for compiler in ["g++", "gcc", "clang++", "clang", "c++", "cc"]:
            if (path := shutil.which(compiler, mode=os.X_OK)) is not None:
                results.append(Path(path).resolve())
    if not results:
        warnings.warn("No compiler found. Set environment variable `CXX` to override")
    return tuple(dict.fromkeys(results))


def main() -> None:
    parser = argparse.ArgumentParser(description="MLC Config Tool")
    parser.add_argument("--includedir", action="store_true", help="Print the include directory")
    parser.add_argument("--libdir", action="store_true", help="Print the library directory")
    parser.add_argument("--probe-compiler", action="store_true", help="Probe the compiler")
    parser.add_argument("--probe-msvc", action="store_true", help="Probe MSVC")
    parser.add_argument("--probe-vcvarsall", action="store_true", help="Probe vcvarsall.bat")

    def _tuple_path_to_str(paths: tuple[Path, ...]) -> str:
        return ";".join(str(path) for path in paths)

    args = parser.parse_args()
    has_action = False
    if args.includedir:
        has_action = True
        print(_tuple_path_to_str(includedir()))
    if args.libdir:
        has_action = True
        print(libdir())
    if args.probe_compiler:
        has_action = True
        print(_tuple_path_to_str(probe_compiler()))
    if args.probe_msvc:
        has_action = True
        print(_tuple_path_to_str(probe_msvc()))
    if args.probe_vcvarsall:
        has_action = True
        print(probe_vcvarsall())
    if not has_action:
        parser.print_help()


if __name__ == "__main__":
    main()
