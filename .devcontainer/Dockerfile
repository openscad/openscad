# Dockerfile for Development Container.

# Debian 12 (Bookworm) base image.
FROM mcr.microsoft.com/devcontainers/base:bookworm

# Essential dependencies.
COPY ./uni-get-dependencies.sh /tmp/
RUN /tmp/uni-get-dependencies.sh

# Debuggers.
RUN apt-get -y install gdb lldb

# Dependencies for tests (as documented in `doc/testing.md`).
RUN apt-get -y install cmake python3 python3-venv python3-pip
