# Building
For all builds, the repo must be cloned with submodules.  
Use `git clone --recurse-submodules <URL>` for cloning the repo, or run `git submodule update --init --recursive` after cloning.  
As is standard for rack plugins, the `$RACK_DIR` environment variable must be set if you're not building from the `plugins` folder of a local VCV build.

* [Requirements](#requirements)
* [Local & development builds](#local-builds)
* [Library builds](#library-builds)

# Requirements
* CMake. Needs to be in $PATH.

# Local builds
There are two ways of building the plugin: A bash script, and an interactive shell written in Python with more features.

## Bash script (__build.sh)
No additonal requirements.

Arguments:
* -r/--release: Specifies a release build. Default.
* -d/--debug: Specifies a debug build.

## Interactive shell (__build_shell.py)
Additional requirements:
* Python 3
* [cmd2](https://github.com/python-cmd2/cmd2) must be installed via pip. (`pip install -U cmd2`)

This can be used to run cppcheck, generate the SVGs, and create the module screenshots for the manual.  
The available commands and their options can be listed by using `help` inside the shell.

# Library Builds
This will build the plugin as it would be in the library, and create a vcvplugin file in the `dist` folder.
Run `make dist` to build, or
```Bash
make dep
make dist
```