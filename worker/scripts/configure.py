#!/usr/bin/env python

#
# Script inspired in bud:
#   https://github.com/indutny/bud
#

import platform
import os
import subprocess
import sys

CC = os.environ.get('CC', 'cc')
script_dir = os.path.dirname(__file__)
root = os.path.normpath(os.path.join(script_dir, '..'))
output_dir = os.path.join(os.path.abspath(root), 'out')

sys.path.insert(0, os.path.join(root, 'deps', 'gyp', 'pylib'))
try:
    import gyp
except ImportError:
    print('Error: you need to install gyp in deps/gyp first, run:')
    print('  ./scripts/get-dep.sh gyp')
    sys.exit(42)

def host_arch():
    machine = platform.machine()
    if machine == 'i386': return 'ia32'
    if machine == 'x86_64': return 'x64'
    if machine == 'aarch64': return 'arm64'
    if machine.startswith('arm'): return 'arm'
    if machine.startswith('mips'): return 'mips'
    return machine  # Return as-is and hope for the best.

def compiler_version():
    proc = subprocess.Popen(CC.split() + ['--version'], stdout=subprocess.PIPE)
    is_clang = b'clang' in proc.communicate()[0].split(b'\n')[0]
    proc = subprocess.Popen(CC.split() + ['-dumpversion'], stdout=subprocess.PIPE)
    version = proc.communicate()[0].split(b'.')
    mayor_version = int(version[:1][0])
    if is_clang is False and mayor_version >= 7:
        proc = subprocess.Popen(CC.split() + ['-dumpfullversion'], stdout=subprocess.PIPE)
        version = proc.communicate()[0].split(b'.')
    version = map(int, version[:2])
    version = tuple(version)
    return (version, is_clang)

def run_gyp(args):
    rc = gyp.main(args)
    if rc != 0:
        print('Error running GYP')
        sys.exit(rc)

if __name__ == '__main__':
    args = sys.argv[1:]

    # GYP bug.
    # On msvs it will crash if it gets an absolute path.
    # On Mac/make it will crash if it doesn't get an absolute path.
    # NOTE ibc: Not sure that it requires absolute path in Mac/make...
    if sys.platform == 'win32':
        args.append(os.path.join(root, 'mediasoup-worker.gyp'))
        common_fn  = os.path.join(root, 'common.gypi')
        # we force vs 2010 over 2008 which would otherwise be the default for gyp.
        if not os.environ.get('GYP_MSVS_VERSION'):
            os.environ['GYP_MSVS_VERSION'] = '2010'
    else:
        args.append(os.path.join(os.path.abspath(root), 'mediasoup-worker.gyp'))
        common_fn  = os.path.join(os.path.abspath(root), 'common.gypi')

    if os.path.exists(common_fn):
        args.extend(['-I', common_fn])

    args.append('--depth=' + root)

    # There's a bug with windows which doesn't allow this feature.
    if sys.platform != 'win32':
        if '-f' not in args:
            args.extend('-f make'.split())
        if 'ninja' not in args:
            args.extend(['-Goutput_dir=' + output_dir])
            args.extend(['--generator-output', output_dir])
        (major, minor), is_clang = compiler_version()
        args.append('-Dgcc_version=%d' % (10 * major + minor))
        args.append('-Dclang=%d' % int(is_clang))
        if is_clang is False and major == 4 and minor <= 8:
            raise RuntimeError('gcc <= 4.8 not supported, please upgrade your gcc')

    if not any(a.startswith('-Dhost_arch=') for a in args):
        args.append('-Dhost_arch=%s' % host_arch())

    if not any(a.startswith('-Dtarget_arch=') for a in args):
        args.append('-Dtarget_arch=%s' % host_arch())

    if any(a.startswith('-Dopenssl_fips=') for a in args):
        fips_fn = os.path.join(os.path.abspath(root), 'fips.gypi')
        args.extend(['-I', fips_fn])
    else:
        args.append('-Dopenssl_fips=')

    if 'asan' in args:
        args.append('-Dmediasoup_asan=true')
        args = filter(lambda arg: arg != 'asan', args)
    else:
        args.append('-Dmediasoup_asan=false')

    args.append('-Dnode_byteorder=' + sys.byteorder)

    gyp_args = list(args)
    print(gyp_args)
    run_gyp(gyp_args)
