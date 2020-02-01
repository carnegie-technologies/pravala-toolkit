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

#include "error/Error.hpp"
#include "basic/Buffer.hpp"
#include "object/PooledOwnedObject.hpp"
#include "event/EventManager.hpp"
#include "log/TextLog.hpp"

namespace Pravala
{
class String;
class SerialPort;

class SerialPortOwner
{
    protected:
        /// @brief Called when a line of data is received from the serial port
        /// A line is terminated by a '\r' or '\n'
        /// @param [in] port The SerialPort object this was received from.
        /// @param [in] line A line of text received excluding end of line character.
        virtual void receiveLine ( SerialPort * port, const String & line );

        /// @brief Called when a byte of data is received from the serial port
        /// @param [in] port The SerialPort object this was received from.
        /// @param [in] byte A byte of data received.
        virtual void receiveByte ( SerialPort * port, uint8_t byte );

        /// @brief Called when a serial port has been closed
        /// @note This is not called when a port is closed during unref
        /// @param [in] port The SerialPort object who's port has just been closed.
        virtual void portClosed ( SerialPort * port ) = 0;

        /// @brief Destructor.
        virtual ~SerialPortOwner()
        {
        }

        friend class SerialPort;
};

class SerialPort:
    public EventManager::FdEventHandler,
    public PooledOwnedObject<SerialPort, SerialPortOwner>
{
    public:
        /// @brief Port read mode -- how data received callbacks will be called
        enum PortReadMode
        {
            Discard = 0,    ///< Discard any received data and do not call callbacks
            LineMode,       ///< Line mode (call callbacks each time a line is in the buffer)
            ByteMode,       ///< Byte mode (call callbacks for each byte received)
            Invalid         ///< Invalid mode. Must be last entry in this enum!
        };

        /// @brief Generates a SerialPort object
        /// @param [in] owner The owner of this SerialPort object
        /// @return A SerialPort object
        static SerialPort * generate ( SerialPortOwner * owner, PortReadMode mode );

        /// @brief Open a serial port
        /// @note Unref this object to close the port. It makes no sense to keep buffers
        /// of the previous port around.
        /// @param [in] port Path of port to open (e.g. "/dev/ttyS0" or "COM1:")
        /// @return Standard error code
        ERRCODE openPort ( const String & port );

        /// @brief Close the port/fd
        /// @note This will call the portClosed callback only if the port was actually closed.
        /// It will not be called if the port was already closed.
        /// @param [in] notifyOwner True to notify our owner, false not to.
        bool closePort ( bool notifyOwner = true );

        /// @brief Write some data out the serial port
        /// @param [in] data Data to write out
        /// @param [in] appendReturn If this is true, then this function will append '\r' before sending
        /// @return Standard error code
        ERRCODE send ( const String & data, bool appendReturn = true );

        /// @brief Write some data out the serial port
        /// @param [in] data Data to write out
        /// @return Standard error code
        ERRCODE send ( const MemHandle & data );

        /// @brief Return the serial port device this object is using
        /// @return The serial port device this Object is using
        inline const String & getPortName() const
        {
            return _portName;
        }

        /// @brief Returns if this port is active (open) or not
        /// @return True if this port is active (open)
        inline bool isActive() const
        {
            return ( _fd >= 0 );
        }

    protected:
        /// @brief Receive an FdEvent from EventManager
        /// @param [in] fd The fd the event happened on.
        /// @param [in] events Bitmask of the events that occurred.
        void receiveFdEvent ( int fd, short events );

        /// @brief Called when the object is about to return to the object pool.
        void returnsToPool();

        /// @brief Creates a new SerialPort object
        /// @return A new SerialPort object, 0 if new failed
        inline static SerialPort * generateNew()
        {
            return new SerialPort();
        }

    private:
        static TextLog _log; ///< Log stream

        /// @brief Serial port device this Object is using
        String _portName;

        /// @brief Port reading mode -- action to take on read events with data
        PortReadMode _mode;

        /// @brief File descriptor open used for serial communications
        int _fd;

        /// @brief True if write events are disabled (i.e. have nothing to write)
        bool _evWriteDisabled;

        /// @brief Read buffer
        RwBuffer _readBuf;

        /// @brief Write buffer
        RwBuffer _writeBuf;

        /// @brief Constructor
        SerialPort();

        /// @brief Destructor
        ~SerialPort();

        /// @brief Consume data in _readBuf by the byte
        /// @note Unref may be called by the callbacks in this function!
        /// @note This function must only be called when we already have an owner
        void consumeReceivedDataByByte();

        /// @brief Consume data in _readBuf by line
        /// @note unref may be called by the callbacks in this function!
        /// Calls back our owner with lines of data split by '\r' or '\n' (with the '\r' or '\n' stripped)
        /// @note This function must only be called when we already have an owner
        void consumeReceivedDataByLine();

        /// @brief Platform specific function to open and initialize the serial port
        ERRCODE openPortPriv ( const String & port );

        POOLED_OWNED_OBJECT_HDR ( SerialPort, SerialPortOwner );
};
}
