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
import shutil
import subprocess
import sys

from pathlib import Path
if __name__ == '__main__':
    from common import *
else:
    from .common import *

SETTINGS_BACKUP_NAME = f"{VCV_SETTINGS_FILE}_bak"
DEFAULTS_OVERRIDE_BACKUP_NAME = f"{MODULE_DEFAULTS_OVERRIDE_FILE}_bak"
SCREENSHOTS_DEFAULTS_OVERRIDE_FILE = "screenshots_config_default.json"
def make_docs_images(globalData):
    if globalData.rackPath is None:
        print(VCV_NOT_FOUND_ERROR_MSG)
        return

    newDefaultsOverridePath = (globalData.repoDir / SCRIPTS_DIR / SCREENSHOTS_DEFAULTS_OVERRIDE_FILE).resolve()
    if not newDefaultsOverridePath.is_file():
        print(f"error: file '{SCREENSHOTS_DEFAULTS_OVERRIDE_FILE}' not found")
        return

    print("deleting old screenshots")
    screenshotsPath = (globalData.rackSdkDir / VCV_SCREENSHOTS_DIR / MODULE_SLUG).resolve()
    if screenshotsPath.is_dir():
        for fileName in glob.iglob("*.png", root_dir=screenshotsPath, recursive=False):
            (screenshotsPath / fileName).unlink(missing_ok=True)

    vcvConfigPath = (globalData.rackSdkDir / VCV_SETTINGS_FILE).resolve()
    vcvConfigBackupPath = globalData.rackSdkDir / SETTINGS_BACKUP_NAME
    vcvConfigExists = vcvConfigPath.is_file()

    defaultsOverridePath = (globalData.rackSdkDir / MODULE_DEFAULTS_OVERRIDE_FILE).resolve()
    defaultsOverrideBackupPath = globalData.rackSdkDir / DEFAULTS_OVERRIDE_BACKUP_NAME
    defaultsOverrideExists = defaultsOverridePath.is_file()

    if vcvConfigExists:
        print("backing up VCV settings file")
        vcvConfigPath.replace(vcvConfigBackupPath)
    if defaultsOverrideExists:
        print("backing up defaults override file")
        defaultsOverridePath.replace(defaultsOverrideBackupPath)

    print("copying new defaults override file")
    shutil.copyfile(newDefaultsOverridePath, defaultsOverridePath)

    print("launching VCV to generate screenshots")
    vcvArgs = [globalData.rackPath, "-d", "-t 2"]
    subprocess.run(vcvArgs, cwd=globalData.rackSdkDir)

    if screenshotsPath.is_dir():
        print(f"copying screenshots to '{DOCS_SCREENSHOTS_DIR}'")

        docsScreenshotsPath = (globalData.repoDir / DOCS_SCREENSHOTS_DIR).resolve()
        docsScreenshotsPath.mkdir(parents=True, exist_ok=True)

        for fileName in glob.iglob("*.png", root_dir=screenshotsPath, recursive=False):
            (screenshotsPath / fileName).replace(docsScreenshotsPath / fileName)
    else:
        print("error: could not find screenshots")

    vcvConfigPath.unlink(missing_ok=True)
    if vcvConfigExists:
        print("restoring VCV settings file")
        vcvConfigBackupPath.replace(vcvConfigPath)
    defaultsOverridePath.unlink(missing_ok=True)
    if defaultsOverrideExists:
        print("restoring defaults override file")
        defaultsOverrideBackupPath.replace(defaultsOverridePath)

if __name__ == '__main__':
    make_docs_images(GlobalData())
