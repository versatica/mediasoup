# Ignore these pylint warnings, concentions and refactoring messages:
# - W0301: Unnecessary semicolon (unnecessary-semicolon)
# - W0622: Redefining built-in 'format' (redefined-builtin)
# - W0702: No exception type(s) specified (bare-except)
# - C0114: Missing module docstring (missing-module-docstring)
# - C0301: Line too long (line-too-long)
#
# pylint: disable=W0301,W0622,W0702,C0114,C0301

#
# This is a tasks.py file for the pip invoke package: https://docs.pyinvoke.org/.
#
# It's a replacement of our Makefile with same tasks.
#
# Usage:
#   invoke --list
#

import sys;
import os;
import inspect;
import shutil;
from invoke import task;

MEDIASOUP_BUILDTYPE = os.getenv('MEDIASOUP_BUILDTYPE') or 'Release';
WORKER_DIR = os.path.dirname(os.path.abspath(
    inspect.getframeinfo(inspect.currentframe()).filename
));
# NOTE: MEDIASOUP_OUT_DIR is overrided by build.rs.
MEDIASOUP_OUT_DIR = os.getenv('MEDIASOUP_OUT_DIR') or f'{WORKER_DIR}/out';
MEDIASOUP_INSTALL_DIR = os.getenv('MEDIASOUP_INSTALL_DIR') or f'{MEDIASOUP_OUT_DIR}/{MEDIASOUP_BUILDTYPE}';
BUILD_DIR = os.getenv('BUILD_DIR') or f'{MEDIASOUP_INSTALL_DIR}/build';
PIP_DIR = f'{MEDIASOUP_OUT_DIR}/pip';
# If available (only on some *nix systems), os.sched_getaffinity(0) gets set of
# CPUs the calling thread is restricted to. Instead, os.cpu_count() returns the
# total number of CPUs in a system (it doesn't take into account how many of them
# the calling thread can use).
NUM_CORES = len(os.sched_getaffinity(0)) if hasattr(os, 'sched_getaffinity') else os.cpu_count();
PYTHON = os.getenv('PYTHON') or sys.executable;
MESON = os.getenv('MESON') or f'{PIP_DIR}/bin/meson';
MESON_VERSION = os.getenv('MESON_VERSION') or '1.2.1';
# MESON_ARGS can be used to provide extra configuration parameters to meson,
# such as adding defines or changing optimization options. For instance, use
# `MESON_ARGS="-Dms_log_trace=true -Dms_log_file_line=true" npm i` to compile
# worker with tracing and enabled.
# NOTE: On Windows make sure to add `--vsenv` or have MSVS environment already
# active if you override this parameter.
MESON_ARGS = os.getenv('MESON_ARGS') if os.getenv('MESON_ARGS') else '--vsenv' if os.name == 'nt' else '';
# Let's use a specific version of ninja to avoid buggy version 1.11.1:
# https://mediasoup.discourse.group/t/partly-solved-could-not-detect-ninja-v1-8-2-or-newer/
# https://github.com/ninja-build/ninja/issues/2211
# https://github.com/ninja-build/ninja/issues/2212
NINJA_VERSION = os.getenv('NINJA_VERSION') or '1.10.2.4';
PYLINT = f'{PIP_DIR}/bin/pylint';
PYLINT_VERSION = os.getenv('PYLINT_VERSION') or '3.0.2';
NPM = os.getenv('NPM') or 'npm';
LCOV = f'{WORKER_DIR}/deps/lcov/bin/lcov';
DOCKER = os.getenv('DOCKER') or 'docker';
# pty=True in ctx.run() is not available on Windows so if stdout is not a TTY
# let's assume PTY is not supported. Related issue in invoke project:
# https://github.com/pyinvoke/invoke/issues/561
PTY_SUPPORTED = sys.stdout.isatty();

# Disable `*.pyc` files creation.
os.environ['PYTHONDONTWRITEBYTECODE'] = 'true';

# Instruct meson where to look for ninja binary.
if os.name == 'nt':
    # Windows is, of course, special.
    os.environ['NINJA'] = f'{PIP_DIR}/bin/ninja.exe';
else:
    os.environ['NINJA'] = f'{PIP_DIR}/bin/ninja';

# Instruct Python where to look for modules it needs, such that meson actually
# runs from installed location.
# NOTE: For some reason on Windows adding `:{PYTHONPATH}` breaks things.
if os.name == 'nt':
    os.environ['PYTHONPATH'] = PIP_DIR;
else:
    PYTHONPATH = os.getenv('PYTHONPATH') or '';
    os.environ['PYTHONPATH'] = f'{PIP_DIR}:{PYTHONPATH}';


@task
def meson_ninja(ctx):
    """
    Install meson and ninja (also update Python pip and setuptools packages)
    """
    if os.path.isfile(MESON):
        return;

    # Updated pip and setuptools are needed for meson.
    # `--system` is not present everywhere and is only needed as workaround for
    # Debian-specific issue (copied from https://github.com/gluster/gstatus/pull/33),
    # fallback to command without `--system` if the first one fails.
    try:
        ctx.run(
            f'{PYTHON} -m pip install --system --target={PIP_DIR} pip setuptools',
            echo=True,
            hide=True
        );
    except:
        ctx.run(
            f'{PYTHON} -m pip install --target={PIP_DIR} pip setuptools',
            echo=True,
            pty=PTY_SUPPORTED
        );

    # Workaround for NixOS and Guix that don't work with pre-built binaries, see:
    # https://github.com/NixOS/nixpkgs/issues/142383.
    pip_build_binaries = '--no-binary :all:' if os.path.isfile('/etc/NIXOS') or os.path.isdir('/etc/guix') else '';

    # Install meson and ninja using pip into our custom location, so we don't
    # depend on system-wide installation.
    ctx.run(
        f'{PYTHON} -m pip install --upgrade --target={PIP_DIR} {pip_build_binaries} meson=={MESON_VERSION} ninja=={NINJA_VERSION}',
        echo=True,
        pty=PTY_SUPPORTED
    );


@task(pre=[meson_ninja])
def setup(ctx):
    """
    Run meson setup
    """
    # We add --reconfigure first as a workaround for this issue:
    # https://github.com/ninja-build/ninja/issues/1997
    if MEDIASOUP_BUILDTYPE == 'Release':
        try:
            with ctx.cd(WORKER_DIR):
                ctx.run(
                    f'{MESON} setup --prefix {MEDIASOUP_INSTALL_DIR} --bindir "" --libdir "" --buildtype release -Db_ndebug=true -Db_pie=true -Db_staticpic=true --reconfigure {MESON_ARGS} {BUILD_DIR}',
                    echo=True,
                    pty=PTY_SUPPORTED
                );
        except:
            with ctx.cd(WORKER_DIR):
                ctx.run(
                    f'{MESON} setup --prefix {MEDIASOUP_INSTALL_DIR} --bindir "" --libdir "" --buildtype release -Db_ndebug=true -Db_pie=true -Db_staticpic=true {MESON_ARGS} {BUILD_DIR}',
                    echo=True,
                    pty=PTY_SUPPORTED
                );
    elif MEDIASOUP_BUILDTYPE == 'Debug':
        try:
            with ctx.cd(WORKER_DIR):
                ctx.run(
                    f'{MESON} setup --prefix {MEDIASOUP_INSTALL_DIR} --bindir "" --libdir "" --buildtype debug -Db_pie=true -Db_staticpic=true --reconfigure {MESON_ARGS} {BUILD_DIR}',
                    echo=True,
                    pty=PTY_SUPPORTED
                );
        except:
            with ctx.cd(WORKER_DIR):
                ctx.run(
                    f'{MESON} setup --prefix {MEDIASOUP_INSTALL_DIR} --bindir "" --libdir "" --buildtype debug -Db_pie=true -Db_staticpic=true {MESON_ARGS} {BUILD_DIR}',
                    echo=True,
                    pty=PTY_SUPPORTED
                );
    else:
        try:
            with ctx.cd(WORKER_DIR):
                ctx.run(
                    f'{MESON} setup --prefix {MEDIASOUP_INSTALL_DIR} --bindir "" --libdir "" --buildtype {MEDIASOUP_BUILDTYPE} -Db_ndebug=if-release -Db_pie=true -Db_staticpic=true --reconfigure {MESON_ARGS} {BUILD_DIR}',
                    echo=True,
                    pty=PTY_SUPPORTED
                );
        except:
            with ctx.cd(WORKER_DIR):
                ctx.run(
                    f'{MESON} setup --prefix {MEDIASOUP_INSTALL_DIR} --bindir "" --libdir "" --buildtype {MEDIASOUP_BUILDTYPE} -Db_ndebug=if-release -Db_pie=true -Db_staticpic=true {MESON_ARGS} {BUILD_DIR}',
                    echo=True,
                    pty=PTY_SUPPORTED
                );


@task
def clean(ctx): # pylint: disable=unused-argument
    """
    Clean the installation directory
    """
    try:
        shutil.rmtree(MEDIASOUP_INSTALL_DIR);
    except:
        pass;


@task
def clean_build(ctx): # pylint: disable=unused-argument
    """
    Clean the build directory
    """
    try:
        shutil.rmtree(BUILD_DIR);
    except:
        pass;


@task
def clean_pip(ctx): # pylint: disable=unused-argument
    """
    Clean the local pip directory
    """
    try:
        shutil.rmtree(PIP_DIR);
    except:
        pass;


@task(pre=[meson_ninja])
def clean_subprojects(ctx):
    """
    Clean meson subprojects
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} subprojects purge --include-cache --confirm',
            echo=True,
            pty=PTY_SUPPORTED
        );


@task
def clean_all(ctx):
    """
    Clean meson subprojects and all installed/built artificats
    """
    with ctx.cd(WORKER_DIR):
        try:
            ctx.run(
                f'{MESON} subprojects purge --include-cache --confirm',
                echo=True,
                pty=PTY_SUPPORTED
            );
        except:
            pass;

        try:
            shutil.rmtree(MEDIASOUP_OUT_DIR);
        except:
            pass;


@task(pre=[meson_ninja])
def update_wrap_file(ctx, subproject):
    """
    Update the wrap file of a subproject
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} subprojects update --reset {subproject}',
            echo=True,
            pty=PTY_SUPPORTED
        );


@task(pre=[setup])
def flatc(ctx):
    """
    Compile FlatBuffers FBS files
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} compile -C {BUILD_DIR} flatbuffers-generator',
            echo=True,
            pty=PTY_SUPPORTED
        );


@task(pre=[setup, flatc], default=True)
def mediasoup_worker(ctx):
    """
    Compile mediasoup-worker binary
    """
    if os.getenv('MEDIASOUP_WORKER_BIN'):
        print('skipping mediasoup-worker compilation due to the existence of the MEDIASOUP_WORKER_BIN environment variable');
        return;

    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} compile -C {BUILD_DIR} -j {NUM_CORES} mediasoup-worker',
            echo=True,
            pty=PTY_SUPPORTED
        );
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} install -C {BUILD_DIR} --no-rebuild --tags mediasoup-worker',
            echo=True,
            pty=PTY_SUPPORTED
        );


@task(pre=[setup, flatc])
def libmediasoup_worker(ctx):
    """
    Compile libmediasoup-worker library
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} compile -C {BUILD_DIR} -j {NUM_CORES} libmediasoup-worker',
            echo=True,
            pty=PTY_SUPPORTED
        );
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} install -C {BUILD_DIR} --no-rebuild --tags libmediasoup-worker',
            echo=True,
            pty=PTY_SUPPORTED
        );


@task(pre=[setup, flatc])
def xcode(ctx):
    """
    Setup Xcode project
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} setup --buildtype {MEDIASOUP_BUILDTYPE} --backend xcode {MEDIASOUP_OUT_DIR}/xcode',
            echo=True,
            pty=PTY_SUPPORTED
        );


@task(pre=[meson_ninja])
def lint(ctx):
    """
    Lint source code
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{NPM} run lint --prefix scripts/',
            echo=True,
            pty=PTY_SUPPORTED
        );

    if not os.path.isfile(PYLINT):
        # Install pylint using pip into our custom location.
        ctx.run(
            f'{PYTHON} -m pip install --upgrade --target={PIP_DIR} pylint=={PYLINT_VERSION}',
            echo=True,
            pty=PTY_SUPPORTED
        );

    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{PYLINT} tasks.py',
            echo=True,
            pty=PTY_SUPPORTED
        );


@task
def format(ctx):
    """
    Format source code according to lint rules
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{NPM} run format --prefix scripts/',
            echo=True,
            pty=PTY_SUPPORTED
        );


@task(pre=[setup, flatc])
def test(ctx):
    """
    Run worker tests
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} compile -C {BUILD_DIR} -j {NUM_CORES} mediasoup-worker-test',
            echo=True,
            pty=PTY_SUPPORTED
        );
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} install -C {BUILD_DIR} --no-rebuild --tags mediasoup-worker-test',
            echo=True,
            pty=PTY_SUPPORTED
        );

    mediasoup_test_tags = os.getenv('MEDIASOUP_TEST_TAGS') or '';

    # On Windows lcov doesn't work (at least not yet) and we need to add .exe to
    # the binary path.
    if os.name == 'nt':
        with ctx.cd(WORKER_DIR):
            ctx.run(
                f'{BUILD_DIR}/mediasoup-worker-test.exe --invisibles --use-colour=yes {mediasoup_test_tags}',
                echo=True,
                pty=PTY_SUPPORTED
            );
    else:
        ctx.run(
            f'{LCOV} --directory {WORKER_DIR} --zerocounters',
            echo=True,
            pty=PTY_SUPPORTED
        );
        with ctx.cd(WORKER_DIR):
            ctx.run(
                f'{BUILD_DIR}/mediasoup-worker-test --invisibles --use-colour=yes {mediasoup_test_tags}',
                echo=True,
                pty=PTY_SUPPORTED
            );


@task(pre=[setup, flatc])
def test_asan(ctx):
    """
    Run worker test with Address Sanitizer
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} compile -C {BUILD_DIR} -j {NUM_CORES} mediasoup-worker-test-asan',
            echo=True,
            pty=PTY_SUPPORTED
        );
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} install -C {BUILD_DIR} --no-rebuild --tags mediasoup-worker-test-asan',
            echo=True,
            pty=PTY_SUPPORTED
        );

    mediasoup_test_tags = os.getenv('MEDIASOUP_TEST_TAGS') or '';

    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'ASAN_OPTIONS=detect_leaks=1 {BUILD_DIR}/mediasoup-worker-test-asan --invisibles --use-colour=yes {mediasoup_test_tags}',
            echo=True,
            pty=PTY_SUPPORTED
        );


@task
def tidy(ctx):
    """
    Perform C++ checks with clang-tidy
    """
    mediasoup_tidy_checks = os.getenv('MEDIASOUP_TIDY_CHECKS') or '';

    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{PYTHON} ./scripts/clang-tidy.py -clang-tidy-binary=./scripts/node_modules/.bin/clang-tidy -clang-apply-replacements-binary=./scripts/node_modules/.bin/clang-apply-replacements -header-filter="(Channel/**/*.hpp|DepLibSRTP.hpp|DepLibUV.hpp|DepLibWebRTC.hpp|DepOpenSSL.hpp|DepUsrSCTP.hpp|LogLevel.hpp|Logger.hpp|MediaSoupError.hpp|RTC/**/*.hpp|Settings.hpp|Utils.hpp|Worker.hpp|common.hpp|handles/**/*.hpp)" -p={BUILD_DIR} -j={NUM_CORES} -checks={mediasoup_tidy_checks} -quiet',
            echo=True,
            pty=PTY_SUPPORTED
        );


@task(pre=[setup, flatc])
def fuzzer(ctx):
    """
    Build the mediasoup-worker-fuzzer binary (which uses libFuzzer)
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} compile -C {BUILD_DIR} -j {NUM_CORES} mediasoup-worker-fuzzer',
            echo=True,
            pty=PTY_SUPPORTED
        );
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} install -C {BUILD_DIR} --no-rebuild --tags mediasoup-worker-fuzzer',
            echo=True,
            pty=PTY_SUPPORTED
        );


@task
def fuzzer_run_all(ctx):
    """
    Run all fuzzer cases
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'LSAN_OPTIONS=verbosity=1:log_threads=1 {BUILD_DIR}/mediasoup-worker-fuzzer -artifact_prefix=fuzzer/reports/ -max_len=1400 fuzzer/new-corpus deps/webrtc-fuzzer-corpora/corpora/stun-corpus deps/webrtc-fuzzer-corpora/corpora/rtp-corpus deps/webrtc-fuzzer-corpora/corpora/rtcp-corpus',
            echo=True,
            pty=PTY_SUPPORTED
        );


@task
def docker(ctx):
    """
    Build a Linux Ubuntu Docker image with fuzzer capable clang++
    """
    if os.getenv('DOCKER_NO_CACHE') == 'true':
        with ctx.cd(WORKER_DIR):
            ctx.run(
                f'{DOCKER} build -f Dockerfile --no-cache --tag mediasoup/docker:latest .',
                echo=True,
                pty=PTY_SUPPORTED
            );
    else:
        with ctx.cd(WORKER_DIR):
            ctx.run(
                f'{DOCKER} build -f Dockerfile --tag mediasoup/docker:latest .',
                echo=True,
                pty=PTY_SUPPORTED
            );


@task
def docker_run(ctx):
    """
    Run a container of the Ubuntu Docker image created in the docker task
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{DOCKER} run --name=mediasoupDocker -it --rm --privileged --cap-add SYS_PTRACE -v {WORKER_DIR}/../:/mediasoup mediasoup/docker:latest',
            echo=True,
            pty=True # NOTE: Needed to enter the terminal of the Docker image.
        );


@task
def docker_alpine(ctx):
    """
    Build a Linux Alpine Docker image
    """
    if os.getenv('DOCKER_NO_CACHE') == 'true':
        with ctx.cd(WORKER_DIR):
            ctx.run(
                f'{DOCKER} build -f Dockerfile.alpine --no-cache --tag mediasoup/docker-alpine:latest .',
                echo=True,
                pty=PTY_SUPPORTED
            );
    else:
        with ctx.cd(WORKER_DIR):
            ctx.run(
                f'{DOCKER} build -f Dockerfile.alpine --tag mediasoup/docker-alpine:latest .',
                echo=True,
                pty=PTY_SUPPORTED
            );


@task
def docker_alpine_run(ctx):
    """
    Run a container of the Alpine Docker image created in the docker_alpine task
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{DOCKER} run --name=mediasoupDockerAlpine -it --rm --privileged --cap-add SYS_PTRACE -v {WORKER_DIR}/../:/mediasoup mediasoup/docker-alpine:latest',
            echo=True,
            pty=True # NOTE: Needed to enter the terminal of the Docker image.
        );
