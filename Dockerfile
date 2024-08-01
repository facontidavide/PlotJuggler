FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
  build-essential \
  cmake \
  libbfd-dev \
  libdwarf-dev \
  libzmq3-dev \
  libdw-dev \
  python3 \
  python3-dev \
  python3-pip \
  python-is-python3 \
  wget \
  software-properties-common \

  qt5-default \
  qtbase5-dev \
  libqt5svg5-dev \
  libqt5websockets5-dev \
  libqt5opengl5-dev \
  libqt5x11extras5-dev \

  # opendbc/cereal
  capnproto \
  curl \
  git \
  python-openssl \
  libbz2-dev \
  libcapnp-dev \
  libssl-dev \
  libffi-dev \
  libreadline-dev \
  libsqlite3-dev \
  clang \
  ocl-icd-opencl-dev \
  opencl-headers

RUN curl -L https://github.com/pyenv/pyenv-installer/raw/master/bin/pyenv-installer | bash
ENV PATH="/root/.pyenv/bin:/root/.pyenv/shims:${PATH}"
RUN pyenv install 3.11.4 && \
    pyenv global 3.11.4 && \
    pyenv rehash

RUN pip3 install pkgconfig jinja2

# installs scons, pycapnp, cython, etc.
ENV PYTHONPATH /tmp/plotjuggler/3rdparty
COPY 3rdparty/opendbc/requirements.txt /tmp/
RUN pip3 install Cython && pip3 install --no-cache-dir -r /tmp/requirements.txt
