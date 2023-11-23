# This is a tasks.py file for the pip invoke package: https://docs.pyinvoke.org/.
#
# It's a replacement of our Makefile with same tasks. Obviously it requires
# having the pip invoke package installed, however I think we could just
# include it in the mediasoup repo in the worker/pip-invoke folder and make
# npm-scripts.mjs invoke its executable in worker/pip-invoke/bin/invoke (same
# for Rust). However that's hard since we also need to later override PYTHONPATH
# env for pip invoke module to be found. So...
#
# Perhaps it's easier if we run worker/scripts/get-pip-invoke.sh during the
# installation. That script installs pip invoke package in default pip location.
#
# Or we can set PYTHONPATH env within npm-scripts.mjs (and Rust) and also in the
# Makefile, so we don't remove it and instead make it behave as a proxy to invoke.
#
# Let's see.


import sys;
import os;
import inspect;
import shutil;
import subprocess;
import re;
from invoke import task;


MEDIASOUP_BUILDTYPE = os.getenv('MEDIASOUP_BUILDTYPE') or 'Release';

WORKER_DIR = os.path.dirname(os.path.abspath(
    inspect.getframeinfo(inspect.currentframe()).filename
));
OUT_DIR = os.getenv('OUT_DIR') or f'{WORKER_DIR}/out';
INSTALL_DIR = os.getenv('INSTALL_DIR') or f'{OUT_DIR}/{MEDIASOUP_BUILDTYPE}';
BUILD_DIR = os.getenv('BUILD_DIR') or f'{INSTALL_DIR}/build';
PIP_DIR = f'{OUT_DIR}/pip';

NUM_CORES = re.findall(
    r'\b\d+\b', subprocess.check_output(f'{WORKER_DIR}/scripts/cpu_cores.sh', text=True)
)[0];

PYTHON = os.getenv('PYTHON') or sys.executable;
MESON = os.getenv('MESON') or f'{PIP_DIR}/bin/meson';
MESON_VERSION = os.getenv('MESON_VERSION') or '1.2.1';
# MESON_ARGS can be used to provide extra configuration parameters to meson,
# such as adding defines or changing optimization options. For instance, use
# `MESON_ARGS="-Dms_log_trace=true -Dms_log_file_line=true" npm i` to compile
# worker with tracing and enabled.
# NOTE: On Windows make sure to add `--vsenv` or have MSVS environment already
# active if you override this parameter.
# TODO: Replicate this:
# # Activate VS environment on Windows by default.
# ifeq ($(OS),Windows_NT)
# ifeq ($(MESON_ARGS),"")
#   MESON_ARGS = $(subst $\",,"--vsenv")
# endif
# endif
MESON_ARGS = '';
# Let's use a specific version of ninja to avoid buggy version 1.11.1:
# https://mediasoup.discourse.group/t/partly-solved-could-not-detect-ninja-v1-8-2-or-newer/
# https://github.com/ninja-build/ninja/issues/2211
# https://github.com/ninja-build/ninja/issues/2212
NINJA_VERSION = os.getenv('NINJA_VERSION') or '1.10.2.4';
NPM = os.getenv('NPM') or 'npm';
LCOV = os.getenv('LCOV') or f'{WORKER_DIR}/deps/lcov/bin/lcov';

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
# NOTE: For some reason on Windows adding `:${PYTHONPATH}` breaks things.
PYTHONPATH = os.getenv('PYTHONPATH') or '';
if os.name == 'nt':
    os.environ['PYTHONPATH'] = PIP_DIR;
else:
    os.environ['PYTHONPATH'] = f'{PIP_DIR}:{PYTHONPATH}';

# TODO: MESON_ARGS stuff (if not done above).

# TODO: Remove.
print('WORKER_DIR:', WORKER_DIR);
print('OUT_DIR:', OUT_DIR);
print('INSTALL_DIR:', INSTALL_DIR);
print('BUILD_DIR:', BUILD_DIR);
print('PIP_DIR:', PIP_DIR);
print('NUM_CORES:', NUM_CORES);
print('PYTHON:', PYTHON);
print('MESON:', MESON);


@task
def meson_ninja(ctx):
    """
    Install meson and ninja (also update Python pip and setuptools packages)
    """
    if os.path.isdir(PIP_DIR):
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
            echo=True
        );

    # Workaround for NixOS and Guix that don't work with pre-built binaries, see:
    # https://github.com/NixOS/nixpkgs/issues/142383.
    PIP_BUILD_BINARIES = '';

    if os.path.isfile('/etc/NIXOS') or os.path.isdir('/etc/guix'):
        PIP_BUILD_BINARIES = '--no-binary :all:';

    # Install meson and ninja using pip into our custom location, so we don't
    # depend on system-wide installation.
    ctx.run(
        f'{PYTHON} -m pip install --upgrade --target={PIP_DIR} {PIP_BUILD_BINARIES} meson=={MESON_VERSION} ninja=={NINJA_VERSION}',
        echo=True
    );


@task(pre=[meson_ninja])
def setup(ctx):
    """
    Run meson setup
    """
    # We try to call `--reconfigure` first as a workaround for this issue:
    # https://github.com/ninja-build/ninja/issues/1997
    if MEDIASOUP_BUILDTYPE == 'Release':
        try:
            with ctx.cd(WORKER_DIR):
                ctx.run(
                    f'{MESON} setup --prefix {INSTALL_DIR} --bindir "" --libdir "" --buildtype release -Db_ndebug=true -Db_pie=true -Db_staticpic=true --reconfigure {MESON_ARGS} {BUILD_DIR}',
                    echo=True
                );
        except:
            with ctx.cd(WORKER_DIR):
                ctx.run(
                    f'{MESON} setup --prefix {INSTALL_DIR} --bindir "" --libdir "" --buildtype release -Db_ndebug=true -Db_pie=true -Db_staticpic=true {MESON_ARGS} {BUILD_DIR}',
                    echo=True
                );
    elif MEDIASOUP_BUILDTYPE == 'Debug':
        try:
            with ctx.cd(WORKER_DIR):
                ctx.run(
                    f'{MESON} setup --prefix {INSTALL_DIR} --bindir "" --libdir "" --buildtype debug -Db_pie=true -Db_staticpic=true --reconfigure {MESON_ARGS} {BUILD_DIR}',
                    echo=True
                );
        except:
            with ctx.cd(WORKER_DIR):
                ctx.run(
                    f'{MESON} setup --prefix {INSTALL_DIR} --bindir "" --libdir "" --buildtype debug -Db_pie=true -Db_staticpic=true {MESON_ARGS} {BUILD_DIR}',
                    echo=True
                );
    else:
        try:
            with ctx.cd(WORKER_DIR):
                ctx.run(
                    f'{MESON} setup --prefix {INSTALL_DIR} --bindir "" --libdir "" --buildtype {MEDIASOUP_BUILDTYPE} -Db_ndebug=if-release -Db_pie=true -Db_staticpic=true --reconfigure {MESON_ARGS} {BUILD_DIR}',
                    echo=True
                );
        except:
            with ctx.cd(WORKER_DIR):
                ctx.run(
                    f'{MESON} setup --prefix {INSTALL_DIR} --bindir "" --libdir "" --buildtype {MEDIASOUP_BUILDTYPE} -Db_ndebug=if-release -Db_pie=true -Db_staticpic=true {MESON_ARGS} {BUILD_DIR}',
                    echo=True
                );


@task
def clean(ctx):
    """
    Clean the installation directory
    """
    try:
        shutil.rmtree(INSTALL_DIR);
    except:
        pass;


@task
def clean_build(ctx):
    """
    Clean the build directory
    """
    try:
        shutil.rmtree(BUILD_DIR);
    except:
        pass;


@task
def clean_pip(ctx):
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
            echo=True
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
                echo=True
            );
        except:
            pass;

        try:
            shutil.rmtree(OUT_DIR);
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
            echo=True
        );


@task(pre=[setup])
def flatc(ctx):
    """
    Compile FlatBuffers FBS files
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} compile -C {BUILD_DIR} flatbuffers-generator',
            echo=True
        );


@task(pre=[setup, flatc])
def mediasoup_worker(ctx):
    """
    Compile mediasoup-worker binary
    """
    if os.getenv('MEDIASOUP_WORKER_BIN'):
        printf('skipping mediasoup-worker compilation due to the existence of the MEDIASOUP_WORKER_BIN environment variable');
        return;

    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} compile -C {BUILD_DIR} -j {NUM_CORES} mediasoup-worker',
            echo=True
        );
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} compile -C {BUILD_DIR} --no-rebuild --tags mediasoup-worker',
            echo=True
        );


@task(pre=[setup, flatc])
def libmediasoup_worker(ctx):
    """
    Compile libmediasoup-worker library
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} compile -C {BUILD_DIR} -j {NUM_CORES} libmediasoup-worker',
            echo=True
        );
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} compile -C {BUILD_DIR} --no-rebuild --tags libmediasoup-worker',
            echo=True
        );


@task(pre=[setup, flatc])
def xcode(ctx):
    """
    Setup Xcode project
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} setup --buildtype {MEDIASOUP_BUILDTYPE} --backend xcode {MEDIASOUP_OUT_DIR}/xcode',
            echo=True
        );


@task
def lint(ctx):
    """
    Lint source code
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{NPM} run lint --prefix scripts/',
            echo=True
        );


@task
def format(ctx):
    """
    Format source code according to lint rules
    """
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{NPM} run format --prefix scripts/',
            echo=True
        );


@task(pre=[setup, flatc])
def test(ctx):
    """
    Run worker tests
    """
    ctx.run('echo $MEDIASOUP_LOCAL_DEV', echo=True);

    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} compile -C {BUILD_DIR} -j {NUM_CORES} mediasoup-worker-test',
            echo=True
        );
    with ctx.cd(WORKER_DIR):
        ctx.run(
            f'{MESON} install -C {BUILD_DIR}  --no-rebuild --tags mediasoup-worker-test',
            echo=True
        );

    MEDIASOUP_TEST_TAGS = os.getenv('MEDIASOUP_TEST_TAGS') or '';

    # On Windows lcov doesn't work (at least not yet) and we need to add .exe to
    # the binary path.
    if os.name == 'nt':
        with ctx.cd(WORKER_DIR):
            ctx.run(
                f'{BUILD_DIR}/mediasoup-worker-test.exe --invisibles --use-colour=yes {MEDIASOUP_TEST_TAGS}',
                echo=True
            );
    else:
        ctx.run(
            f'{LCOV} --directory {WORKER_DIR} --zerocounters',
            echo=True
        );
        with ctx.cd(WORKER_DIR):
            ctx.run(
                f'{BUILD_DIR}/mediasoup-worker-test --invisibles --use-colour=yes {MEDIASOUP_TEST_TAGS}',
                echo=True
            );


@task(pre=[meson_ninja], default=True)
def foo(ctx):
    """
    Foo things
    """

    print('foo!');

