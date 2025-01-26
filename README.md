[![License:MIT](https://img.shields.io/badge/License-MIT-blue?style=plastic)](LICENSE)
[![C++ CI build](../../actions/workflows/build.yml/badge.svg)](../../actions/workflows/build.yml)

### cntrinit
another minimal init for containers

### <sub>Notes</sub>
I created cntrinit for a container system I've been working on; it doesn't have much difference with
other existing very good inits, reason for creating another one is that I might find some
additional features useful and therefor I can add them to this version.

I usually write my code in C++, but since I wanted my init to be able to be built statically and
had problems to link C++ code statically, I decided to keep with C.

This init can execute another program- but also is capable to run on it's own as it's also used
for so called infra containers that are used for shared namespaces; similar to pods on podman.

Currently there are now features that wouldn't be available on similar inits, besides possibility
to write pid files, for both processes, parent and child.
