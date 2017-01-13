"""Builds the download page based on the released files.

This is just a little helper to execute after building all the release files. It will generate the contents of a wiki page that I can upload to wildbear to finish off a release.
"""

import hashlib
import sys

RELEASE_LISTING = [
    'fct.h',
    'fctx-doc-%(ver)s.tar.gz',
    'fctx-src-%(ver)s.tar.gz',
    'patch-%(ver)s.gz',
    'README.rst',
    'NEWS-%(ver)s',
    'diffstat-%(ver)s',
    'ChangeLog-%(ver)s'
]

fname_args = {'ver':sys.argv[1]}

print "== %(ver)s (Stable) ==" % (fname_args)
print "||<tablestyle=\"align:right;margin:2em;\">'''Files''' ||<:>'''md5'''||"

for l in RELEASE_LISTING:
    m = hashlib.md5()
    fname = l % (fname_args)
    m.update(open(fname).read())
    digest = m.hexdigest()
    args = fname_args.copy()
    args["fname"] = fname
    args["digest"] = digest
    print "|| [[http://fctx.wildbearsoftware.com/static/fctx/download/%(ver)s/%(fname)s|%(fname)s]] || %(digest)s ||" % args

print
print
print "=== Release Notes ==="
print
release_lines = open('NEWS-%(ver)s' % args).readlines()
notes = []
start_parse = False
for line in release_lines:
    # First underline in first release note section
    if line.find(sys.argv[1]) >= 0:
        start_parse = True
    elif line.startswith("Whats"):
        start_parse = False
    elif start_parse and not line.startswith("-----"):
        notes.append(line)

chunks =[]
chunk =""
for line in notes:
    if line.strip().startswith("-"):
        if bool(chunk):
           chunks.append(chunk + "\n")
        chunk = " " + line.replace(" - ", " * ").strip() + " " 
    else:
        chunk += line.strip() + " "
chunks.append(chunk)

print "".join(chunks)
