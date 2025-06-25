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

import argparse
import cmd2
import scripts.compile_svgs as compile_svgs
import scripts.make_docs_images as make_docs_images
import shutil
import subprocess
import sys

from scripts.common import *

CMAKE_BUILD = "dep/cmake-build"

class CmdShell(cmd2.Cmd):
    intro = "Type help or ? to list commands.\n"
    prompt = "> "

    def __init__(self):
        super().__init__(allow_cli_args=False)
        self.globalData = GlobalData()

    runCmd_parser = cmd2.Cmd2ArgumentParser()
    runCmd_LogGroup = runCmd_parser.add_mutually_exclusive_group()
    runCmd_LogGroup.add_argument("-l", "--logfile", default = None, help = "When passed, logs VCV Rack's output to the specified file")
    runCmd_LogGroup.add_argument("-s", "--stdout", action = "store_true", help = "When passed, logs VCV Rack's output to stdout")
    @cmd2.with_argparser(runCmd_parser)
    def do_run(self, args):
        'Runs VCV Rack in development mode'
        if self.globalData.rackPath is None:
            print(VCV_NOT_FOUND_ERROR_MSG)
            return

        print("starting VCV Rack in development mode")
        process = subprocess.Popen(
            [self.globalData.rackPath, "-d"],
            cwd = self.globalData.rackSdkDir,
            text = True,
            stdout = subprocess.PIPE,
            stderr = subprocess.STDOUT
        )
        if args.logfile is not None:
            with open(args.logfile, "w") as outFile:
                while process.poll() is None:
                    outFile.write(process.stdout.readline())
        elif args.stdout and process.stdout is not None:
            while process.poll() is None:
                self.stdout.write(process.stdout.readline())

    def do_build(self, argStr):
        'Builds the plugin: build [release|debug]'
        argStr = argStr.strip()
        match argStr:
            case "release" | "":
                buildType = BuildType.Release
            case "debug":
                buildType = BuildType.Debug

            case _:
                print(f"error: expected 'release' or 'debug', got '{argStr}'")
                return

        globalData = self.globalData
        buildInfo = BuildInfo(globalData, buildType)

        if globalData.cmakePath is None:
            print("error: CMake could not be found")
            return

        print("generating CMake build files")
        cmakeResult = subprocess.run([
                globalData.cmakePath, "-B", buildInfo.dir, f"-DRACK_SDK_DIR={globalData.rackSdkDir}",
                f"-DCMAKE_BUILD_TYPE={buildInfo.cmakeType}", f"-DCMAKE_INSTALL_PREFIX={buildInfo.dir}/dist",
                buildInfo.dir
            ],
            cwd = globalData.repoDir
        )
        if cmakeResult.returncode != 0:
            print(f"failed to generate build files: CMake failed with exit code {cmakeResult.returncode}")
            return

        print("building plugin")
        cmakeResult = subprocess.run([globalData.cmakePath, "--build", buildInfo.dir, "--", "-j", str(globalData.threadCount)], cwd = globalData.repoDir)
        if cmakeResult.returncode != 0:
            print(f"failed to generate build files: CMake failed with exit code {cmakeResult.returncode}")
            return

        print("installing plugin files")
        cmakeResult = subprocess.run([globalData.cmakePath, "--install", buildInfo.dir], cwd = globalData.repoDir)
        if cmakeResult.returncode != 0:
            print(f"failed to generate build files: CMake failed with exit code {cmakeResult.returncode}")
            return

        try:
            print(f"copying '{COMPILE_DATABASE}' to repo root")
            shutil.copyfile(
                Path(globalData.repoDir, buildInfo.dir, COMPILE_DATABASE).resolve(),
                Path(globalData.repoDir, COMPILE_DATABASE).resolve()
            )
            print("copying plugin file to repo root")
            shutil.copyfile(
                Path(globalData.repoDir, buildInfo.dir, buildInfo.pluginDll).resolve(),
                Path(globalData.repoDir, buildInfo.pluginDll).resolve()
            )
        except (shutil.SameFileError, OSError) as err:
            print(f"error: {str(err)}")

    cppcheck_parser = cmd2.Cmd2ArgumentParser()
    cppcheck_parser.add_argument("-f", "--force", action = "store_true", help = "Passes --force to cppcheck")
    cppcheck_parser.add_argument("--max-config", dest = "maxConfig", default = None, type = int, help = "Passes --max-config <N> to cppcheck")
    cppcheck_parser.add_argument("-i", "--inconclusive", action = "store_true", help = "Passes --inconclusive to cppcheck")
    # cppcheck_parser.add_argument("-r", "--html", dest = "asHtml", action = "store_true", help = "Generate report as HTML")
    cppcheck_parser.add_argument("-o", "--output-file", dest = "outFile", default = None, help = "The file to output to")
    cppcheck_parser.add_argument("-j", dest = "coreCount", default = None, type = int, help = "How many cores cppcheck should use")
    cppcheck_parser.add_argument("-b", "--build", choices = [ "debug", "release" ], default = "debug", help = "The type of build")
    @cmd2.with_argparser(cppcheck_parser)
    def do_cppcheck(self, args):
        'Runs cppcheck based on compile_commands.json'
        match args.build:
            case "debug" | "":
                buildType = BuildType.Debug
            case "release":
                buildType = BuildType.Release

        globalData = self.globalData
        buildInfo = BuildInfo(globalData, buildType)

        if globalData.cppcheckPath is None:
            print("error: cppcheck could not be found")
            return
        # if args.asHtml and globalData.cppcheckHTMLPath is None:
        #     print(f"error: cppcheck-htmlreport could not be found")
        #     return
        # if args.asHtml and args.outFile is not None:
        #     print(f"error: -r/--html cannot be used together with -o/--output-file")
        #     return
        if args.force and args.maxConfig is not None:
            print(f"error: -f/--force cannot be used together with --max-config")
            return

        cppcheckArgs = [
            globalData.cppcheckPath,
            f"--project={COMPILE_DATABASE}",
            f"--cppcheck-build-dir={buildInfo.cppcheckDir}", "--std=c++17",
            f"--relative-paths={globalData.rackSdkDir};{globalData.repoDir}",
            "--error-exitcode=1",
            "--suppressions-list=CppCheckSuppressions.txt", "--inline-suppr",
            "--enable=warning,portability,missingInclude",
            "--check-level=exhaustive",
        ]
        if args.inconclusive:
            cppcheckArgs.append("--inconclusive")
        if args.force:
            cppcheckArgs.append("--force")
        if args.maxConfig is not None:
            cppcheckArgs.append(f"--max-configs={args.maxConfig}")
        if args.coreCount is not None:
            cppcheckArgs.append(f"-j{args.coreCount}")
        if args.outFile is not None:
            cppcheckArgs.append(f"--output-file={args.outFile}")
        # xmlReportPath = f"{buildInfo.cppcheckDir}/__XMLReport.xml"
        # if args.asHtml:
        #     cppcheckArgs.append(f"--output-file={xmlReportPath}")

        print(f"running {' '.join(cppcheckArgs)}")
        Path(globalData.repoDir, buildInfo.cppcheckDir).resolve().mkdir(parents = True, exist_ok = True)
        cppcheckResult = subprocess.run(cppcheckArgs, cwd = globalData.repoDir)

        # if args.asHtml:
        #     htmlreportDir = f"{buildInfo.cppcheckDir}/HTMLReport"
        #     htmlreportArgs = [
        #         globalData.cppcheckHTMLPath,
        #         f"--file={xmlReportPath}",
        #         f"--report-dir={htmlreportDir}",
        #     ]
        #     print(f"running {' '.join(htmlreportArgs)}")
        #     Path(globalData.repoDir, htmlreportDir).resolve().mkdir(parents = True, exist_ok = True)
        #     subprocess.run(htmlreportArgs, cwd = globalData.repoDir)

    def do_svg(self, args):
        'Processes the SVG files'
        try:
            compile_svgs.compile_svgs(self.globalData)
        except Exception as err:
            print(f"error: {str(err)}")

    def do_make_images(self, args):
        'Generates the documentation\'s images'
        try:
            make_docs_images.make_docs_images(self.globalData)
        except Exception as err:
            print(f"error: {str(err)}")

def main():
    if len(sys.argv) > 1:
        try:
            CmdShell().onecmd(' '.join(sys.argv[1:]))
        except (cmd2.exceptions.Cmd2ArgparseError, cmd2.exceptions.PassThroughException, cmd2.exceptions.SkipPostcommandHooks):
            return
    else:
        CmdShell().cmdloop()

if __name__ == '__main__':
    main()
