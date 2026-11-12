FROM ubuntu:26.04

# Install dependencies.
RUN set -x \
	&& apt-get update \
	&& apt-get install --yes \
		clang-21 make pkg-config bash-completion wget curl git screen python3-pip python3-yaml \
		zlib1g-dev libgss-dev libssl-dev libxml2-dev gdb procps file gnupg

# Install clang-format 23 and clang-tidy 23 from apt.llvm.org since Ubuntu
# does not ship them.
RUN set -x \
	&& curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor -o /usr/share/keyrings/llvm.gpg \
	&& echo "deb [signed-by=/usr/share/keyrings/llvm.gpg] http://apt.llvm.org/resolute/ llvm-toolchain-resolute-23 main" > /etc/apt/sources.list.d/llvm.list \
	&& apt-get update \
	&& apt-get install --yes clang-format-23 clang-tidy-23

# Install node 26.
RUN curl -fsSL https://deb.nodesource.com/setup_26.x | bash - \
	&& apt-get install --yes nodejs

# Enable core dumps.
RUN set -x \
	&& echo "mkdir -p /tmp/cores && chmod 777 /tmp/cores && echo \"/tmp/cores/core.%e.sig%s.%p\" > /proc/sys/kernel/core_pattern && ulimit -c unlimited" >> ~/.bashrc

# Make CC and CXX point to clang/clang++ installed above.
ENV LANG="C.UTF-8"
ENV CC="clang-21"
ENV CXX="clang++-21"

ENV MEDIASOUP_LOCAL_DEV="true"

WORKDIR "/foo bar/mediasoup"

CMD ["bash"]
