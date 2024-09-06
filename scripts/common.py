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
import platform
import shutil

from enum import Enum
from pathlib import Path

# Module paths & data
MODULE_SLUG = "OuroborosModules"
MODULE_DEFAULTS_OVERRIDE_FILE = "OuroborosModules_Default.json"
SVG_PATH_SRC = "res_src"
SVG_PATH_DST = "res"
SCRIPTS_DIR = "scripts"
DOCS_SCREENSHOTS_DIR = "docs/images/modules"

# VCV paths
VCV_SETTINGS_FILE = "settings.json"
VCV_SCREENSHOTS_DIR = "screenshots"

# Error messages
VCV_NOT_FOUND_ERROR_MSG = "error: VCV not found in RACK_DIR."

class SystemOS(Enum):
    Windows = 1
    Linux = 2,
    MacOS = 3,

class GlobalData:
    def __init__(self):
        self.curOS = getOS()
        self.repoDir = getRepoPath()
        self.rackSdkDir = Path(self.repoDir, os.environ.get("RACK_DIR", "../..")).resolve()
        self.cmakePath = shutil.which("cmake")
        self.cmakeBuild = "dep/cmake-build"
        self.threadCount = os.cpu_count()
        self.inkscapePath = os.environ.get("INKSCAPE_PATH")

        rackFilename = "Rack"
        if self.curOS == SystemOS.Windows:
            rackFilename = "Rack.exe"
        self.rackPath = Path(self.rackSdkDir, rackFilename).resolve()
        if not os.path.isfile(self.rackPath):
            self.rackPath = None

def getOS():
    sysString = platform.system().casefold()
    if "mingw32".casefold() in sysString:
        return SystemOS.Windows
    if "cygwin".casefold() in sysString:
        return SystemOS.Windows
    if "windows".casefold() in sysString:
        return SystemOS.Windows

    if "darwin".casefold() in sysString:
        return SystemOS.MacOS

    return SystemOS.Linux

def getRepoPath():
    curPath = Path(__file__).resolve().parent
    while not (curPath / "__build_shell.py").is_file():
        if curPath == curPath.parent:
            raise Exception("Could not find repo base directory")
        curPath = curPath.parent

    return curPath

def checkSrcFileNewer(pathSrc, pathDst):
    if not os.path.isfile(pathSrc):
        return None
    if not os.path.isfile(pathDst):
        return True

    return os.path.getmtime(pathSrc) >= os.path.getmtime(pathDst)