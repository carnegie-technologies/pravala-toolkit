#!/bin/bash

# Run "sed -i" passing the appropriate parameter to skip creating a backup file.

# NOTE: Darwin sed will also add a newline to the last line of the file
# if the last line did not end with a newline.

if [[ "$(sed --version 2>/dev/null)" =~ "GNU sed" ]]; then
  exec sed -i "$@"
else
  # Pass options assuming it's BSD sed
  exec sed -i '' "$@"
fi
