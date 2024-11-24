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
  opencl-headers \
  portaudio19-dev

RUN curl -LsSf https://astral.sh/uv/install.sh | sh
ENV PATH="/root/.local/bin:${PATH}"
RUN uv venv --python 3.11.4
RUN uv pip install pkgconfig jinja2 Cython

ENV PYTHONPATH /tmp/plotjuggler/3rdparty
COPY 3rdparty/openpilot/opendbc_repo/pyproject.toml /tmp/opendbc_repo/pyproject.toml
COPY 3rdparty/openpilot/pyproject.toml /tmp/openpilot/pyproject.toml
RUN uv pip install --no-cache-dir /tmp/opendbc_repo
RUN uv pip install --no-cache-dir /tmp/openpilot
