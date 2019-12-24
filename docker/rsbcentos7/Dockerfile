FROM centos:centos7
RUN yum update -y && \
    yum install -y centos-release-scl && \
    yum install -y \
	autoconf \
	automake \
	boost-devel \
	cairomm-devel \
	devtoolset-7 \
	make \
	pangomm-devel \
	python3-devel \
	python3-pip \
    	git \
    	sqlite-devel \
	&& \
    yum clean all
RUN pip3 install xattr
ADD build /build
VOLUME /src
WORKDIR /src
CMD /build
