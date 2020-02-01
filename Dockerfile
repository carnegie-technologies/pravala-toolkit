# A Dockerfile for creating a build environment for this project.
# Build arguments:
# - UID - the id of the user (defaults to 1000)
# - GID - the group id of the user (defaults to 1000)

FROM ubuntu:bionic

RUN apt-get -q update &&\
    DEBIAN_FRONTEND="noninteractive" apt-get -q install -y \
        -o Dpkg::Options::="--force-confnew" --no-install-recommends \
        build-essential pkg-config git cmake bison flex ninja-build \
        libssl-dev libcurl4-openssl-dev libgtest-dev &&\
    apt-get -q clean -y &&\
    rm -rf /var/lib/apt/lists/* &&\
    rm -f /var/cache/apt/*.bin

ARG UID=1000
ARG GID=1000

RUN echo "Using UID:${UID} and GID:${GID} for internal user" &&\
    groupadd -g ${GID} user &&\
    useradd -m -u ${UID} -g user user

USER ${UID}
