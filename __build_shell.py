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

import cmd
import scripts.compile_svgs as compile_svgs
import shutil
import subprocess
import sys

from scripts.common import *

CMAKE_BUILD = "dep/cmake-build"

class CmdShell(cmd.Cmd):
    intro = "Type help or ? to list commands.\n"
    prompt = "> "

    def __init__(self):
        cmd.Cmd.__init__(self)
        self.globalData = GlobalData()

    def do_run(self, args):
        'Runs VCV in development mode'
        if self.globalData.rackPath is None:
            print(VCV_NOT_FOUND_ERROR_MSG)
            return

        subprocess.run([self.globalData.rackPath, "-d"], cwd=self.globalData.rackSdkDir)

    def do_build(self, argStr):
        'Builds the plugin: build [release|debug]'
        argStr = argStr.strip()
        buildType = "Release"
        match argStr:
            case "release" | "":
                buildType = "Release"
            case "debug":
                buildType = "Debug"

            case _:
                print(f"error: Expected 'release' or 'debug', got '{argStr}'.")
                return

        globalData = self.globalData
        cmakeBuild = f"{globalData.cmakeBuild}_{buildType}"

        if globalData.cmakePath is None:
            print("error: CMake could not be found.")
            return

        pluginDll = "plugin.so"
        match globalData.curOS:
            case SystemOS.Windows:
                pluginDLL = "plugin.dll"
            case SystemOS.MacOS:
                pluginDLL = "plugin.dylib"
            case SystemOS.Linux | _:
                pluginDLL = "plugin.so"

        subprocess.run(
            [
                globalData.cmakePath, "-B", cmakeBuild, f"-DRACK_SDK_DIR={globalData.rackSdkDir}",
                f"-DCMAKE_BUILD_TYPE={buildType}", f"-DCMAKE_INSTALL_PREFIX={cmakeBuild}/dist", cmakeBuild
            ],
            cwd = globalData.repoDir
        )
        subprocess.run([globalData.cmakePath, "--build", cmakeBuild, "--", "-j", str(globalData.threadCount)], cwd=globalData.repoDir)
        subprocess.run([globalData.cmakePath, "--install", cmakeBuild], cwd=globalData.repoDir)

        try:
            shutil.copyfile(
                Path(globalData.repoDir, cmakeBuild, "compile_commands.json").resolve(),
                Path(globalData.repoDir, "compile_commands.json").resolve()
            )
            shutil.copyfile(
                Path(globalData.repoDir, cmakeBuild, pluginDLL).resolve(),
                Path(globalData.repoDir, pluginDLL).resolve()
            )
        except (shutil.SameFileError, OSError) as err:
            print(f"error: {str(err)}")

    def do_svg(self, args):
        'Processes the SVG files'
        try:
            compile_svgs.compile_svgs(self.globalData)
        except Exception as err:
            print(f"error: {str(err)}")

    def do_exit(self, args):
        'Exits the shell'
        return True

    def do_EOF(self, line):
        return True

def main():
    if len(sys.argv) > 1:
        CmdShell().onecmd(' '.join(sys.argv[1:]))
    else:
        CmdShell().cmdloop()

if __name__ == '__main__':
    main()
