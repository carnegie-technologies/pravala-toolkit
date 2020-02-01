# Code Styling

This directory contains scripts and settings for various code styling tasks,
like formatting rules and source file checking.

Tools in this directory:

## format.sh

This script finds and reformats all known source files that are part
of the GIT repository. It can be run from any directory (as long as it
is a part of a git repository), but will only find files that are
in that particular directory and its subdirectories.
When run from the top directory of a GIT repository it will find
all files that are part of it.

It is possible to ignore some files using '.styleignore' file (see below).

This script does not use any options.

## check_lines.sh

This script inspects all known source files that are part of the GIT
repository, to make sure that lines are not too long.
Just like format.sh it can be run from any directory
and will check all files in that directory and in its subdirectories.

It will print offending lines, and return error if there is at least one
line that is too long.

It is possible to ignore some files using '.styleignore' file (see below).

It supports following options:
* '-e' - When this option is provided, the script will exit with an error code
after the first file with too long lines. If it is not used, it will print
all problems.

* 'number' - This can be passed to modify the maximum allowed line length.
Otherwise the default value (120) is used.

Both options are optional, but need to be in that order
(-e first, followed by the number)

## .styleignore

Both format.sh and check_lines.sh use the same file for ignoring specific
source files/directories. This file will be looked for in the directory
from which the script was called. So if the script is called from
a subdirectory of a repository, the top-level ignore file will
not be used.

This file contains patters to ignore. They are checked against
the paths to the files (including directories), starting at the directory
from which the sacript was called.

For example, to ignore any source files inside '3rdparty' subdirectory
(because in general we don't want to format or check 3rd-party code),
this entry should be added to .styleignore: '^3rdparty/'
