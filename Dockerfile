FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
  build-essential \
  cmake \  
  qtbase5-dev \
  libqt5svg5-dev \
  libqt5websockets5-dev \
  libqt5opengl5-dev \
  libqt5x11extras5-dev \
  libbfd-dev \
  libdwarf-dev \
  libdw-dev \
  libbz2-dev \
  libcapnp-dev \
  python3 \
  python3-pip
RUN pip3 install jinja2
ENV PYTHONPATH /tmp/plotjuggler/3rdparty