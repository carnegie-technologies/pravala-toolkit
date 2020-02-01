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

#pragma once

#include "log/TextLog.hpp"

struct sqlite3;
struct sqlite3_stmt;

#ifdef NO_LOGGING

#define SLOG_DB_ERR( stream, logLevel, error_code, db, x )    do { ( void ) error_code; ( void ) db; } while ( false )

#else

/// @brief A helper macro for logging database-related errors.
/// It uses SLOG_ERR, but it also takes the database object.
/// If the error code is Error::DatabaseError, then the result of calling getLastResultCodeStr() on the database object
/// will be appended to the log message. Otherwise it won't be included.
#define SLOG_DB_ERR( stream, logLevel, error_code, db, x ) \
    do { \
        if ( error_code == Error::DatabaseError ) { \
            SLOG_ERR ( stream, logLevel, error_code, x << ": " << db.getLastResultCodeStr() ); \
        } \
        else { \
            SLOG_ERR ( stream, logLevel, error_code, x ); \
        } \
    } while ( false )
#endif

/// @brief A helper macro for logging database-related errors.
/// Just like SLOG_DB_ERR, but it uses '_log' stream.
#define LOG_DB_ERR( logLevel, error_code, db, x )    SLOG_DB_ERR ( _log, logLevel, error_code, db, x )

namespace Pravala
{
/// @brief SQLite database wrapper
///
/// To use it, you should:
/// 1. open()
/// 2. create/update database if necessary
/// 3. prepare() a statement
/// 4. bind() if necessary
/// 5. step() to execute
/// 6. if step() hasData(), then call getColumn() several times with each column index to get the response
/// 7. repeat 5 and 6 until step() no longer hasData()
/// 8. restart() if you want to do the same query again
/// 9. finalize()
/// 10. close() (or let this object be destroyed)
///
///
/// You can also use the Database::Finalizer RAII class to avoid having to _db.finalize() everywhere.
///
/// @note All functions in this class are blocking!
class Database
{
    public:
        /// @brief Convenience class that will call _db.finalize() on destruction
        class Finalizer
        {
            public:
                /// @brief Constructor
                /// @param [in] db Database object to finalize when this object is destroyed
                Finalizer ( Database & db );

                /// @brief Destructor
                ~Finalizer();

            private:
                Database & _db; ///< Database object this object will finalize
        };

        /// @brief Constructor
        Database();

        /// @brief Destructor
        virtual ~Database();

        /// @brief Returns true if the database has been opened
        /// @return true if the database has been opened
        inline bool isOpen() const
        {
            return _db != 0;
        }

        /// @brief Returns the path of the open database file, empty if the database is not open.
        /// @return The path of the open database file.
        inline const String & getPath() const
        {
            return _path;
        }

        /// @brief Returns the last internal result code returned by the database API.
        /// @return The last internal result code returned by the database API.
        inline int getLastResultCode() const
        {
            return _lastResultCode;
        }

        /// @brief Returns description of the last internal result code returned by the database API.
        /// @param [in] includeCode If true (default), the message will include the numeric code value.
        /// @return Description of the last internal result code returned by the database API.
        inline String getLastResultCodeStr ( bool includeCode = true ) const
        {
            return resultCodeStr ( _lastResultCode, includeCode );
        }

        /// @brief Returns description of the given result code returned by the database API.
        /// @param [in] resCode The code to return the description of.
        /// @param [in] includeCode If true (default), the message will include the numeric code value.
        /// @return Description of the provided result code.
        static String resultCodeStr ( int resCode, bool includeCode = true );

        /// @brief Open this database
        /// @param [in] path Path to the database file to open
        /// @param [in] readonly True to open the database in read-only mode, this will result in open
        ///                      failing if the database doesn't already exist.
        /// @return Standard error code.
        ERRCODE open ( const String & path, const bool readonly = false );

        /// @brief Open this database
        /// @param [out] dbVersion If the open succeeded, then the database version will be written here, 0 if version
        /// is unknown or missing
        /// @param [in] path Path to the database file to open
        /// @param [in] readonly True to open the database in read-only mode, this will result in open
        ///                      failing if the database doesn't already exist.
        /// @return Standard error code.
        ERRCODE open ( int & dbVersion, const String & path, const bool readonly = false );

        /// @brief A convenience function for opening and updating the database.
        /// @param [in] path Path to the database file to open.
        /// @param [in] reqVersion Requested version of the database.
        /// @param [in] updateData Update data - see updateDatabase().
        /// @return True if the database has been opened (and updated) successfully; False otherwise.
        bool open ( const String & path, int reqVersion, const char ** updateData[] );

        /// @brief Update the database to the specific version.
        ///
        /// It takes 'updateData' which should be an array of pointers to arrays of strings that contain update steps.
        /// At the first level of updateData there are pointers to versions, one entry per database version.
        /// At the second level there are entries to run to update the database to that specific version.
        /// The last entry of each of the tables (both at the first and second level) should be 0.
        /// If, at the first level, a 0 value before reaching the reqVersion is considered an error and interrupts
        /// the update. Example:
        ///
        /// static const char * VER_1[] = { "CREATE TABLE people ( name TEXT )", 0 };
        /// static const char * VER_2[] = {
        ///   "ALTER TABLE people ADD COLUMN age INTEGER",
        ///   "ALTER TABLE people ADD COLUMN salary INTEGER",
        ///   0 };
        ///
        /// static const char ** VERSIONS[] = { VER_1, VER_2, 0 };
        ///
        /// update ( 0, 2, VERSIONS );
        ///
        /// @note Since value '0' in updateData means 'end of array', to have an empty update step
        /// (just to bump the version?) something like this should be used:
        ///
        /// static const char * VER_3[] = { 0 };
        /// static const char ** VERSIONS[] = { VER_1, VER_2, VER_3, 0 };
        ///
        /// @param [in] currentVersion What the current version of the database is.
        /// @param [in] reqVersion The requested version.
        /// @param [in] updateData Update data.
        /// @return True if the update was successful; False otherwise.
        bool update ( int currentVersion, int reqVersion, const char ** updateData[] );

        /// @brief Explicitly close this database
        /// @note A database is implicitly closed when this object is destroyed
        /// @param [in] removeFile If true, remove the database file after closing. Does nothing if already closed.
        void close ( bool removeFile = false );

        /// @brief Create or update the version table to this version
        /// @note This should be called within a transaction!
        /// @param [in] version Version to set
        /// @return Standard error code.
        ERRCODE setVersion ( int version );

        /// @brief Begin a transaction
        /// @return Standard error code.
        ERRCODE begin();

        /// @brief Commit a transaction
        /// @return Standard error code.
        ERRCODE commit();

        /// @brief Rollback a transaction
        /// @return Standard error code.
        ERRCODE rollback();

        /// @brief Truncate a table, this removes all data from the given table, but not the table itself
        /// @param [in] table The table name
        /// @return Standard error code.
        ERRCODE truncate ( const char * table );

        /// @brief Execute a statement
        /// @return Standard error code.
        ERRCODE exec ( const char * statement );

        /// @brief Prepare a statement
        /// If a statement has already been prepared, it will be finalized first
        /// @param [in] statement SQL statement to prepare
        /// @return Standard error code.
        ERRCODE prepare ( const String & statement );

        /// @brief Prepare a statement
        /// If a statement has already been prepared, it will be finalized first
        /// @param [in] statement SQL statement to prepare
        /// @param [in] len Length of statement (including the terminating null),
        ///                 -1 to auto-detect based on first null in statement.
        /// @return Standard error code.
        ERRCODE prepare ( const char * statement, int len = -1 );

        /// @brief Finalize the current statement
        /// @return Standard error code.
        ERRCODE finalize();

        /// @brief Restarts the current prepared statement
        /// @return Standard error code.
        ERRCODE restart();

        // Note the conspicuous lack of unsigned bind and get functions.
        // SQLite doesn't natively support them... so we don't provide functions for them
        // to avoid weird behaviour (e.g. using a 3 billion int which is interpreted as -1 billion,
        // then and not finding it doing where X > 2 billion)

        /// @brief Bind a bool to a prepared statement index
        /// @param [in] idx Index of '?' to bind to, the first index is 1
        /// @param [in] val Value to bind
        /// @return Standard error code.
        ERRCODE bind ( uint8_t idx, bool val );

        /// @brief Bind an int to a prepared statement index
        /// @param [in] idx Index of '?' to bind to, the first index is 1
        /// @param [in] val Value to bind
        /// @return Standard error code.
        ERRCODE bind ( uint8_t idx, int val );

        /// @brief Bind an int64 to a prepared statement index
        /// @param [in] idx Index of '?' to bind to, the first index is 1
        /// @param [in] val Value to bind
        /// @return Standard error code.
        ERRCODE bind ( uint8_t idx, int64_t val );

        /// @brief Bind a double to a prepared statement index
        /// @param [in] idx Index of '?' to bind to, the first index is 1
        /// @param [in] val Value to bind
        /// @return Standard error code.
        ERRCODE bind ( uint8_t idx, double val );

        /// @brief Bind a string to a prepared statement index
        /// @param [in] idx Index of '?' to bind to, the first index is 1
        /// @param [in] val Value to bind
        /// @param [in] copy True if SQLite should make a copy of this object.
        /// If this is false, then the caller must ensure that val is not changed or moved in memory
        /// until the query is complete. i.e. either close(), finalize() or bind ( idx, ... ) is called
        /// @return Standard error code.
        ERRCODE bind ( uint8_t idx, const String & val, bool copy = true );

        /// @brief Bind a string to a prepared statement index
        /// @param [in] idx Index of '?' to bind to, the first index is 1
        /// @param [in] val Value to bind
        /// @param [in] len Length of val, -1 to assume val is a null terminated string
        /// @param [in] copy True if SQLite should make a copy of this object.
        /// If this is false, then the caller must ensure that val is not changed or moved in memory
        /// until the query is complete. i.e. either close(), finalize() or bind ( idx, ... ) is called
        /// @return Standard error code.
        ERRCODE bind ( uint8_t idx, const char * val, int len = -1, bool copy = true );

        /// @brief Bind a binary blob to a prepared statement index (convenience helper)
        /// @param [in] idx Index of '?' to bind to, the first index is 1
        /// @param [in] val Value to bind
        /// @param [in] copy True if SQLite should make a copy of this object.
        /// If this is false, then the caller must ensure that val is not changed or moved in memory
        /// until the query is complete. i.e. either close(), finalize() or bind ( idx, ... ) is called
        /// @return Standard error code.
        inline ERRCODE bind ( uint8_t idx, const Buffer & val, bool copy = true )
        {
            return bindBlob ( idx, val.get(), val.size(), copy );
        }

        /// @brief Bind a binary blob to a prepared statement index
        /// @param [in] idx Index of '?' to bind to, the first index is 1
        /// @param [in] val Value to bind
        /// @param [in] len Length of val
        /// @param [in] copy True if SQLite should make a copy of this object.
        /// If this is false, then the caller must ensure that val is not changed or moved in memory
        /// until the query is complete. i.e. either close(), finalize() or bind ( idx, ... ) is called
        /// @return Standard error code.
        ERRCODE bindBlob ( uint8_t idx, const char * val, int len, bool copy = true );

        /// @brief Bind a null to a prepared statement index
        /// @param [in] idx Index of '?' to bind to, the first index is 1
        /// @return Standard error code.
        ERRCODE bindNull ( uint8_t idx );

        /// @brief Step through the result-set from the current prepared statement, executing the query if necessary.
        /// @return Standard error code. Possible codes:
        ///          - NotInitialized     - When the database is not initialized.
        ///          - DatabaseError      - When the database operation failed. The internal result code
        ///                                 can be obtained using getLastResultCode() method.
        ///          - DatabaseHasDataRow - This step() succeeded and there is a data row available that can
        ///                                 be read with the getColumn() method.
        ///          - Success            - There were no errors, but also this step() didn't fetch a new data row.
        ///                                 This could happen after reading all available data rows, but also after,
        ///                                 for example, performing an insert query. Note that 'Success' code means
        ///                                 that there is NO data row available. Either the previous call to the step()
        ///                                 method fetched the last row, or there were no data rows at all.
        ERRCODE step();

        /// @brief Gets a bool column from a result-set
        /// This should only be called
        /// @param [in] iCol Column number, the first index is 0
        /// @param [out] val Value from column in result-set
        /// @return Standard error code
        ERRCODE getColumn ( uint8_t iCol, bool & val );

        /// @brief Gets an int column from a result-set
        /// This should only be called
        /// @param [in] iCol Column number, the first index is 0
        /// @param [out] val Value from column in result-set
        /// @return Standard error code
        ERRCODE getColumn ( uint8_t iCol, int & val );

        /// @brief Gets an int64 column from a result-set
        /// @param [in] iCol Column number, the first index is 0
        /// @param [out] val Value from column in result-set
        /// @return Standard error code
        ERRCODE getColumn ( uint8_t iCol, int64_t & val );

        /// @brief Gets a string from a column from a result-set
        /// @param [in] iCol Column number, the first index is 0
        /// @param [out] val Value from column in result-set
        /// @return Standard error code
        ERRCODE getColumn ( uint8_t iCol, String & val );

        /// @brief Gets a blob from a column from a result-set
        /// @param [in] iCol Column number, the first index is 0
        /// @param [out] val Value from column in result-set
        /// @return Standard error code
        ERRCODE getColumn ( uint8_t iCol, MemHandle & val );

    private:
        static TextLog _log; ///< Logging stream

        String _path; ///< Path to this database

        sqlite3 * _db; ///< sqlite3 database
        sqlite3_stmt * _statement; ///< prepared statement

        int _lastResultCode; ///< The last result code returned by any of the DB operations (including step())

        /// @brief Returns the standard error code based on the current value of _lastResultCode.
        /// @return Success, DatabaseError, or DatabaseHasDataRow - depending on the value of _lastResultCode.
        ERRCODE mapLastResultCode() const;

        /// @brief Callback from SQLite when an error occurs
        /// @param [in] pArg pData value (not used)
        /// @param [in] iErrCode SQLite result code or extended result code
        /// @param [in] zMsg Log message
        static void callbackErrorLog ( void * pArg, int iErrCode, const char * zMsg );

        /// @brief Copy constructor - does not exist.
        Database ( const Database & other );

        /// @brief Assignment operator - does not exist.
        Database & operator= ( const Database & other );
};
}
