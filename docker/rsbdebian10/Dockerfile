FROM debian:buster
RUN apt-get update -y && \
    apt-get install -y --no-install-recommends \
	    acl \
	    autoconf \
	    automake \
	    git \
	    libboost-dev \
	    libboost-filesystem-dev \
	    libboost-system-dev \
	    libcairomm-1.0-dev \
	    libpangomm-1.4-dev \
	    libsqlite3-dev \
	    lynx \
	    rsync \
	    sqlite3 \
	    xattr \
    	    build-essential \
	    && \
    apt-get clean
ADD build /build
VOLUME /src
WORKDIR /src
CMD /build
