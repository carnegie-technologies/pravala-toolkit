# A Dockerfile for creating a build environment for this project.
# Build arguments:
# - UID - the id of the user (defaults to 1000)
# - GID - the group id of the user (defaults to 1000)

FROM ubuntu:bionic

RUN apt-get -q update &&\
    DEBIAN_FRONTEND="noninteractive" apt-get -q install -y \
        -o Dpkg::Options::="--force-confnew" --no-install-recommends \
        curl unzip git patch \
        openjdk-8-jdk-headless &&\
    apt-get -q clean -y &&\
    rm -rf /var/lib/apt/lists/* &&\
    rm -f /var/cache/apt/*.bin

ENV JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64

# Download and unpack sdkmanager and install components for building Android apps.
# Also create an empty /root/.android/repositories.cfg to avoid warnings.
# We pass 'yes' to sdkmanager to agree to license terms.
#
# Specific project requirements (as of 2019-11-12):
#  - build-tools;28.0.3
#  - platforms;android-28
#  - platforms;android-23 (28 on its own is not sufficient)
#  - cmake;3.6.4111459
#  - ndk;20.1.5948944
#
RUN mkdir -p /usr/local/android-sdk-linux /root/.android &&\
    touch /root/.android/repositories.cfg &&\
    curl -O https://dl.google.com/android/repository/sdk-tools-linux-4333796.zip &&\
    echo "Extracting..." &&\
    unzip -qd /usr/local/android-sdk-linux sdk-tools-linux-4333796.zip &&\
    rm sdk-tools-linux-4333796.zip &&\
    yes | /usr/local/android-sdk-linux/tools/bin/sdkmanager \
      "platform-tools" \
      "tools" \
      "build-tools;28.0.3" \
      "platforms;android-23" \
      "platforms;android-28" \
      "cmake;3.6.4111459" \
      "ndk;20.1.5948944"

ENV ANDROID_HOME=/usr/local/android-sdk-linux

ARG UID=1000
ARG GID=1000

RUN echo "Using UID:${UID} and GID:${GID} for internal user" &&\
    groupadd -g ${GID} user &&\
    useradd -m -u ${UID} -g user user

USER ${UID}
