# Create a virtual environment with all tools installed
# ref: https://quay.io/repository/centos/centos
FROM quay.io/centos/centos:stream AS env
# Install system build dependencies
ENV PATH=/usr/local/bin:$PATH
RUN dnf -y update \
&& dnf -y install git wget gcc-toolset-11 \
&& dnf clean all \
&& rm -rf /var/cache/dnf

RUN echo "source /opt/rh/gcc-toolset-11/enable" >> /etc/bashrc
SHELL ["/bin/bash", "--login", "-c"]

# Install bazel 5
# see: https://docs.bazel.build/versions/main/install-redhat.html#installing-on-centos-7
RUN dnf config-manager --add-repo \
https://copr.fedorainfracloud.org/coprs/vbatts/bazel/repo/epel-8/vbatts-bazel-epel-8.repo \
&& dnf -y install bazel5 \
&& dnf clean all \
&& rm -rf /var/cache/dnf

# Install Python
RUN dnf -y update \
&& dnf -y install python39-devel \
&& dnf clean all \
&& rm -rf /var/cache/dnf

FROM env AS devel
WORKDIR /home/project
COPY . .

FROM devel AS build
RUN bazel version
RUN bazel build -s -c opt --cxxopt=-std=c++17 ...

FROM build AS test
RUN bazel test -c opt --cxxopt=-std=c++17 --test_output=errors ortools/... examples/...
