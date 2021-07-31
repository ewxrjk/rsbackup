FROM fedora:34
RUN yum update -y && \
	yum install -y \
	acl \
	autoconf \
	automake \
	boost-devel \
	cairomm-devel \
	diffutils \
	findutils \
	gcc-c++ \
	git \
	make \
	pangomm-devel \
	python3-devel \
	python3-pip \
	rsync \
	sqlite-devel \
	&& \
	yum clean all
RUN pip3 install xattr
ADD build /build
VOLUME /src
WORKDIR /src
CMD /build
