"""
tasks.py for the Python `invoke` package: https://docs.pyinvoke.org/.

It's a replacement of our Makefile with same tasks.

Usage:
    pip install invoke
    invoke --list
"""

import glob
import inspect
import os
import shutil
import sys
from contextlib import contextmanager, suppress

from invoke import call, task

MEDIASOUP_BUILDTYPE = os.getenv("MEDIASOUP_BUILDTYPE") or "Release"
WORKER_DIR = os.path.dirname(
    os.path.abspath(inspect.getframeinfo(inspect.currentframe()).filename)
)
# NOTE: MEDIASOUP_OUT_DIR is overrided by build.rs.
MEDIASOUP_OUT_DIR = os.getenv("MEDIASOUP_OUT_DIR") or f"{WORKER_DIR}/out"
MEDIASOUP_INSTALL_DIR = (
    os.getenv("MEDIASOUP_INSTALL_DIR") or f"{MEDIASOUP_OUT_DIR}/{MEDIASOUP_BUILDTYPE}"
)
BUILD_DIR = os.getenv("BUILD_DIR") or f"{MEDIASOUP_INSTALL_DIR}/build"
# NOTE: Each set of Meson options gets its own build directory. Meson keeps the
# options it's not given again, so sharing a single build directory would leak
# options such as `ms_build_tests` (which adds MS_TEST and MS_LOG_STD to every
# compiled file) into the next build, and would also rebuild everything on every
# switch from a task to another.
TEST_BUILD_DIR = f"{BUILD_DIR}-test"
TEST_ASAN_ADDRESS_BUILD_DIR = f"{BUILD_DIR}-test-asan-address"
TEST_ASAN_UNDEFINED_BUILD_DIR = f"{BUILD_DIR}-test-asan-undefined"
FUZZER_BUILD_DIR = f"{BUILD_DIR}-fuzzer"
BUILD_DIRS = [
    BUILD_DIR,
    TEST_BUILD_DIR,
    TEST_ASAN_ADDRESS_BUILD_DIR,
    TEST_ASAN_UNDEFINED_BUILD_DIR,
    FUZZER_BUILD_DIR,
]
# Custom pip folder for invoke package.
# NOTE: We invoke `pip install` always with `--no-user` to make it not complain
# about "can not combine --user and --target".
PIP_INVOKE_DIR = f"{MEDIASOUP_OUT_DIR}/pip_invoke"
# Custom pip folder for meson and ninja packages.
PIP_MESON_NINJA_DIR = f"{MEDIASOUP_OUT_DIR}/pip_meson_ninja"
# Custom pip folder for ruff package.
PIP_RUFF_DIR = f"{MEDIASOUP_OUT_DIR}/pip_ruff"
# If available (only on some *nix systems), os.sched_getaffinity(0) gets set of
# CPUs the calling thread is restricted to. Instead, os.cpu_count() returns the
# total number of CPUs in a system (it doesn't take into account how many of them
# the calling thread can use).
NUM_CORES = (
    len(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else os.cpu_count()
)
PYTHON = os.getenv("PYTHON") or sys.executable
MESON = os.getenv("MESON") or f"{PIP_MESON_NINJA_DIR}/bin/meson"
MESON_VERSION = os.getenv("MESON_VERSION") or "1.12.0"
# MESON_ARGS can be used to provide extra configuration parameters to meson,
# such as adding defines or changing optimization options. For instance, use
# `MESON_ARGS="-Dms_log_trace=true -Dms_log_file_line=true" npm i` to compile
# worker with tracing and enabled.
# NOTE: On Windows make sure to add `--vsenv` or have MSVS environment already
# active if you override this parameter.
MESON_ARGS = (
    os.getenv("MESON_ARGS")
    if os.getenv("MESON_ARGS")
    else "--vsenv"
    if os.name == "nt"
    else ""
)
NINJA_VERSION = os.getenv("NINJA_VERSION") or "1.13.2"
RUFF_VERSION = os.getenv("RUFF_VERSION") or "0.15.15"
NPM = os.getenv("NPM") or "npm"
DOCKER = os.getenv("DOCKER") or "docker"
# pty=True in ctx.run() is not available on Windows so if stdout is not a TTY
# let's assume PTY is not supported. Related issue in invoke project:
# https://github.com/pyinvoke/invoke/issues/561
PTY_SUPPORTED = os.name != "nt" and sys.stdout.isatty()
# Use sh (widely supported, more than bash) if not in Windows.
SHELL = "/bin/sh" if os.name != "nt" else None

# Disable `*.pyc` files creation.
os.environ["PYTHONDONTWRITEBYTECODE"] = "true"

# Instruct meson where to look for ninja binary.
if os.name == "nt":
    # Windows is, of course, special.
    os.environ["NINJA"] = f"{PIP_MESON_NINJA_DIR}/bin/ninja.exe"
else:
    os.environ["NINJA"] = f"{PIP_MESON_NINJA_DIR}/bin/ninja"

# Instruct Python where to look for modules it needs, such that meson actually
# runs from installed location.
# NOTE: On Windows we must use ";" instead of ":"" to separate paths.
PYTHONPATH = os.getenv("PYTHONPATH") or ""
if os.name == "nt":
    os.environ["PYTHONPATH"] = (
        f"{PIP_INVOKE_DIR};{PIP_MESON_NINJA_DIR};{PIP_RUFF_DIR};{PYTHONPATH}"
    )
else:
    os.environ["PYTHONPATH"] = (
        f"{PIP_INVOKE_DIR}:{PIP_MESON_NINJA_DIR}:{PIP_RUFF_DIR}:{PYTHONPATH}"
    )


@contextmanager
def cd_worker():
    """
    Context manager to change to worker/ folder the safe way
    """

    original_dir = os.getcwd()
    os.chdir(WORKER_DIR)
    try:
        yield
    finally:
        os.chdir(original_dir)


@task
def meson_ninja(ctx):
    """
    Install meson and ninja (also update Python pip and setuptools packages)
    """

    if os.path.isfile(MESON):
        return

    # Updated pip and setuptools are needed for meson.
    # `--system` is not present everywhere and is only needed as workaround for
    # Debian-specific issue (copied from https://github.com/gluster/gstatus/pull/33),
    # fallback to command without `--system` if the first one fails.
    try:
        ctx.run(
            f'"{PYTHON}" -m pip install --system --upgrade --no-user --target "{PIP_MESON_NINJA_DIR}" pip setuptools',
            echo=True,
            hide=True,
            shell=SHELL,
        )
    except Exception:
        ctx.run(
            f'"{PYTHON}" -m pip install --upgrade --no-user --target "{PIP_MESON_NINJA_DIR}" pip setuptools',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )

    # Workaround for NixOS and Guix that don't work with pre-built binaries, see:
    # https://github.com/NixOS/nixpkgs/issues/142383.
    pip_build_binaries = (
        "--no-binary :all:"
        if os.path.isfile("/etc/NIXOS") or os.path.isdir("/etc/guix")
        else ""
    )

    # Install meson and ninja using pip into our custom location, so we don't
    # depend on system-wide installation.
    ctx.run(
        f'"{PYTHON}" -m pip install --upgrade --no-user --target "{PIP_MESON_NINJA_DIR}" {pip_build_binaries} meson=={MESON_VERSION} ninja=={NINJA_VERSION}',
        echo=True,
        pty=PTY_SUPPORTED,
        shell=SHELL,
    )


@task(pre=[meson_ninja])
def setup(ctx, meson_args=MESON_ARGS, build_dir=BUILD_DIR):
    """
    Run meson setup
    """

    if MEDIASOUP_BUILDTYPE == "Release":
        with cd_worker():
            ctx.run(
                f'"{MESON}" setup --prefix "{MEDIASOUP_INSTALL_DIR}" --bindir "" --libdir "" --buildtype release -Db_ndebug=true {meson_args} "{build_dir}"',
                echo=True,
                pty=PTY_SUPPORTED,
                shell=SHELL,
            )
    elif MEDIASOUP_BUILDTYPE == "Debug":
        with cd_worker():
            ctx.run(
                f'"{MESON}" setup --prefix "{MEDIASOUP_INSTALL_DIR}" --bindir "" --libdir "" --buildtype debug {meson_args} "{build_dir}"',
                echo=True,
                pty=PTY_SUPPORTED,
                shell=SHELL,
            )
    else:
        with cd_worker():
            ctx.run(
                f'"{MESON}" setup --prefix "{MEDIASOUP_INSTALL_DIR}" --bindir "" --libdir "" --buildtype {MEDIASOUP_BUILDTYPE} -Db_ndebug=if-release {meson_args} "{build_dir}"',
                echo=True,
                pty=PTY_SUPPORTED,
                shell=SHELL,
            )


@task
def clean(ctx):
    """
    Clean the objects and binaries of mediasoup, keeping those of subprojects
    """

    # NOTE: Meson keeps the objects of each target in a "<target>.p" directory,
    # and those of the subprojects and the dependencies in their own
    # subdirectories, so this removes just what belongs to mediasoup itself.
    # glob.escape() is needed because the path of the build directory may
    # contain characters that glob would otherwise take as wildcards.
    for build_dir in BUILD_DIRS:
        for path in glob.glob(f"{glob.escape(build_dir)}/*.p"):
            shutil.rmtree(path, ignore_errors=True)

    # NOTE: The installed artifacts are files, while the build directories are
    # directories.
    for path in glob.glob(f"{glob.escape(MEDIASOUP_INSTALL_DIR)}/*"):
        if os.path.isfile(path):
            # NOTE: Be as tolerant as shutil.rmtree() above, since in Windows
            # removing a binary that is being run fails.
            with suppress(OSError):
                os.remove(path)


@task
def clean_build(ctx):
    """
    Clean the build directories
    """

    for build_dir in BUILD_DIRS:
        shutil.rmtree(build_dir, ignore_errors=True)


@task
def clean_pip(ctx):
    """
    Clean the local pip setup
    """

    shutil.rmtree(PIP_MESON_NINJA_DIR, ignore_errors=True)
    shutil.rmtree(PIP_RUFF_DIR, ignore_errors=True)


@task(pre=[meson_ninja])
def clean_subprojects(ctx):
    """
    Clean meson subprojects
    """

    with cd_worker():
        ctx.run(
            f'"{MESON}" subprojects purge --include-cache --confirm',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )


@task
def clean_all(ctx):
    """
    Clean meson subprojects and all installed/built artificats
    """

    with cd_worker():
        with suppress(Exception):
            ctx.run(
                f'"{MESON}" subprojects purge --include-cache --confirm',
                echo=True,
                pty=PTY_SUPPORTED,
                shell=SHELL,
            )

        shutil.rmtree(MEDIASOUP_OUT_DIR, ignore_errors=True)
        shutil.rmtree("include/FBS", ignore_errors=True)


@task(pre=[meson_ninja])
def check_wrap_status(ctx):
    """
    Check status of subprojects
    """

    with cd_worker():
        ctx.run(f'"{MESON}" wrap status', echo=True, pty=PTY_SUPPORTED, shell=SHELL)


@task(pre=[meson_ninja])
def update_wrap_file(ctx, subproject):
    """
    Update the wrap file of a subproject
    """

    with cd_worker():
        ctx.run(
            f'"{MESON}" subprojects update --reset {subproject}',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )


@task(pre=[meson_ninja])
def flatc(ctx, meson_args=MESON_ARGS, build_dir=BUILD_DIR):
    """
    Compile FlatBuffers FBS files
    """

    # NOTE: The generated C++ headers are written into the build directory, so
    # they must be generated in the very same build directory that compiles them
    # and with the very same Meson options. setup() is called here instead of
    # being declared as a pre task because a pre task cannot receive the
    # parameters given to the task it precedes.
    setup(ctx, meson_args=meson_args, build_dir=build_dir)

    with cd_worker():
        ctx.run(
            f'"{MESON}" compile -C "{build_dir}" flatbuffers-generator',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )


@task(pre=[flatc], default=True)
def mediasoup_worker(ctx):
    """
    Compile mediasoup-worker binary
    """

    if os.getenv("MEDIASOUP_WORKER_BIN"):
        print(
            "skipping mediasoup-worker compilation due to the existence of the MEDIASOUP_WORKER_BIN environment variable"
        )
        return

    with cd_worker():
        ctx.run(
            f'"{MESON}" compile -C "{BUILD_DIR}" -j {NUM_CORES} mediasoup-worker',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )
    with cd_worker():
        ctx.run(
            f'"{MESON}" install -C "{BUILD_DIR}" --no-rebuild --tags mediasoup-worker',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )


@task(pre=[flatc])
def libmediasoup_worker(ctx):
    """
    Compile libmediasoup-worker library
    """

    with cd_worker():
        ctx.run(
            f'"{MESON}" compile -C "{BUILD_DIR}" -j {NUM_CORES} libmediasoup-worker',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )
    with cd_worker():
        ctx.run(
            f'"{MESON}" install -C "{BUILD_DIR}" --no-rebuild --tags libmediasoup-worker',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )


@task(pre=[flatc])
def xcode(ctx):
    """
    Setup Xcode project
    """

    with cd_worker():
        ctx.run(
            f'"{MESON}" setup --buildtype {MEDIASOUP_BUILDTYPE.lower()} --backend xcode "{MEDIASOUP_OUT_DIR}/xcode"',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )


def _install_ruff(ctx):
    """
    Install ruff via pip into our custom location.
    """

    if os.path.isdir(PIP_RUFF_DIR):
        return

    ctx.run(
        f'"{PYTHON}" -m pip install --upgrade --no-user --target="{PIP_RUFF_DIR}" ruff=={RUFF_VERSION}',
        echo=True,
        pty=PTY_SUPPORTED,
        shell=SHELL,
    )


@task
def lint(ctx):
    """
    Lint C++ and Python source code
    """

    with cd_worker():
        ctx.run(
            f'"{NPM}" run lint --prefix scripts/',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )

    _install_ruff(ctx)

    with cd_worker():
        ctx.run(f'"{PYTHON}" -m ruff check', echo=True, pty=PTY_SUPPORTED, shell=SHELL)
        ctx.run(
            f'"{PYTHON}" -m ruff format --check',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )


@task
def format(ctx):
    """
    Format C++ and Python source code according to lint rules
    """

    with cd_worker():
        ctx.run(
            f'"{NPM}" run format --prefix scripts/',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )

    _install_ruff(ctx)

    with cd_worker():
        ctx.run(
            f'"{PYTHON}" -m ruff check --fix', echo=True, pty=PTY_SUPPORTED, shell=SHELL
        )
        ctx.run(f'"{PYTHON}" -m ruff format', echo=True, pty=PTY_SUPPORTED, shell=SHELL)


@task(
    pre=[
        call(
            flatc,
            meson_args=MESON_ARGS + " -Dms_build_tests=true",
            build_dir=TEST_BUILD_DIR,
        )
    ]
)
def tidy(ctx):
    """
    Performs C++ code checks according to `worker/.clang-tidy` rules
    """

    with cd_worker():
        ctx.run(
            f'"{NPM}" run tidy --prefix scripts/',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
            # NOTE: Tell the script which build directory holds the
            # compile_commands.json to use.
            env={**os.environ, "BUILD_DIR": TEST_BUILD_DIR},
        )


@task(
    pre=[
        call(
            flatc,
            meson_args=MESON_ARGS + " -Dms_build_tests=true",
            build_dir=TEST_BUILD_DIR,
        )
    ]
)
def tidy_fix(ctx):
    """
    Performs C++ code checks according to `worker/.clang-tidy` rules and applies
    fixes
    """

    with cd_worker():
        ctx.run(
            f'"{NPM}" run tidy:fix --prefix scripts/',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
            # NOTE: Tell the script which build directory holds the
            # compile_commands.json to use.
            env={**os.environ, "BUILD_DIR": TEST_BUILD_DIR},
        )


@task(
    pre=[
        call(
            flatc,
            meson_args=MESON_ARGS + " -Dms_build_tests=true",
            build_dir=TEST_BUILD_DIR,
        )
    ]
)
def test(ctx):
    """
    Run worker tests
    """

    with cd_worker():
        ctx.run(
            f'"{MESON}" compile -C "{TEST_BUILD_DIR}" -j {NUM_CORES} mediasoup-worker-test',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )
    with cd_worker():
        ctx.run(
            f'"{MESON}" install -C "{TEST_BUILD_DIR}" --no-rebuild --tags mediasoup-worker-test',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )

    mediasoup_worker_test = (
        "mediasoup-worker-test.exe" if os.name == "nt" else "mediasoup-worker-test"
    )
    mediasoup_test_tags = os.getenv("MEDIASOUP_TEST_TAGS") or ""

    with cd_worker():
        ctx.run(
            f'"{TEST_BUILD_DIR}/{mediasoup_worker_test}" --invisibles --colour-mode=ansi {mediasoup_test_tags}',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )


@task(
    pre=[
        call(
            flatc,
            meson_args=MESON_ARGS
            + " -Dms_build_tests=true -Db_sanitize=address -Db_lundef=false",
            build_dir=TEST_ASAN_ADDRESS_BUILD_DIR,
        )
    ]
)
def test_asan_address(ctx):
    """
    Run worker test with Address Sanitizer with '-fsanitize=address'
    """

    with cd_worker():
        ctx.run(
            f'"{MESON}" compile -C "{TEST_ASAN_ADDRESS_BUILD_DIR}" -j {NUM_CORES} mediasoup-worker-test-asan-address',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )
    with cd_worker():
        ctx.run(
            f'"{MESON}" install -C "{TEST_ASAN_ADDRESS_BUILD_DIR}" --no-rebuild --tags mediasoup-worker-test-asan-address',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )

    mediasoup_test_tags = os.getenv("MEDIASOUP_TEST_TAGS") or ""

    with cd_worker():
        ctx.run(
            f'"{TEST_ASAN_ADDRESS_BUILD_DIR}/mediasoup-worker-test-asan-address" --invisibles {mediasoup_test_tags}',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
            env={
                **os.environ,
                "ASAN_OPTIONS": "halt_on_error=1:print_stacktrace=1:detect_leaks=1:symbolize=1:detect_stack_use_after_return=1:strict_init_order=1:check_initialization_order=1:detect_container_overflow=1",
            },
        )


@task(
    pre=[
        call(
            flatc,
            meson_args=MESON_ARGS
            + " -Dms_build_tests=true -Db_sanitize=undefined -Db_lundef=false",
            build_dir=TEST_ASAN_UNDEFINED_BUILD_DIR,
        )
    ]
)
def test_asan_undefined(ctx):
    """
    Run worker test with undefined Sanitizer with -fsanitize=undefined
    """

    with cd_worker():
        ctx.run(
            f'"{MESON}" compile -C "{TEST_ASAN_UNDEFINED_BUILD_DIR}" -j {NUM_CORES} mediasoup-worker-test-asan-undefined',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )
    with cd_worker():
        ctx.run(
            f'"{MESON}" install -C "{TEST_ASAN_UNDEFINED_BUILD_DIR}" --no-rebuild --tags mediasoup-worker-test-asan-undefined',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )

    mediasoup_test_tags = os.getenv("MEDIASOUP_TEST_TAGS") or ""

    with cd_worker():
        ctx.run(
            f'"{TEST_ASAN_UNDEFINED_BUILD_DIR}/mediasoup-worker-test-asan-undefined" --invisibles {mediasoup_test_tags}',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
            # Exit with error if there are issues.
            # NOTE: Ignore well known UBSan errors in OpenSSL.
            env={
                **os.environ,
                "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1:suppressions=ubsan_suppressions.txt",
            },
        )


@task(
    pre=[
        call(
            flatc,
            meson_args=MESON_ARGS
            + " -Dms_build_fuzzer=true -Db_sanitize=address -Db_lundef=false",
            build_dir=FUZZER_BUILD_DIR,
        )
    ]
)
def fuzzer(ctx):
    """
    Build the mediasoup-worker-fuzzer binary (which uses libFuzzer)
    """

    # NOTE: We need to pass '-Db_sanitize=address' to enable fuzzer in all Meson
    # subprojects, so we pass it to the setup() task (through the flatc() task).

    with cd_worker():
        ctx.run(
            f'"{MESON}" compile -C "{FUZZER_BUILD_DIR}" -j {NUM_CORES} mediasoup-worker-fuzzer',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )
    with cd_worker():
        ctx.run(
            f'"{MESON}" install -C "{FUZZER_BUILD_DIR}" --no-rebuild --tags mediasoup-worker-fuzzer',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )


@task
def fuzzer_run_all(ctx):
    """
    Run all fuzzer cases
    """

    with cd_worker():
        ctx.run(
            f'LSAN_OPTIONS=verbosity=1:log_threads=1 "{FUZZER_BUILD_DIR}/mediasoup-worker-fuzzer" -artifact_prefix=fuzzer/reports/ -max_len=1400 fuzzer/new-corpus deps/webrtc-fuzzer-corpora/corpora/stun-corpus deps/webrtc-fuzzer-corpora/corpora/rtp-corpus deps/webrtc-fuzzer-corpora/corpora/rtcp-corpus',
            echo=True,
            pty=PTY_SUPPORTED,
            shell=SHELL,
        )


@task
def docker(ctx):
    """
    Build a Linux Ubuntu Docker image with fuzzer capable clang++
    """

    if os.getenv("DOCKER_NO_CACHE") == "true":
        with cd_worker():
            ctx.run(
                f'"{DOCKER}" build -f Dockerfile --no-cache --tag mediasoup/docker:latest .',
                echo=True,
                pty=PTY_SUPPORTED,
                shell=SHELL,
            )
    else:
        with cd_worker():
            ctx.run(
                f'"{DOCKER}" build -f Dockerfile --tag mediasoup/docker:latest .',
                echo=True,
                pty=PTY_SUPPORTED,
                shell=SHELL,
            )


@task
def docker_run(ctx):
    """
    Run a container of the Ubuntu Docker image created in the docker task
    """

    with cd_worker():
        ctx.run(
            f'"{DOCKER}" run --name=mediasoupDocker -it --rm --privileged --cap-add SYS_PTRACE -v "{WORKER_DIR}/../:/foo bar/mediasoup" mediasoup/docker:latest',
            echo=True,
            pty=True,  # NOTE: Needed to enter the terminal of the Docker image.
            shell=SHELL,
        )


@task
def docker_alpine(ctx):
    """
    Build a Linux Alpine Docker image
    """

    if os.getenv("DOCKER_NO_CACHE") == "true":
        with cd_worker():
            ctx.run(
                f'"{DOCKER}" build -f Dockerfile.alpine --no-cache --tag mediasoup/docker-alpine:latest .',
                echo=True,
                pty=PTY_SUPPORTED,
                shell=SHELL,
            )
    else:
        with cd_worker():
            ctx.run(
                f'"{DOCKER}" build -f Dockerfile.alpine --tag mediasoup/docker-alpine:latest .',
                echo=True,
                pty=PTY_SUPPORTED,
                shell=SHELL,
            )


@task
def docker_alpine_run(ctx):
    """
    Run a container of the Alpine Docker image created in the docker_alpine task
    """

    with cd_worker():
        ctx.run(
            f'"{DOCKER}" run --name=mediasoupDockerAlpine -it --rm --privileged --cap-add SYS_PTRACE -v "{WORKER_DIR}/../:/foo bar/mediasoup" mediasoup/docker-alpine:latest',
            echo=True,
            pty=True,  # NOTE: Needed to enter the terminal of the Docker image.
            shell=SHELL,
        )


@task
def docker_386(ctx):
    """
    Build a 386 Linux Debian (32 bits arch) Docker image
    """

    if os.getenv("DOCKER_NO_CACHE") == "true":
        with cd_worker():
            ctx.run(
                f'"{DOCKER}" build --platform linux/386 -f Dockerfile.386 --no-cache --tag mediasoup/docker-386:latest .',
                echo=True,
                pty=PTY_SUPPORTED,
                shell=SHELL,
            )
    else:
        with cd_worker():
            ctx.run(
                f'"{DOCKER}" build --platform linux/386 -f Dockerfile.386 --tag mediasoup/docker-386:latest .',
                echo=True,
                pty=PTY_SUPPORTED,
                shell=SHELL,
            )


@task
def docker_386_run(ctx):
    """
    Run a container of the 386 Linux Debian (32 bits arch) Docker image created
    in the docker_386 task
    """

    with cd_worker():
        ctx.run(
            f'"{DOCKER}" run --name=mediasoupDocker386 -it --rm --privileged --cap-add SYS_PTRACE -v "{WORKER_DIR}/../:/foo bar/mediasoup" mediasoup/docker-386:latest',
            echo=True,
            pty=True,  # NOTE: Needed to enter the terminal of the Docker image.
            shell=SHELL,
        )
