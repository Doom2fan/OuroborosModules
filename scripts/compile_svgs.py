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

import glob
import os
import shutil
import subprocess
import sys

from pathlib import Path
if __name__ == '__main__':
    from common import *
else:
    from .common import *

INKSCAPE_ARGS = [
    "--without-gui", "--batch-process",
    "--actions='select-by-id:Widgets;selection-hide;select-all;"
    "selection-ungroup;select-all;clone-unlink-recursively;select-all;"
    "object-to-path;export-text-to-path;export-overwrite;export-do;'",
]

def compile_svgs(globalData):
    if globalData.inkscapePath is None:
        print("error: INKSCAPE_PATH must be set to the inkscape EXE.")
        return

    srcDir = Path(globalData.repoDir, SVG_PATH_SRC).resolve()
    dstDir = Path(globalData.repoDir, SVG_PATH_DST).resolve()

    newerFiles = []
    for fileName in glob.iglob("**/*.svg", root_dir=srcDir, recursive=True):
        filePathSrc = (srcDir / fileName).resolve()
        filePathDst = (dstDir / fileName).resolve()

        if not os.path.isfile(filePathSrc):
            continue
        if not checkSrcFileNewer(filePathSrc, filePathDst):
            continue

        fileDstDir = filePathDst.parent
        if not fileDstDir.is_dir():
            fileDstDir.mkDir(parents=True, exist_ok=True)

        shutil.copyfile(filePathSrc, filePathDst)
        newerFiles.append(str(filePathDst))

    if len(newerFiles) < 1:
        return

    argsList = INKSCAPE_ARGS.copy()
    argsList.insert(0, globalData.inkscapePath)
    argsList.extend(newerFiles)
    subprocess.run(argsList)

if __name__ == '__main__':
    compile_svgs(GlobalData())
