#! /bin/bash
#
# openscad-docker-build.sh -- a simple script to compile openscad using a docker container.
#
# (C) 2018 jnweiger@gmail.com
# Distribute under GPL-2.0 or ask.

test -d openscad || git clone --depth=50 --branch=master https://github.com/openscad/openscad.git openscad
( cd openscad; \
  git submodule update --init --recursive; \
  cd submodules/manifold; \
  git apply thrust.diff )

mkdir -p docker
cat <<'EOF'> docker/Dockerfile
# openscad build env.
FROM ubuntu:16.04
RUN apt-get update -y; apt-get install -y git gcc

ENV CXX=g++
ENV CC=gcc

RUN $CC --version

RUN apt-get install -qq sudo; echo >> /etc/sudoers 'ALL ALL=(ALL) NOPASSWD: ALL'
EOF

if [ "$(md5sum docker/Dockerfile)" != "$(cat docker/Dockerfile.md5 2>/dev/null)" ]; then
  docker build -t openscad-build docker
  md5sum docker/Dockerfile > docker/Dockerfile.md5
fi

# Continue docker build outside the Dockerfile.
# This is needed, as docker build cannot have volumes and cannot react to changed github contents.
RUN()
{
  rm -f docker/cid 
  docker run --cidfile docker/cid -v $PWD/openscad:/github/openscad -v /etc/passwd:/etc/passwd:ro --user $(stat -c "%u:%g" openscad) --workdir /github/openscad openscad-build "$@"
}
commit() { docker commit $(cat docker/cid) openscad-build; }

RUN sudo ./scripts/uni-get-dependencies.sh
commit
RUN bash -c ./scripts/check-dependencies.sh
RUN qmake openscad.pro
RUN make

ls -la openscad/openscad	# should show the final binary...
RUN sudo checkinstall -D make install
