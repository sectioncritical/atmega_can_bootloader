# Notes about base image
#
# We need to use bullseye because we need the versions of
# doxygen, gcc-avr, and cppcheck that are included. If we use buster, then
# those utilities do not propertly support the ATtiny1614.

FROM python:3.10-slim-bullseye
WORKDIR /project

# base tools
RUN apt-get update          \
    && apt-get install -y   \
        avr-libc            \
        build-essential     \
        cppcheck            \
        curl                \
        doxygen             \
        gcc-avr             \
        git                 \
        make                \
        pkg-config          \
        unzip zip           \
    && rm -rf /var/lib/apt/lists/*

CMD ["/bin/bash"]
