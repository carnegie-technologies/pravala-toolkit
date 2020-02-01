/*
 *  Copyright 2019 Carnegie Technologies
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <cstdio>

#include "GitInfo.hpp"
#include "sys/File.hpp"

#define GIT_COMMIT_ID_LEN    40

using namespace Pravala;

/// @brief Checks whether given string is a GIT ID
/// @param [in] val The value to check.
/// @return True if the value looks like a GIT ID, false otherwise
static bool isGitId ( const String & val )
{
    String tmp ( val.trimmed() );

    if ( tmp.length() != GIT_COMMIT_ID_LEN )
        return false;

    for ( int i = 0; i < tmp.length(); ++i )
    {
        const char c = tmp[ i ];

        if ( !( c >= '0' && c <= '9' ) && !( c >= 'a' && c <= 'f' ) && !( c >= 'A' && c <= 'F' ) )
        {
            return false;
        }
    }

    return true;
}

String GitInfo::getGitDirectory ( const String & startingDir )
{
    String curDir ( startingDir );

    // We start from 'startingDir' and try to find the directory with GIT files.
    // Since we maybe pointed to a sub directory inside git structure (instead of the top level git directory),
    // we want to try a few directories up.
    // We will try to go up (by adding '/../') 10 times and repeat the search every time.
    // Every time curDirectory is the directory we are checking for '.git' sub directory.

    for ( int i = 0; i < 10; ++i )
    {
        String str;
        String tryPath ( curDir );
        MemHandle buf;

        // tryPath is our current directory.
        // If it contains '.git', it may either be a directory, or a file, in case we are inside git's submodule.
        // We want to be able to handle both. If our startingDir was pointing at the submodule,
        // we want to return that submodule's git root, instead of the main repository's git root.
        // Let's try to treat it as a file first (as if it was a submodule).
        // If it fails, it means that either there is no '.git' inside current directory, or it's not a file.
        // Also, if it is a file, but doesn't start with 'gitdir: ' than we ignore it.

        if ( buf.readFile ( String ( tryPath ).append ( "/.git" ) )
             && ( str = buf.toString().trimmed() ).startsWith ( "gitdir: " ) )
        {
            // This is a file and looks like git submodule's root!
            // So let's try to go to inside that submodule's git root and look for GIT files there.
            // The path to the GIT root of this module is specified after 'gitdir: '.
            // The content of this file is, for example: "gitdir: ../.git/modules/submodule_name".
            tryPath.append ( "/" ).append ( str.substr ( strlen ( "gitdir: " ) ) );
        }
        else
        {
            // There is no '.git' file, or it has incorrect content.
            // Let's try to treat '.git' as a directory:
            tryPath.append ( "/.git" );
        }

        // Here tryPath points either to '.git' subdirectory of the curDir, or the submodule's root.
        // Regardless, we want to check if there are specific files inside:

        if ( File::exists ( String ( tryPath ).append ( "/index" ) )
             && File::exists ( String ( tryPath ).append ( "/config" ) )
             && buf.readFile ( String ( tryPath ).append ( "/HEAD" ) ) )
        {
            // We had all the files we were looking for, and reading from "HEAD" file succeeded.
            // HEAD contains either the name of the branch the repository is using right now,
            // or the GIT ID of the change if it is in detached state.
            // In the first case it contains something like: "ref: refs/heads/master".
            // In the second case it contains 40 hex characters with the GIT ID.
            // Either of them is fine. If it doesn't start with "ref: refs/heads/" and it doesn't look like
            // GIT ID, we ignore the current directory and we keep looking.

            str = buf.toString().trimmed();

            if ( str.startsWith ( "ref: refs/heads/" ) || isGitId ( str ) )
            {
                return tryPath.append ( "/" );
            }
        }

        // Let's try the next directory up.

        curDir.append ( "/.." );
    }

    // We couldn't find anything...
    return String::EmptyString;
}

ERRCODE GitInfo::readGitBranch ( const String & projectRoot, String & output )
{
    output.clear();

    const String gitRoot ( getGitDirectory ( projectRoot ) );

    if ( gitRoot.isEmpty() )
        return Error::NotFound;

    MemHandle buf;

    if ( !buf.readFile ( String ( gitRoot ).append ( "HEAD" ) ) )
        return Error::NotFound;

    const String str ( buf.toString().trimmed() );

    // If the content of HEAD file starts with 'ref: refs/heads/' then the name of the branch follows that string.
    // If it looks like GIT ID, then we are in detached state and there is no branch.
    // If it is neither then something is wrong...

    if ( str.startsWith ( "ref: refs/heads/" ) )
    {
        output = str.substr ( strlen ( "ref: refs/heads/" ) );
    }
    else if ( isGitId ( str ) )
    {
        output = "<unknown>";
    }
    else
    {
        return Error::InvalidData;
    }

    return Error::Success;
}

ERRCODE GitInfo::readGitRev ( const String & projectRoot, bool shortRev, String & output )
{
    output.clear();

    const String gitRoot ( getGitDirectory ( projectRoot ) );

    if ( gitRoot.isEmpty() )
        return Error::NotFound;

    MemHandle buf;

    if ( !buf.readFile ( String ( gitRoot ).append ( "HEAD" ) ) )
        return Error::NotFound;

    String str ( buf.toString().trimmed() );

    // If the content of HEAD file starts with 'ref: refs/heads/' then the name of the branch follows that string.
    // To get the GIT ID of the latest change, we have to check that branch.
    // To do that, we read the file that is specified after 'ref: ' prefix.
    // For example, we may read 'refs/heads/master' file inside this directory.
    // That file should contain 40 hex GIT ID.
    // If the HEAD file doesn't start with 'ref: ' than it could mean that we are in detached state.
    // In that case HEAD itself should contain the GIT ID.
    // Whether we are reading ID from HEAD, or from the branch file, we should get the correct GIT ID.
    // If we don't then there is some error...

    if ( str.startsWith ( "ref: " ) )
    {
        // This looks like a branch file, let's read it instead of HEAD.

        String gitId;
        const String refName ( str.substr ( strlen ( "ref: " ) ) );

        if ( buf.readFile ( String ( gitRoot ).append ( refName ) ) )
        {
            gitId = buf.toString().trimmed();
        }

        if ( gitId.isEmpty() || !isGitId ( gitId ) )
        {
            StringList allLines;
            StringList files;

            // If the refs are packed and not under refs/...
            files.append ( "packed-refs" );

            // Just in case...
            files.append ( "info/refs" );

            for ( size_t f = 0; f < files.size(); ++f )
            {
                if ( buf.readFile ( String ( gitRoot ).append ( files.at ( f ) ) ) )
                {
                    const StringList tmp = buf.toStringList ( "\r\n" );

                    for ( size_t i = 0; i < tmp.size(); ++i )
                    {
                        allLines.append ( tmp.at ( i ) );
                    }
                }
            }

            for ( size_t i = 0; i < allLines.size(); ++i )
            {
                const StringList cols = allLines.at ( i ).split ( " \t" );

                if ( cols.size() == 2 && cols.at ( 1 ) == refName && isGitId ( cols.at ( 0 ) ) )
                {
                    gitId = cols.at ( 0 );
                    break;
                }
            }
        }

        str = gitId;
    }

    // Now 'str' should contain valid GIT ID, read either from HEAD file or from the branch file.

    if ( !isGitId ( str ) )
        return Error::InvalidData;

    // We have a correct ID, which is our revision info.
    // If 'shortRev' is requested, we keep only 7 characters of it. Otherwise we return the whole thing.

    if ( shortRev )
    {
        output = str.substr ( 0, 7 );
    }
    else
    {
        output = str;
    }

    return Error::Success;
}
