#!/bin/bash
#
# A script that checks line lengths.
#
# Arguments (both are optional, but should be in this order):
# -e     : Exit after the first invalid file has been found.
# number : The max line length to use. By default 120 is used.
#

if [[ "${1}" == "-e" ]]; then
  EXIT_ON_ERROR=1
  shift
else
  EXIT_ON_ERROR=0
fi

if [[ -z "${1}" ]]; then
  MAX_LINE_LENGTH=120
elif [[ "${1}" =~ ^[1-9]+[0-9]*$ ]]; then
  MAX_LINE_LENGTH=${1}
else
  echo "'${1}' is not a valid line length"
  exit 1
fi

IGNORE_FILE=".styleignore"

# Read patterns that we should ignore:
if [[ -f ${IGNORE_FILE} ]]; then
  # We need to remove \r which may be added on Windows.
  # Otherwise patterns include that \r and don't match properly.
  IGN_PATTERNS=$(cat ${IGNORE_FILE} | tr -d '\r')
  N=$(echo "${IGN_PATTERNS}" | wc -l)
  echo "Using ${N} ignore pattern(s) from '${IGNORE_FILE}'"
else
  IGN_PATTERNS=
fi

# Now let's find files to format

echo "Examining files in ${PWD}..."

IFS=$(echo -en "\n\b")

GIT_FILES=$(git ls-files '*.h' '*.c' '*.hpp' '*.cpp' '*.java' '*.js' '*.ts' \
  '*.cs' '*.txt' '*.md' '*.sh' '*.bash' '*.m' '*.mm' '*.py' '*.php' '*.less' \
  '*.vue', '*.css', '*.scss')

# We will go over all the files found, filter out those to ignore and check the rest.

# We need this for grep to handle UTF-8 files properly:
export LC_CTYPE=en_US.utf8

RESULT=0
MAX_PLUS_ONE=$((${MAX_LINE_LENGTH} + 1))

for f in ${GIT_FILES}; do
  if [[ ! -f "${f}" || -L "${f}" ]]; then
    continue
  fi
  IGN_MATCH=0
  for p in ${IGN_PATTERNS}; do
    [[ "${f}" =~ ${p} ]] && IGN_MATCH=1 && break
  done

  if [[ ${IGN_MATCH} -eq 0 ]]; then
    OUT=$(grep -nH ".\{${MAX_PLUS_ONE}\}" "${f}")
    if [[ -n "${OUT}" ]]; then
      if [[ ${RESULT} -eq 0 ]]; then
        echo
        echo "Too long (>${MAX_LINE_LENGTH} characters) lines found:"
        echo
        RESULT=1
      fi
      echo "${OUT}"
      if [[ ${EXIT_ON_ERROR} -ne 0 ]]; then
        exit 1
      fi
    fi
  fi
done

if [[ ${RESULT} -eq 0 ]]; then
  echo
  echo "No problems found!"
  exit 0
fi

exit ${RESULT}
