[![License:MIT](https://img.shields.io/badge/License-MIT-blue?style=plastic)](LICENSE)
[![C CI build](../../actions/workflows/build.yml/badge.svg)](../../actions/workflows/build.yml)

### cntrinit
another minimal init for containers

cntrinit is a tiny PID 1 / init for containers, in the same spirit as
[tini](https://github.com/krallin/tini) and
[catatonit](https://github.com/openSUSE/catatonit). It launches your process,
forwards signals to it, reaps zombie/orphaned processes and propagates the
child's exit status. It can also run on its own, without a child, as the init
of an *infra* container (a shared-namespace "pause" container, similar to a pod
on podman).

It is written in C and links **fully statically with no runtime dependencies**,
so the resulting binary can be dropped into any container image, scratch
included.

### <sub>Notes</sub>
I created cntrinit for a container system I've been working on. It doesn't differ
much from the other, very good inits out there; the reason for writing another
one is that I occasionally find additional features useful and can then add them
to this version.

I usually write my code in C++, but since I wanted the init to build statically
and had trouble linking C++ statically, I kept this one in C.

### Building

cntrinit has no dependencies beyond a C compiler and libc.

```sh
make            # builds ./cntrinit (static, no-pie)
make strip      # optional: strip the binary down to a few tens of KB
make install    # installs to $(DESTDIR)$(PREFIX)/bin (PREFIX defaults to /usr/local)
```

A statically linked musl toolchain (e.g. on Alpine) produces the smallest,
most portable binary, but glibc works too.

### Usage

```
cntrinit [options] -- cmd args
```

Everything after `--` is the command (and its arguments) to run as the child.
If you leave out the command, cntrinit runs as an *infra* init: it simply waits
and exits cleanly on `SIGTERM`/`SIGINT`. Run with a bare `--` and no command to
start in infra mode without the "running without args" warning.

The child keeps the working directory set by the runtime (so `WORKDIR` is
honoured), and the command is resolved through the normal `PATH` lookup when it
contains no slash. Multi-call binaries (busybox, coreutils, ...) keep the name
they were invoked as.

#### Options

| Option | Argument | Description |
| --- | --- | --- |
| `-n`, `--name` | `NAME` | set the init's process name (shown in `ps`/`/proc`) |
| `-k`, `--kill-signal` | `SIGNAL` | signal to send to the child when the parent dies (`PR_SET_PDEATHSIG`) |
| `-g`, `--group-signal` | | forward signals to the child's whole process group, not just the child |
| `-p`, `--pid-file` | `FILE` | write the init's PID to `FILE` |
| `-c`, `--cpid-file` | `FILE` | write the child's PID to `FILE` |
| `-h`, `--help` | | show usage |
| `-v`, `--version` | | show version |
| `-d`, `--debug` | | enable debug output |

`SIGNAL` is given by name in uppercase, e.g. `SIGTERM`; an invalid name prints
the list of supported signals.

#### Exit status

cntrinit exits with the child's exit status. A child terminated by a signal
yields `128 + signum` (the shell convention). If the command cannot be executed,
the exit status is `127`.

#### Examples

```sh
# run a program as PID 1, forwarding signals and reaping zombies
cntrinit -- /usr/bin/myserver --config /etc/my.conf

# name the init, write both pid files, forward to the whole process group
cntrinit -n myserver -g -p /run/init.pid -c /run/app.pid -- myserver

# infra / pause container (no child)
cntrinit --
```
