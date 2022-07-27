FROM ubuntu:22.04

# Install dependencies.
RUN \
	set -x \
	&& apt-get update \
	&& apt-get install --yes \
	   bash-completion wget curl subversion screen gcc g++ cmake ninja-build golang \
	   autoconf libtool apache2 python3-pip python3-dev pkg-config zlib1g-dev \
	   libgss-dev libssl-dev libxml2-dev nasm libarchive-dev make automake \
	   libdbus-1-dev libboost-dev autoconf-archive bash-completion python3-yaml \
	   clang

# Install node 16.
RUN curl -sL https://deb.nodesource.com/setup_16.x | bash -
RUN apt-get install -y nodejs

# Make CC and CXX point to clang/clang++ installed above.
ENV LANG="C.UTF-8"
ENV CC="clang"
ENV CXX="clang++"

WORKDIR /mediasoup

CMD ["bash"]
