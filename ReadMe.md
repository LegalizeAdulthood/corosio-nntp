[![CMake workflow](https://github.com/LegalizeAdulthood/corosio-nntp/actions/workflows/cmake.yml/badge.svg)](https://github.com/LegalizeAdulthood/corosio-nntp/actions/workflows/cmake.yml)

# corosio NNTP

Sample code for the video "News Reader with corosio".

# Obtaining the Source

Use git to clone this repository, then update the vcpkg submodule to bootstrap
the dependency process.

```
git clone https://github.com/LegalizeAdulthood/corosio-nntp
cd corosio-nntp
git submodule init
git submodule update --depth 1
```

# Building

A CMake preset has been provided to perform the usual CMake steps of
configure, build and test.

```
cmake --workflow --preset default
```

Places the build outputs in a sibling directory of the source code directory, e.g. up
and outside of the source directory.

[Utah C++ Programmers](https://meetup.com/utah-cpp-programmers)\
[Past Topics](https://utahcpp.wordpress.com/past-meeting-topics/)\
[Future Topics](https://utahcpp.wordpress.com/future-meeting-topics/)
