#!/bin/bash
#
# Auto-formatting script.
#
# It will automatically format all C/C++, Java, C#, and ObjC/ObjC++
# files in or below the current directory.
# It will ignore all files that match .styleignore file found in the current directory.
#
# It does not use any arguments,
# but it uses following environment variables:
#
# AUTOFORMAT_MAX_TASKS
# The max number of tasks to run in parallel. 1 by default.
#
# AUTOFORMAT_MAX_FILES_PER_TASK
# The max number of files to be processed by a single task.
# Unlimited by default.
#
# More languages could be added as needed.
#
# Arguments:
# --all       Formats all supported files. Normally this script automatically
#             formats only those files, whose formatters are relatively quick.
#             Passing this option enables also the slow and/or optional formatters.

set -eo pipefail

IGNORE_FILE=".styleignore"
MY_DIR=$(dirname "${BASH_SOURCE[0]}")

if [ "${1}" = "--all" ]; then
  INCLUDE_OPT=1
else
  INCLUDE_OPT=0
fi

# The max number of tasks to run in parallel.
if [[ -n "${AUTOFORMAT_MAX_TASKS}" ]]; then
  XARG_OPTS="-P ${AUTOFORMAT_MAX_TASKS}"
else
  XARG_OPTS="-P 1"
fi

# The max number of files per task.
if [[ -n "${AUTOFORMAT_MAX_FILES_PER_TASK}" ]]; then
  XARG_OPTS="${XARG_OPTS} -n ${AUTOFORMAT_MAX_FILES_PER_TASK}"
fi

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

echo "Finding files in ${PWD}..."

# We only want to format regular files.
# The test we do later [ -L file ] won't work on Windows, because git on Windows
# creates regular files with the target path stored inside.
# So we need to make git ls-files print attributes, and only select regular files.
# Those are the files whose mode starts with '10'.
# Following function only prints names of the files that use that mode.
# Also, for 'read' to work properly, we cannot modify IFS until after running that function.

function filter_files() {
  while read A B C D; do
    [[ "${A}" == 10* ]] && echo "${D}"
  done
}

GIT_FILES=$(git ls-files -s '*.h' '*.c' '*.hpp' '*.cpp' '*.java' '*.cs' '*.m' '*.mm' '*.hh' \
  '*.sh' '*.bash' '*.md' \
  | filter_files)

ORG_IFS=${IFS}
IFS=$'\n'

# We will go over all the files found, filter out those to ignore
# and add the rest to one of the arrays - depending on file's extension.

FILES_CPP=""
FILES_CS=""
FILES_JAVA=""
FILES_OBJC=""
FILES_SH=""
FILES_PLAIN=""
FILES_MD=""

NUM_CPP=0
NUM_CS=0
NUM_JAVA=0
NUM_OBJC=0
NUM_SH=0
NUM_PLAIN=0
NUM_MD=0

for f in ${GIT_FILES}; do
  if [[ ! -f "${f}" || -L "${f}" ]]; then
    continue
  fi
  IGN_MATCH=0
  for p in ${IGN_PATTERNS}; do
    [[ "${f}" =~ ${p} ]] && IGN_MATCH=1 && break
  done

  if [[ ${IGN_MATCH} -eq 0 ]]; then
    if [[ "${f}" == *.java ]]; then
      FILES_JAVA+="${f}"$'\n'
      ((++NUM_JAVA))
    elif [[ "${f}" == *.cs ]]; then
      FILES_CS+="${f}"$'\n'
      ((++NUM_CS))
    elif [[ "${f}" == *.m || "${f}" == *.mm || "${f}" == *.hh ]]; then
      FILES_OBJC+="${f}"$'\n'
      ((++NUM_OBJC))
    elif [[ "${f}" == *.sh || "${f}" == *.bash ]]; then
      FILES_SH+="${f}"$'\n'
      ((++NUM_SH))
    elif [[ "${f}" == *.md ]]; then
      FILES_MD+="${f}"$'\n'
      ((++NUM_MD))
    else
      FILES_CPP+="${f}"$'\n'
      ((++NUM_CPP))
    fi
  fi
done

IFS=${ORG_IFS}

# Function that processes files.
# Arguments:
# - description of the type of processing (for logging) (e.g. "format")
# - number of files (nothing is done if it's less than 1)
# - language description (for logging)
# - command to run
# - common command arguments
# - the list of files to format (one per line)
function process_files() {
  if [[ ${2} -gt 0 ]]; then
    echo "Found ${2} ${3} file(s) to ${1}"

    # We don't use whitespaces as separators since there could be files
    # that use them. So we use \n as a separator.
    # Normally we would run xargs -d '\n', but it's not supported on Apple.
    # However, -0 (NULL separator) is.
    # But we can't store \0 inside bash strings, so we build a list of files
    # using \n separators, and replace them with \0 here (with tr command).

    # This removes the last '\n' from the list of files.
    # Otherwise it would be possible for xargs to run the command
    # with a single, empty argument (this happens when
    # the number of files modulo AUTOFORMAT_MAX_TASKS is 0).
    local XARG_FILES=${6%$'\n'}

    echo "${XARG_FILES}" | tr '\n' '\0' | xargs -0 ${XARG_OPTS} ${4} ${5}
  fi
}

# Formats files to use Unix line endings.
# Arguments:
# - number of files (nothing is done if it's less than 1)
# - the list of files to format (one per line)
function format_eol() {
  # Only change line endings on Windows.
  # Other platforms default to LF.
  if [[ "$(uname -s)" =~ "MINGW" ]]; then
    process_files "fix line endings" ${1} "" "sed" "-i s/\r$//" "${2}"
  fi
}

# Formats files removing trailing whitespace
# Arguments:
# - number of files (nothing is done if it's less than 1)
# - the list of files to format (one per line)
function format_tws() {
  process_files "remove trailing whitespace" ${1} "" "${MY_DIR}/sed-i.bash" "s/[[:space:]]*$//" "${2}"
}

# Formats markdown files removing trailing whitespace
# In Markdown, two or more spaces at the end of a line means newline,
# so we must preserve those spaces. All other trailing whitespace can
# be stripped
# Arguments:
# - number of files (nothing is done if it's less than 1)
# - the list of files to format (one per line)
function format_md_tws() {
  process_files "remove trailing whitespace" ${1} "markdown" "${MY_DIR}/sed-i.bash" \
    "s/\([^[:space:]]\)\([[:space:]]\)$/\1/" "${2}"
  process_files "remove whitespace from blank lines" ${1} "markdown" "${MY_DIR}/sed-i.bash" "s/^[[:space:]]*$//" "${2}"
}

# Formats files using uncrustify.
# Arguments:
# - number of files (nothing is done if it's less than 1)
# - language description (for logging)
# - config file for uncrustify
# - the list of files to format (one per line)
function format_unc() {
  # --no-backup makes it modify the originals, instead of creating
  #             files with '.uncrustify' suffix - this should be run
  #             inside a git repository, so it's not dangerous.
  # -L 1-2 disables most logging except for errors
  #        (otherwise we would get 'Parsing xyz.cpp' line for each file).
  process_files "format" ${1} "${2}" \
    "uncrustify" "-c ${MY_DIR}/uncrustify/${3} --no-backup -L 1-2" "${4}"
}

# Let's format each group of files.
# Doesn't matter if there are any, process_files() checks if there is anything to process.

# shfmt doesn't handle CRLF properly. All shell files have to use unix line endings.
format_eol ${NUM_SH} "${FILES_SH}"

format_tws ${NUM_PLAIN} "${FILES_PLAIN}"
format_md_tws ${NUM_MD} "${FILES_MD}"

format_unc ${NUM_OBJC} "ObjC" "objc.cfg" "${FILES_OBJC}"
format_unc ${NUM_CPP} "C/C++" "cpp.cfg" "${FILES_CPP}"
format_unc ${NUM_JAVA} "Java" "java.cfg" "${FILES_JAVA}"
format_unc ${NUM_CS} "C#" "cs.cfg" "${FILES_CS}"
