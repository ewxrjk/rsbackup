FROM centos:centos7
RUN yum update -y && \
    yum install -y centos-release-scl && \
    yum install -y git autoconf automake \
    	sqlite-devel cairomm-devel pangomm-devel make boost-devel \
	devtoolset-7 python3-pip python3-devel && \
    yum clean all
RUN pip3 install xattr
ADD build /build
VOLUME /src
WORKDIR /src
CMD /build
