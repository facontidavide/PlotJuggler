#!/bin/bash -e

docker build -f Dockerfile -t plotjuggler:latest .

docker run \
  --rm \
  --volume $PWD:/tmp/plotjuggler \
  --workdir /tmp/plotjuggler plotjuggler:latest \
  /bin/bash -c "source /.venv/bin/activate && mkdir -p build && cd build && cmake .. && make -j$(nproc)"
