# How to setup your workstation to build with CMake.

This document describes how to setup your workstation to build the Google Cloud
C++ client libraries using CMake. The intended audience is developers of the
client librariues that want to verify their changes will work with CMake and/or
prefer to use CMake for whatever reason. The document assumes you are using a
Linux workstation running Ubuntu, changing the instructions for other
distributions or operating systems is left as an exercise to the reader, and
PRs to improve this document are most welcome!

Unless explicitly stated, this document assumes you running these commands at
the top-level directory of the project, as in:

```console
cd $HOME
git clone git@github:<github-username>/google-cloud-cpp.git
cd google-cloud-cpp
```

## Running the CI build

If you just want to run one of the CI builds based on CMake then just install
Docker (once):

```console
# Make sure the docker server and client are installed in your workstation, you
# only need to do this once:
sudo apt update && sudo apt install docker.io
```

and run the same script the CI build uses:

```console
cd $HOME/google-cloud-cpp
./ci/kokoro/build.sh clang-tidy
```

The build takes a few minutes the first time, as it needs to create a Docker
image with all the development tools and dependencies installed. Future builds
(with no changes to the source) take only a few seconds.

## Install the dependencies in `$HOME/local`.

This is the recommended way to develop `google-cloud-cpp` using CMake. You will
install the dependencies in `$HOME/local` (or a similar directory), and compile
the project against these libraries. The installation needs to be done every
time the version of the dependencies is changed, and it is not done
automatically. But once installed you can use them for any build.

Configure the super-build to install in `$HOME/local`:

```console
cmake -Hsuper -Bcmake-out/si \
    -DGOOGLE_CLOUD_CPP_EXTERNAL_PREFIX=$HOME/local-common -GNinja
```

Remove the `-GNinja` option if you do not have Ninja installed, though
installing it would significantly speed up this build.

Once the configuration is done, install the dependencies, but not the project
itself:

```console
cmake --build cmake-out/si --target project-dependencies
```

Now you can use these dependencies multiple times. To use them, add the
`$HOME/local-common` directory to `CMAKE_PREFIX_PATH` when you configure
the project:

```console
cmake -H. -Bcmake-out/home -DCMAKE_PREFIX_PATH=$HOME/local-common

cmake --build cmake-out/home -- -j $(nproc)

(cd cmake-out/home && ctest --output-on-failure)
```
