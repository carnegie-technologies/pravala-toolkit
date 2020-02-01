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

#include <cassert>
#include <cstring>

#include "basic/MsvcSupport.hpp"
#include "../../CurrentTime.hpp"

extern "C"
{
#include <winsock2.h>
#include <windows.h>
}

using namespace Pravala;

namespace Pravala
{
/// @brief Internal data used by CurrentTime.
class CurrentTimePriv
{
    public:
        /// @brief Default constructor.
        CurrentTimePriv(): _pcFreq ( getPerfFreq() )
        {
        }

        /// @brief Reads current time.
        /// @param [out] tspec The structure to store the time read.
        inline void readTime ( struct timespec & tspec ) const
        {
            // Time in nanoseconds
            const uint64_t nsTime = static_cast<uint64_t> ( static_cast<double> ( getPerfCounter() ) / _pcFreq );

            tspec.tv_sec = static_cast<uint32_t> ( nsTime / 1000000000 );
            tspec.tv_nsec = static_cast<uint32_t> ( nsTime % 1000000000 );
        }

    private:
        const double _pcFreq;        ///< The frequency of the performance counter (in nanoseconds).

        /// @brief Returns the frequency of the performance counter.
        /// @return The frequency of the performance counter (in nanoseconds).
        static double getPerfFreq()
        {
            LARGE_INTEGER li;
            const BOOL cRet = QueryPerformanceFrequency ( &li );

            ( void ) cRet;
            assert ( cRet );

            // This gives us frequency in nanoseconds:
            return static_cast<double> ( li.QuadPart ) / 1000000000.0;
        }

        /// @brief Returns the value of the performance counter.
        /// @return The value of the performance counter.
        static __int64 getPerfCounter()
        {
            LARGE_INTEGER li;
            const BOOL cRet = QueryPerformanceCounter ( &li );

            ( void ) cRet;
            assert ( cRet );

            return li.QuadPart;
        }
};
}

CurrentTime::CurrentTime(): _priv ( new CurrentTimePriv() )
{
    update();
}

CurrentTime::~CurrentTime()
{
    delete _priv;
}

void CurrentTime::readTime ( struct timespec & tspec ) const
{
    assert ( _priv != 0 );

    _priv->readTime ( tspec );
}
