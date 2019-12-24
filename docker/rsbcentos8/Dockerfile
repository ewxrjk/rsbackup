FROM centos:centos8
RUN dnf install -y 'dnf-command(config-manager)'
RUN dnf config-manager --set-enabled PowerTools
RUN yum update -y && \
    yum install -y \
    	git \
	autoconf \
	automake \
    	sqlite-devel \
	cairomm-devel \
	pangomm-devel \
	make \
	boost-devel \
	gcc-c++ \
	python3-pip \
	python3-devel \
	rsync \
	&& \
    yum clean all
RUN pip3 install xattr
ADD build /build
VOLUME /src
WORKDIR /src
CMD /build
