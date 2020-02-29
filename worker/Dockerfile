FROM ubuntu:16.04

# Install dependencies.
RUN \
	set -x \
	&& apt-get update \
	&& apt-get install --yes \
	   bash-completion wget curl subversion screen gcc g++ cmake ninja-build golang \
	   autoconf libtool apache2 python-dev pkg-config zlib1g-dev libgcrypt11-dev \
	   libgss-dev libssl-dev libxml2-dev nasm libarchive-dev make bear automake \
	   libdbus-1-dev libboost-dev autoconf-archive bash-completion python-yaml

# Install node 10.
RUN curl -sL https://deb.nodesource.com/setup_10.x | bash -
RUN apt-get install -y nodejs

# Install clang 7.0.0.
COPY deps/clang-fuzzer/bin /usr/local/bin
COPY deps/clang-fuzzer/lib/clang /usr/local/lib/clang

# Make CC and CXX point to clang/clang++ installed above.
ENV LANG="C.UTF-8"
ENV CC="clang"
ENV CXX="clang++"

WORKDIR /mediasoup

CMD ["bash"]
