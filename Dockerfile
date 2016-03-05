FROM takawang/ubuntu:vl
MAINTAINER Taka Wang <taka@cmwang.net>
 
## Set environment variable
ENV DEBIAN_FRONTEND noninteractive
ENV MB_GIT https://github.com/stephane/libmodbus.git
ENV MOSQUITTO_VERSION 1.4.8

RUN apt-get update
RUN apt-get install -y wget git build-essential libssl-dev libwrap0-dev libc-ares-dev uuid-dev automake libtool cmake

## Install mosquitto
RUN wget http://mosquitto.org/files/source/mosquitto-${MOSQUITTO_VERSION}.tar.gz && \
    tar zxvf mosquitto-${MOSQUITTO_VERSION}.tar.gz && \
    cd mosquitto-${MOSQUITTO_VERSION} && \
    make all && make install && ldconfig && \
    cd .. && rm -rf mosquitto*

## Install libmodbus
RUN cd / && \
    git clone $MB_GIT && \
    cd libmodbus && \
    ./autogen.sh && \
    ./configure --prefix=/usr && \
    make && make install && ldconfig && \
    cd .. && rm -rf libmodbus*

## Build dds-modbus
COPY . /dds-modbus/
RUN mkdir -p /dds-modbus/build && \
    cd /dds-modbus/build && \
    cmake .. && make \
    #&& make install

#CMD /usr/bin/master & /usr/bin/bridge