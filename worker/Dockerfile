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

# Install node 20.
 RUN set -x \
	 && apt-get update \
	 && apt-get install --yes ca-certificates curl gnupg \
	 && mkdir -p /etc/apt/keyrings \
	 && curl -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key \
	 | gpg --dearmor -o /etc/apt/keyrings/nodesource.gpg \
	 && NODE_MAJOR=20 \
	 && echo "deb [signed-by=/etc/apt/keyrings/nodesource.gpg] https://deb.nodesource.com/node_$NODE_MAJOR.x nodistro main" \
	 > /etc/apt/sources.list.d/nodesource.list \
	 && apt-get update \
	 && apt-get install nodejs --yes

# Make CC and CXX point to clang/clang++ installed above.
ENV LANG="C.UTF-8"
ENV CC="clang"
ENV CXX="clang++"

WORKDIR /mediasoup

CMD ["bash"]
