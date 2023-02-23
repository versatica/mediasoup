import io, hashlib, tarfile, urllib.request

def get(url, digest):
  data = urllib.request.urlopen(url).read()
  assert hashlib.sha256(data).hexdigest() == digest
  tar = tarfile.open(fileobj=io.BytesIO(data))
  tar.extractall('worker/out/msys')
  tar.close()

get('https://sourceforge.net/projects/mingw/files/MSYS/Base/msys-core/msys-1.0.19-1/msysCORE-1.0.19-1-msys-1.0.19-bin.tar.xz/download', '8c4157d739a460f85563bc4451e9f1bbd42b13c4f63770d43b9f45a781f07858')
get('https://sourceforge.net/projects/mingw/files/MSYS/Base/libiconv/libiconv-1.14-1/libiconv-1.14-1-msys-1.0.17-dll-2.tar.lzma/download', '196921e8c232259c8e6a6852b9ee8d9ab2d29a91419f0c8dc27ba6f034231683')
get('https://sourceforge.net/projects/mingw/files/MSYS/Base/gettext/gettext-0.18.1.1-1/libintl-0.18.1.1-1-msys-1.0.17-dll-8.tar.lzma/download', '29db8c969661c511fbe2a341ab25c993c5f9c555842a75d6ddbcfa70dec16910')
get('https://sourceforge.net/projects/mingw/files/MSYS/Base/coreutils/coreutils-5.97-3/coreutils-5.97-3-msys-1.0.13-bin.tar.lzma/download', 'f8c7990416ea16a74ac336dcfe0f596bc46b8724b2d58cf8a3509414220b2366')
get('https://sourceforge.net/projects/mingw/files/MSYS/Base/regex/regex-1.20090805-2/libregex-1.20090805-2-msys-1.0.13-dll-1.tar.lzma/download', '85dd8c1e27a90675c5f867be57ba7ae2bb55dde8cd2d19f284c896be134bd3d1')
get('https://sourceforge.net/projects/mingw/files/MSYS/Base/termcap/termcap-0.20050421_1-2/libtermcap-0.20050421_1-2-msys-1.0.13-dll-0.tar.lzma/download', '62b58fe0880f0972fcc84a819265989b02439c1c5185870227bd25f870f7adb6')
get('https://sourceforge.net/projects/mingw/files/MSYS/Base/bash/bash-3.1.23-1/bash-3.1.23-1-msys-1.0.18-bin.tar.xz/download', '38da5419969ab883058a96322bb0f51434dd4e9f71de09cd4f75b96750944533')
get('https://sourceforge.net/projects/mingw/files/MSYS/Base/make/make-3.81-3/make-3.81-3-msys-1.0.13-bin.tar.lzma/download', '847f0cbbf07135801c8e67bf692d29b1821e816ad828753c997fa869a9b89988')
