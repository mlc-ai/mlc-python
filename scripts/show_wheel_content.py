#!/usr/bin/env python3
import sys
from pathlib import Path
from zipfile import ZipFile


def main() -> None:
    directory = Path(sys.argv[1])
    # for each file in the directory that ends with `.whl`
    for file in directory.iterdir():
        if file.suffix == ".whl":
            # print the name of the file
            print(f"============= Wheel: {file} =============")
            # open the file as a zip file
            with ZipFile(file) as z:
                # print the names of the files in the zip file
                print(*z.namelist(), sep="\n")


if __name__ == "__main__":
    main()
