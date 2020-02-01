#!/bin/bash
# Builds (if needed) and runs a docker image for building this project.

set -eo pipefail
cd "$(dirname "$0")"

DOCKER_DIR="${PWD}/.docker"

ID_FILE="${DOCKER_DIR}/image_id"
U_HOME="${DOCKER_DIR}/user"

mkdir -p "${U_HOME}"

echo "Initializing..."

if [ "${ID_FILE}" -nt Dockerfile ]; then
  # '-nt' also checks if it even exists
  IMG_ID=$(cat "${ID_FILE}")

  # Let's make sure that image actually exists and is usable.
  # If it's not, let's unset the IMG_ID
  docker run -it --rm ${IMG_ID} true 2>/dev/null || IMG_ID=""
fi

if [ -z "${IMG_ID}" ]; then
  echo "(Re)generating the docker image..."
  rm -f "${ID_FILE}"

  docker build --build-arg UID=$(id -u) --build-arg GID=$(id -g) --iidfile "${ID_FILE}" .
  IMG_ID=$(cat "${ID_FILE}")

  # We will be mounting the entire .docker/user as user's home directory.
  # Let's populate that directory with files in /home/user inside the image.
  docker run -it -v "${U_HOME}:/tmp/user/" --rm ${IMG_ID} \
    /bin/bash -c "cp /home/user/.* /tmp/user/ &>/dev/null" || true
fi

CMD="$*"
[ "${CMD}" = "" ] && CMD="/bin/bash"

echo "Running inside the image..."

# We pass '-rm' to remove the container when it exits.
# All the data should be stored either in .docker/user or in the project's directory.
docker run -it -v "${PWD}:/project" -v "${U_HOME}:/home/user/" -w /project --rm ${IMG_ID} ${CMD}
