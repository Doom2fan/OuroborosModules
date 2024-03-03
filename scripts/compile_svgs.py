#!/usr/bin/env python3

# Copyright (C) 2024 Chronos "phantombeta" Ouroboros
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

import os
import glob
import shutil
import subprocess

SVG_PATH_SRC = "res_src"
SVG_PATH_DST = "res"
INKSCAPE_ARGS = [
    "--without-gui", "--batch-process",
    "--actions='select-by-id:Widgets;selection-hide;select-all;"
    "selection-ungroup;select-all;clone-unlink-recursively;select-all;"
    "object-to-path;export-text-to-path;export-overwrite;export-do;'",
]


def checkSrcFileNewer(pathSrc, pathDst):
    if not os.path.isfile(pathSrc):
        return None
    if not os.path.isfile(pathDst):
        return True

    return os.path.getmtime(pathSrc) >= os.path.getmtime(pathDst)


def main():
    inkscapePath = os.environ.get('INKSCAPE_PATH')
    if inkscapePath is None:
        print("$INKSCAPE_PATH must be set to the inkscape EXE.")
        return

    srcDir = os.path.abspath(SVG_PATH_SRC)
    dstDir = os.path.abspath(SVG_PATH_DST)

    newerFiles = []
    for fileName in glob.iglob("**/*.svg", root_dir=srcDir, recursive=True):
        filePathSrc = os.path.join(srcDir, fileName)
        filePathDst = os.path.join(dstDir, fileName)

        if not os.path.isfile(filePathSrc):
            continue
        if not checkSrcFileNewer(filePathSrc, filePathDst):
            continue

        fileDstDir = os.path.dirname(filePathDst)
        if not os.path.isdir(fileDstDir):
            os.makedirs (fileDstDir)

        shutil.copyfile(filePathSrc, filePathDst)
        newerFiles.append(filePathDst)

    if len(newerFiles) < 1:
        return

    argsList = INKSCAPE_ARGS.copy()
    argsList.insert(0, inkscapePath)
    argsList.extend(newerFiles)
    subprocess.run(argsList)


if __name__ == '__main__':
    main()
