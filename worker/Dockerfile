FROM ubuntu:25.10

# Install dependencies.
RUN set -x \
	&& apt-get update \
	&& apt-get install --yes \
		clang make pkg-config bash-completion wget curl screen python3-pip python3-yaml \
		zlib1g-dev libgss-dev libssl-dev libxml2-dev gdb clang-format-21

# Install node 24.
RUN curl -fsSL https://deb.nodesource.com/setup_24.x | bash - \
	&& apt-get install --yes nodejs

# Enable core dumps.
RUN set -x \
	&& echo "mkdir -p /tmp/cores && chmod 777 /tmp/cores && echo \"/tmp/cores/core.%e.sig%s.%p\" > /proc/sys/kernel/core_pattern && ulimit -c unlimited" >> ~/.bashrc

# Make CC and CXX point to clang/clang++ installed above.
ENV LANG="C.UTF-8"
ENV CC="clang"
ENV CXX="clang++"

ENV MEDIASOUP_LOCAL_DEV="true"
ENV KEEP_BUILD_ARTIFACTS="1"

WORKDIR "/foo bar/mediasoup"

CMD ["bash"]
