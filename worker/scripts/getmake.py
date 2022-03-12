import io, hashlib, tarfile, urllib.request

def get(url, sha1):
  data = urllib.request.urlopen(url).read()
  assert hashlib.sha1(data).hexdigest() == sha1
  tar = tarfile.open(fileobj=io.BytesIO(data))
  tar.extractall('worker/out/msys')
  tar.close()

get('https://sourceforge.net/projects/mingw/files/MSYS/Base/msys-core/msys-1.0.19-1/msysCORE-1.0.19-1-msys-1.0.19-bin.tar.xz/download', '9200450ad3df8c83be323c9b14ae344d5c1ca784')
get('https://sourceforge.net/projects/mingw/files/MSYS/Base/libiconv/libiconv-1.14-1/libiconv-1.14-1-msys-1.0.17-dll-2.tar.lzma/download', '056d16bfb7a91c3e3b1acf8adb20edea6fceecdd')
get('https://sourceforge.net/projects/mingw/files/MSYS/Base/gettext/gettext-0.18.1.1-1/libintl-0.18.1.1-1-msys-1.0.17-dll-8.tar.lzma/download', '4000b935a5bc30b4c757fde69d27716fa3c2c269')
get('https://sourceforge.net/projects/mingw/files/MSYS/Base/coreutils/coreutils-5.97-3/coreutils-5.97-3-msys-1.0.13-bin.tar.lzma/download', '54ac256a8f0c6a89f1b3c7758f3703b4e56382be')
get('https://sourceforge.net/projects/mingw/files/MSYS/Base/regex/regex-1.20090805-2/libregex-1.20090805-2-msys-1.0.13-dll-1.tar.lzma/download', 'd95faa144cf06625b3932a8e84ed1a6ab6bbe644')
get('https://sourceforge.net/projects/mingw/files/MSYS/Base/termcap/termcap-0.20050421_1-2/libtermcap-0.20050421_1-2-msys-1.0.13-dll-0.tar.lzma/download', 'e4273ccfde8ecf3a7631446fb2b01971a24ff9f7')
get('https://sourceforge.net/projects/mingw/files/MSYS/Base/bash/bash-3.1.23-1/bash-3.1.23-1-msys-1.0.18-bin.tar.xz/download', 'b6ef3399b8d76b5fbbd0a88774ebc2a90e8af13a')
get('https://sourceforge.net/projects/mingw/files/MSYS/Base/make/make-3.81-3/make-3.81-3-msys-1.0.13-bin.tar.lzma/download', 'c7264eb13b05cf2e1a982a3c2619837b96203a27')
