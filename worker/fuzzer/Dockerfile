FROM ubuntu:16.04

# Install dependencies.
RUN \
	set -x \
	&& apt-get update \
	&& apt-get install --yes \
	   wget curl subversion screen gcc g++ cmake ninja-build golang autoconf \
	   libtool apache2 python-dev pkg-config zlib1g-dev libgcrypt11-dev \
	   libgss-dev libssl-dev libxml2-dev ragel nasm libarchive-dev make \
	   automake libdbus-1-dev libboost-dev autoconf-archive

WORKDIR /mediasoup/worker

# Install clang 7.0.0.
COPY deps/clang-fuzzer/bin /usr/local/bin
COPY deps/clang-fuzzer/lib/clang /usr/local/lib/clang

# Copy mediasoup source code.
COPY src src
COPY include include
COPY deps deps
COPY scripts scripts
COPY fuzzer fuzzer
COPY Makefile Makefile
COPY mediasoup-worker.gyp mediasoup-worker.gyp
COPY common.gypi common.gypi
COPY fips.gypi fips.gypi

ENV LANG="C.UTF-8"
ENV CC="clang"
ENV CXX="clang++"

# CMD ["/bin/bash"]
CMD ["make", "fuzzer-run"]
