from __future__ import annotations

import os
import re
import shlex
import shutil
import subprocess
import tempfile
import warnings
from collections.abc import Mapping, Sequence
from pathlib import Path
from types import MappingProxyType
from typing import Any

from mlc import config as mlc_config
from mlc._cython import SYSTEM

DEFAULT_OPTIONS: Mapping[str, Sequence[str]] = {
    "Linux": (
        "-O2",
        "-g",
        "-shared",
        "-fPIC",
        "-std=c++17",
        "-DMLC_COMPILATION=1",
        "-fvisibility=hidden",
    ),
    "Darwin": (
        "-O2",
        "-g",
        "-shared",
        "-fPIC",
        "-std=c++17",
        "-DMLC_COMPILATION=1",
        "-undefined",
        "dynamic_lookup",
        "-fvisibility=hidden",
    ),
    "Windows": (
        "/O2",
        "/Zi",  # Debug information
        "/LD",  # Create DLL
        "/EHsc",  # Enable C++ exceptions
        "/std:c++17",
        "/DNDEBUG",  # No debug
        "/D_WINDLL",  # Windows DLL
        "/D_MBCS",  # Multi-byte character set
        "/DMLC_COMPILATION=1",
    ),
}


def create_shared(
    sources: str | Path | Sequence[Path | str],
    output: str | Path,
    options: Mapping[str, Sequence[str]] = MappingProxyType(DEFAULT_OPTIONS),
) -> None:
    if not isinstance(options, Sequence):
        platform_options = options[SYSTEM]
    else:
        platform_options = options
    with tempfile.TemporaryDirectory() as temp_dir_str:
        work_dir = Path(temp_dir_str).resolve()
        if SYSTEM in ["Linux", "Darwin"]:
            _linux_compile(
                sources=_normalize_sources(sources, work_dir=work_dir),
                output=Path(output).resolve(),
                work_dir=work_dir,
                options=platform_options,
            )
        elif SYSTEM == "Windows":
            _windows_compile(
                sources=_normalize_sources(sources, work_dir=work_dir),
                output=Path(output).resolve(),
                work_dir=work_dir,
                options=platform_options,
            )
        else:
            raise NotImplementedError(f"Unsupported platform: {SYSTEM}")


def _linux_compile(
    sources: list[Path],
    output: Path,
    work_dir: Path,
    options: Sequence[str],
) -> None:
    temp_output = "main.so"
    cmd: list[str] = [
        str(mlc_config.probe_compiler()[0]),
        *options,
        "-o",
        str(temp_output),
    ]
    cmd.extend("-I" + str(path) for path in mlc_config.includedir())
    cmd.extend(str(source) for source in sources)

    exec_env: dict[str, Any] = os.environ.copy()
    if ccache := shutil.which("ccache", mode=os.X_OK):
        cmd.insert(0, ccache)
        exec_env["CCACHE_COMPILERCHECK"] = "mtime"
        exec_env["CCACHE_NOHASHDIR"] = "1"
        exec_env["CCACHE_BASEDIR"] = str(work_dir)
    else:
        warnings.warn("ccache not found")

    print(f"Executing command: {shlex.join(cmd)}")
    proc = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=str(work_dir),
        env=exec_env,
        text=True,
        check=False,
    )
    if proc.returncode != 0:
        msg = f"Compilation error (return code {proc.returncode}):\n{proc.stdout}"
        raise RuntimeError(msg)
    shutil.move(str(work_dir / temp_output), str(output))


def _windows_compile(
    sources: list[Path],
    output: Path,
    work_dir: Path,
    options: Sequence[str],
) -> None:
    def _escape(arg: str) -> str:  #  Escape an argument for the Windows command line.
        if not arg or re.search(r'(["\s])', arg):
            # Wrap in double quotes and escape double quotes inside
            return '"' + arg.replace('"', '""') + '"'
        return arg

    temp_output = work_dir / "main.dll"
    vcvarsall = mlc_config.probe_vcvarsall()
    arch = "x64" if os.environ.get("PROCESSOR_ARCHITECTURE") == "AMD64" else "x86"
    cmd: list[str] = [
        str(mlc_config.probe_compiler()[0]),
        *options,
        f"/Fe:{temp_output!s}",
        str(mlc_config.libdir() / "mlc_registry.lib"),
    ]
    cmd.extend(f"/I{path!s}" for path in mlc_config.includedir())
    cmd.extend(str(source) for source in sources)
    full_cmd = f'"{vcvarsall}" {arch} && {" ".join(_escape(arg) for arg in cmd)}'
    print(f"Executing command: {full_cmd}")
    proc = subprocess.run(
        full_cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=str(work_dir),
        text=True,
        check=False,
        shell=True,
    )
    if proc.returncode != 0:
        msg = f"Compilation error (return code {proc.returncode}):\n{proc.stdout}"
        raise RuntimeError(msg)
    shutil.move(str(temp_output), str(output))


def _normalize_sources(sources: str | Path | Sequence[Path | str], work_dir: Path) -> list[Path]:
    result: list[Path] = []
    if isinstance(sources, str) or not isinstance(sources, Sequence):
        sources = [sources]
    cnt = 0
    for source in sources:
        if isinstance(source, Path):
            result.append(source.resolve())
        elif Path(source).is_file():
            result.append(Path(source).resolve())
        else:
            with (work_dir / f"_mlc_source_{cnt}.cc").open("w", encoding="utf-8") as f:
                f.write(source)
            result.append(Path(f"_mlc_source_{cnt}.cc"))
            cnt += 1
    return result
