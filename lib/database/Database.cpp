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

extern "C"
{
#include "sqlite/sqlite3.h"
}

#include "event/EventManager.hpp"
#include "sys/File.hpp"
#include "Database.hpp"

// We use this mask to obtain the basic result code from extended result code.
#define SQLITE_BASIC_RESULT_CODE_MASK    0x000000FFU

using namespace Pravala;

TextLog Database::_log ( "db" );

static bool logCallbackInitialized ( false );

Database::Database():
    _db ( 0 ),
    _statement ( 0 ),
    _lastResultCode ( SQLITE_OK )
{
    /// @note NOTE: Checking if the EventManager is current "primary" manager may not be the best way to do things.
    ///             Logs are generally not thread-safe, so Database objects should only ever be used on the main thread.
    if ( !logCallbackInitialized && EventManager::isPrimaryManager() )
    {
        logCallbackInitialized = true;
        sqlite3_config ( SQLITE_CONFIG_LOG, callbackErrorLog, 0 );
    }
}

Database::~Database()
{
    close();
}

Database::Finalizer::Finalizer ( Database & db ): _db ( db )
{
}

Database::Finalizer::~Finalizer()
{
    // This is a harmless no-op if the object has already been finalized
    _db.finalize();
}

ERRCODE Database::mapLastResultCode() const
{
    /// @todo TODO We may want to map more SQLite error codes to standard codes.
    ///            If that happens, we may have to modify the SLOG_DB_ERR macro as well!

    switch ( _lastResultCode & SQLITE_BASIC_RESULT_CODE_MASK )
    {
        case SQLITE_OK:
        case SQLITE_DONE:
            return Error::Success;
            break;

        case SQLITE_ROW:
            return Error::DatabaseHasDataRow;
            break;

        default:
            return Error::DatabaseError;
            break;
    }
}

String Database::resultCodeStr ( int resCode, bool includeCode )
{
    if ( includeCode )
    {
        return String ( "%1 [%2]" ).arg ( ::sqlite3_errstr ( resCode ), String::number ( resCode ) );
    }

    return ::sqlite3_errstr ( resCode );
}

ERRCODE Database::open ( const String & path, const bool readonly )
{
    if ( _db != 0 )
    {
        LOG ( L_ERROR, "Could not open database at '" << path
              << "' since this database object is already initialized (using path '" << _path << "')" );

        return Error::AlreadyInitialized;
    }

    // Disable mutexes, since we only support one statement in progress per object anyways
    int flags = SQLITE_OPEN_NOMUTEX;

    if ( readonly )
    {
        flags |= SQLITE_OPEN_READONLY;
    }
    else
    {
        flags |= SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    }

    _path = path;

    _lastResultCode = ::sqlite3_open_v2 ( path.c_str(), &_db, flags, NULL );

    if ( !_db )
    {
        LOG ( L_ERROR, "Internal error. Could not open database at '" << path
              << "'; SQLite result code: " << _lastResultCode << " (" << ::sqlite3_errstr ( _lastResultCode ) << ")" );

        close();
        return Error::DatabaseError;
    }

    const ERRCODE eCode = mapLastResultCode();

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, "Could not open database at '" << path
                  << "'; SQLite result code: " << _lastResultCode << " (" << ::sqlite3_errstr ( _lastResultCode )
                  << "): " << ::sqlite3_errmsg ( _db ) );

        close();
    }
    else
    {
        // enable extended result codes
        ::sqlite3_extended_result_codes ( _db, 1 );
    }

    return eCode;
}

ERRCODE Database::open ( int & dbVersion, const String & path, const bool readonly )
{
    ERRCODE eCode = open ( path, readonly );

    if ( NOT_OK ( eCode ) )
    {
        return eCode;
    }

    if ( !readonly )
    {
        // To avoid 'no such table' error while checking the version number
        exec ( "CREATE TABLE IF NOT EXISTS db_version ( version INTEGER )" );
    }

    // try to query for version
    eCode = prepare ( "SELECT version from db_version LIMIT 1" );

    if ( NOT_OK ( eCode ) || step() != Error::DatabaseHasDataRow )
    {
        // no version
        dbVersion = 0;

        finalize();

        return Error::Success;
    }

    eCode = getColumn ( 0, dbVersion );

    finalize();

    if ( eCode == Error::EmptyRead )
    {
        // NULL version
        dbVersion = 0;

        return Error::Success;
    }
    else if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, _path << ": Error reading the database version" );
    }

    return Error::Success;
}

bool Database::open ( const String & path, int reqVersion, const char ** updateData[] )
{
    int currentVersion = 0;

    if ( NOT_OK ( open ( currentVersion, path ) ) )
    {
        return false;
    }

    if ( currentVersion != reqVersion && !update ( currentVersion, reqVersion, updateData ) )
    {
        LOG ( L_ERROR, _path << ": Could not update database from version "
              << currentVersion << " to version " << reqVersion );

        close();
        return false;
    }

    return true;
}

bool Database::update ( int currentVersion, int reqVersion, const char ** updateData[] )
{
    if ( !updateData )
    {
        LOG ( L_FATAL_ERROR, _path << ": Database cannot be updated using empty update data" );

        // Invalid parameter
        return false;
    }

    if ( currentVersion < 0 || currentVersion > reqVersion )
    {
        LOG ( L_ERROR, _path << ": Database is in invalid/future version " << currentVersion
              << "; Required version is " << reqVersion );

        // Invalid/future version
        return false;
    }

    if ( currentVersion == reqVersion )
    {
        LOG ( L_DEBUG, _path << ": Database is already in required version " << reqVersion
              << " - no need to update" );

        // Already up-to-date
        return true;
    }

    // Instead of starting from currentVersion, we always start from the beginning,
    // to make sure that we don't go beyond the updateData array (its last entry should be 0).

    int version = 0;

    while ( version < reqVersion )
    {
        const char ** ptr = updateData[ version ];

        if ( !ptr )
        {
            // updateData too small?
            // We saw 0 value (which means this is the end of the array) before reaching the requested version.
            return false;
        }

        ++version;

        // Now 'version' is set to the version that will be set if this update step succeeds.

        if ( version <= currentVersion )
        {
            // Database is already at, or past, this version - no need to run this step!
            continue;
        }

        ERRCODE eCode = begin();

        if ( NOT_OK ( eCode ) )
        {
            LOG_ERR ( L_DEBUG, eCode, _path << ": Could not start the transaction" );

            return false;
        }

        bool isOk = true;

        while ( isOk && *ptr != 0 )
        {
            eCode = exec ( *ptr );

            isOk = IS_OK ( eCode );

            if ( !isOk )
            {
                LOG_ERR ( L_ERROR, eCode,
                          _path << ": Error running database update step; SQLite result code: " << _lastResultCode
                          << " (" << ::sqlite3_errstr ( _lastResultCode ) << "): " << ::sqlite3_errmsg ( _db ) );
            }

            ++ptr;
        }

        if ( !isOk
             || NOT_OK ( setVersion ( version ) )
             || NOT_OK ( commit() ) )
        {
            rollback();
            return false;
        }
    }

    return true;
}

void Database::close ( bool removeFile )
{
    if ( !_db )
        return;

    // ignore the result code, since it indicates that the prepared statement failed,
    // which we don't care about anymore if we're closing it
    finalize();

    // ignore return code here as well
    ::sqlite3_close ( _db );

    if ( removeFile && !_path.isEmpty() && !File::remove ( _path ) )
    {
        LOG ( L_ERROR, "Failed to remove database file: '" << _path << "'" );
    }

    _path.clear();
    _db = 0;
}

ERRCODE Database::setVersion ( int version )
{
    ERRCODE eCode = exec ( "DELETE FROM db_version" );

    UNTIL_ERROR ( eCode, prepare ( "INSERT INTO db_version(version) VALUES(?)" ) );
    UNTIL_ERROR ( eCode, bind ( 1, version ) );
    UNTIL_ERROR ( eCode, step() );

    const int lastCode = _lastResultCode;

    finalize();

    // We want to report the result code before 'finalize':
    _lastResultCode = lastCode;

    return eCode;
}

ERRCODE Database::begin()
{
    return exec ( "BEGIN" );
}

ERRCODE Database::commit()
{
    return exec ( "COMMIT" );
}

ERRCODE Database::rollback()
{
    return exec ( "ROLLBACK" );
}

ERRCODE Database::truncate ( const char * table )
{
    // SQLite doesn't support TRUNCATE, instead use DELETE + VACUUM.

    ERRCODE eCode = exec ( String ( "DELETE FROM %1" ).arg ( table ).c_str() );

    UNTIL_ERROR ( eCode, exec ( "VACUUM" ) );

    return eCode;
}

ERRCODE Database::exec ( const char * statement )
{
    if ( !_db )
        return Error::NotInitialized;

    _lastResultCode = ::sqlite3_exec ( _db, statement, 0, 0, 0 );

    LOG ( L_DEBUG2, _path << ": Exec(" << statement << "): " << _lastResultCode );

    const ERRCODE eCode = mapLastResultCode();

    if ( NOT_OK ( eCode ) )
    {
        LOG_ERR ( L_ERROR, eCode, _path << ": Error running database query '" << statement
                  << "'; SQLite error code: " << _lastResultCode
                  << " (" << ::sqlite3_errstr ( _lastResultCode ) << "): " << ::sqlite3_errmsg ( _db ) );
    }

    return eCode;
}

ERRCODE Database::prepare ( const String & statement )
{
    if ( !_db )
        return Error::NotInitialized;

    // plus 1 to length to include the null character so that we get the performance gain
    return prepare ( statement.c_str(), statement.length() + 1 );
}

ERRCODE Database::prepare ( const char * statement, int len )
{
    if ( !_db )
        return Error::NotInitialized;

    _lastResultCode = ::sqlite3_prepare_v2 ( _db, statement, len, &_statement, 0 );

    LOG ( L_DEBUG2, _path << ": Prepare(" << statement << "): " << _lastResultCode
          << "; State: " << ( ( long unsigned ) _statement ) );

    return mapLastResultCode();
}

ERRCODE Database::finalize()
{
    if ( !_statement )
        return Error::NotInitialized;

    _lastResultCode = ::sqlite3_finalize ( _statement );

    LOG ( L_DEBUG2, _path << ": Finalize: " << _lastResultCode << "; State: " << ( ( long unsigned ) _statement ) );

    _statement = 0;

    return mapLastResultCode();
}

ERRCODE Database::restart()
{
    if ( !_statement )
        return Error::NotInitialized;

    assert ( _db != 0 );

    _lastResultCode = ::sqlite3_reset ( _statement );

    return mapLastResultCode();
}

ERRCODE Database::bind ( uint8_t idx, bool val )
{
    // SQLite's API doesn't support bool specifically, a bool is really just an int
    return bind ( idx, ( int ) ( val ? 1 : 0 ) );
}

ERRCODE Database::bind ( uint8_t idx, int val )
{
    assert ( idx > 0 );

    if ( idx < 1 )
        return Error::InvalidParameter;

    if ( !_statement )
        return Error::NotInitialized;

    assert ( _db != 0 );

    _lastResultCode = ::sqlite3_bind_int ( _statement, idx, val );

    return mapLastResultCode();
}

ERRCODE Database::bind ( uint8_t idx, double val )
{
    assert ( idx > 0 );

    if ( idx < 1 )
        return Error::InvalidParameter;

    if ( !_statement )
        return Error::NotInitialized;

    assert ( _db != 0 );

    _lastResultCode = ::sqlite3_bind_double ( _statement, idx, val );

    return mapLastResultCode();
}

ERRCODE Database::bind ( uint8_t idx, int64_t val )
{
    assert ( idx > 0 );

    if ( idx < 1 )
        return Error::InvalidParameter;

    if ( !_statement )
        return Error::NotInitialized;

    assert ( _db != 0 );

    _lastResultCode = ::sqlite3_bind_int64 ( _statement, idx, val );

    return mapLastResultCode();
}

ERRCODE Database::bind ( uint8_t idx, const String & val, bool copy )
{
    assert ( idx > 0 );

    return bind ( idx, val.c_str(), val.length(), copy );
}

ERRCODE Database::bind ( uint8_t idx, const char * val, int len, bool copy )
{
    assert ( idx > 0 );

    if ( idx < 1 )
        return Error::InvalidParameter;

    if ( !_statement )
        return Error::NotInitialized;

    assert ( _db != 0 );

    static const char * empty = "";

    if ( val != 0 && val[ 0 ] == '\0' && len < 1 )
    {
        // If an empty string is being passed in, then pass our local static empty string without copying.
        // Use the regular call if a null pointer is being passed in, as that binds a NULL instead of an empty string.
        // This is to workaround how SQLite appears to treat SQLITE_TRANSIENT + an empty string as a NULL.

        _lastResultCode = ::sqlite3_bind_text ( _statement, idx, empty, 0, SQLITE_STATIC );
    }
    else
    {
        // if copy is true, then set SQLITE_TRANSIENT so that SQLite will take a copy.
        // Otherwise set SQLITE_STATIC, so that it will be up to the caller to ensure it exists
        // and isn't destroyed/moved until after finalize() is called, or this index is rebound.
        //
        // SQLite also supports a mode where it'll call a callback when it's done, but it's rather hard
        // to develop with that.

        _lastResultCode = ::sqlite3_bind_text ( _statement, idx, val, len, copy ? SQLITE_TRANSIENT : SQLITE_STATIC );
    }

    return mapLastResultCode();
}

ERRCODE Database::bindBlob ( uint8_t idx, const char * val, int len, bool copy )
{
    assert ( idx > 0 );

    if ( idx < 1 )
        return Error::InvalidParameter;

    if ( !_statement )
        return Error::NotInitialized;

    assert ( _db != 0 );

    // if copy is true, then set SQLITE_TRANSIENT so that SQLite will take a copy.
    // Otherwise set SQLITE_STATIC, so that it will be up to the caller to ensure it exists
    // and isn't destroyed/moved until after finalize() is called, or this index is rebound.
    //
    // SQLite also supports a mode where it'll call a callback when it's done, but it's rather hard
    // to develop with that.
    _lastResultCode = ::sqlite3_bind_blob ( _statement, idx, val, len, copy ? SQLITE_TRANSIENT : SQLITE_STATIC );

    return mapLastResultCode();
}

ERRCODE Database::bindNull ( uint8_t idx )
{
    assert ( idx > 0 );

    if ( idx < 1 )
        return Error::InvalidParameter;

    if ( !_statement )
        return Error::NotInitialized;

    assert ( _db != 0 );

    _lastResultCode = ::sqlite3_bind_null ( _statement, idx );

    return mapLastResultCode();
}

ERRCODE Database::step()
{
    if ( !_statement )
        return Error::NotInitialized;

    _lastResultCode = ::sqlite3_step ( _statement );

    return mapLastResultCode();
}

ERRCODE Database::getColumn ( uint8_t iCol, bool & val )
{
    int tmp = 0;

    ERRCODE eCode = getColumn ( iCol, tmp );

    if ( IS_OK ( eCode ) )
    {
        val = ( tmp != 0 );
    }

    return eCode;
}

ERRCODE Database::getColumn ( uint8_t iCol, int & val )
{
    if ( ( _lastResultCode & SQLITE_BASIC_RESULT_CODE_MASK ) != SQLITE_ROW )
        return Error::WrongState;

    if ( iCol >= ::sqlite3_column_count ( _statement ) )
        return Error::InvalidIndex;

    const int type = ::sqlite3_column_type ( _statement, iCol );

    if ( type == SQLITE_NULL )
    {
        return Error::EmptyRead;
    }

    val = ::sqlite3_column_int ( _statement, iCol );

    return Error::Success;
}

ERRCODE Database::getColumn ( uint8_t iCol, int64_t & val )
{
    if ( ( _lastResultCode & SQLITE_BASIC_RESULT_CODE_MASK ) != SQLITE_ROW )
        return Error::WrongState;

    if ( iCol >= ::sqlite3_column_count ( _statement ) )
        return Error::InvalidIndex;

    const int type = ::sqlite3_column_type ( _statement, iCol );

    if ( type == SQLITE_NULL )
    {
        return Error::EmptyRead;
    }

    val = ::sqlite3_column_int64 ( _statement, iCol );

    return Error::Success;
}

ERRCODE Database::getColumn ( uint8_t iCol, String & val )
{
    if ( ( _lastResultCode & SQLITE_BASIC_RESULT_CODE_MASK ) != SQLITE_ROW )
        return Error::WrongState;

    if ( iCol >= ::sqlite3_column_count ( _statement ) )
        return Error::InvalidIndex;

    const int type = ::sqlite3_column_type ( _statement, iCol );

    if ( type == SQLITE_NULL )
    {
        return Error::EmptyRead;
    }

    // sqlite3_column_text wants to return a const unsigned char* probably for proper UTF-8 support.
    // we don't really care for it, it's an arbitrary block of data to us, so just cast it to const char*
    val = ( const char * )::sqlite3_column_text ( _statement, iCol );

    return Error::Success;
}

ERRCODE Database::getColumn ( uint8_t iCol, MemHandle & val )
{
    if ( ( _lastResultCode & SQLITE_BASIC_RESULT_CODE_MASK ) != SQLITE_ROW )
        return Error::WrongState;

    if ( iCol >= ::sqlite3_column_count ( _statement ) )
        return Error::InvalidIndex;

    val.clear();

    const int type = ::sqlite3_column_type ( _statement, iCol );

    const int len = ::sqlite3_column_bytes ( _statement, iCol );

    if ( type == SQLITE_NULL || len < 1 )
    {
        return Error::EmptyRead;
    }

    const void * result = ::sqlite3_column_blob ( _statement, iCol );

    assert ( result != 0 );

    val = MemHandle ( len );

    char * const buf = val.getWritable();

    if ( !buf || val.size() != ( size_t ) len )
    {
        val.clear();

        return Error::MemoryError;
    }

    assert ( buf != 0 );

    memcpy ( buf, result, len );

    return Error::Success;
}

void Database::callbackErrorLog ( void * /*pArg*/, int iErrCode, const char * zMsg )
{
    ( void ) iErrCode;
    ( void ) zMsg;

    /// @note NOTE: Checking if the EventManager is current "primary" manager may not be the best way to do things.
    ///             Logs are generally not thread-safe, so Database objects should only ever be used on the main thread.
    if ( EventManager::isPrimaryManager() )
    {
        LOG ( L_ERROR, "SQLite error; Code: " << iErrCode << "; " << zMsg );
    }
}
