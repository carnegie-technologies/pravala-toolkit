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
#include <ctime>
#include <cerrno>
#include <unistd.h>

#include "basic/Buffer.hpp"
#include "basic/List.hpp"
#include "config/ConfigStringMap.hpp"
#include "config/ConfigCore.hpp"
#include "sys/Time.hpp"
#include "app/StdApp.hpp"
#include "event/EventManager.hpp"
#include "event/Timer.hpp"

using namespace Pravala;

struct TimeDesc
{
    long unsigned sTime;
    long unsigned usTime;
    long unsigned usDiff;

    TimeDesc()
    {
        _currentTime.readTime ( _lastTime );
    }

    void update()
    {
        struct timespec cTime;

        _currentTime.readTime ( cTime );

        sTime = cTime.tv_sec;
        usTime = cTime.tv_nsec / 1000;
        usDiff = ( cTime.tv_sec - _lastTime.tv_sec ) * 1000 * 1000
                 + cTime.tv_nsec / 1000
                 - ( _lastTime.tv_nsec / 1000 );

        _lastTime = cTime;
    }

    private:
        static CurrentTime _currentTime;
        struct timespec _lastTime;
};

CurrentTime TimeDesc::_currentTime;

class EolTester: public EventManager::LoopEndEventHandler
{
    public:
        EolTester ( size_t repeats ): _repeats ( repeats )
        {
            EventManager::loopEndSubscribe ( this );
        }

    protected:
        virtual void receiveLoopEndEvent()
        {
            if ( _repeats > 0 )
            {
                --_repeats;
                EventManager::loopEndSubscribe ( this );
            }

            _tDesc.update();

            printf ( "LOOP-END: %lu.%06lu (+%lu us)\n", _tDesc.sTime, _tDesc.usTime, _tDesc.usDiff );
            fflush ( stdout );
        }

    private:
        TimeDesc _tDesc;
        size_t _repeats;
};

class Tester: public Timer::Receiver
{
    private:
        String _id;
        FixedTimer _timer;
        TimeDesc _tDesc;

    protected:
        void timerExpired ( Timer * /* timer */ )
        {
            _tDesc.update();

            unsigned long diff = _tDesc.usDiff;
            const char * units = "us";

            if ( _timer.FixedTimeout >= 100 )
            {
                diff /= 1000;
                units = "ms";
            }

            printf ( "%s: %lu.%06lu (+%lu %s) [delay: %u; use t-time: %d]\n",
                     _id.c_str(),
                     _tDesc.sTime, _tDesc.usTime,
                     diff, units,
                     _timer.FixedTimeout, _timer.UseTimerTime );
            fflush ( stdout );

            // Start the timer again!
            _timer.start();
        }

    public:
        Tester ( String id, uint32_t delay, bool useTimerTime ):
            _id ( id ), _timer ( *this, delay, useTimerTime )
        {
            _timer.start();
        }

        virtual ~Tester()
        {
        }
};

static ConfigNumber<size_t> optEndOfLoopRepeats (
        ConfigOpt::FlagInitializeOnly,
        "test.eol.repeats",
        "The number of times to repeat end-of-loop tests",
        0
);

static ConfigStringMap optTimerDelays (
        ConfigOpt::FlagInitializeOnly,
        "test.timer.*.delay",
        "Creates a timer 'ID' with the given delay ( in ms ) "
);

static ConfigStringMap optTimerUseT (
        ConfigOpt::FlagInitializeOnly,
        "test.timer.*.use_t-time",
        "Whether timer 'ID' will use timer time or not ( default is no ) "
);

int main ( int argc, char * argv[] )
{
    ConfigCore::EcfgInit ecfgInit ( "config_file = ./test.conf\n" );

    StdApp app ( argc, argv );

    printf ( "Timer Tester\n" );
    printf ( "Uses config file: '%s'\n", ConfigCore::optConfigPath.c_str() );
    printf ( "Possible options:\n" );
    printf ( "os.timers.resolution - the resolution of timers (in ms) >0, <=1000\n" );
    printf ( "os.timers.read_ahead_slots - the number of slots in the future to inspect looking for timers\n" );
    printf ( "os.timers.base_level_bits - the number of bits at the base level of timer wheels (between 8 and 24)\n" );
    printf ( "test.timer.ID.delay - creates a timer 'ID' with the given delay (in ms)\n" );
    printf ( "test.timer.ID.use_t-time - whether timer 'ID' will use timer time or not (default is no)\n" );

    app.init();

    printf ( "Using timer resolution: %d, base_level_bits: %d and read-ahead slots: %d\n",
             TimerManager::optResolution.value(),
             TimerManager::optBaseLevelBits.value(),
             TimerManager::optReadAheadSlots.value() );

    const MemHandle stats ( "/proc/self/status" );
    StringList sList = stats.toStringList ( "\n" );

    for ( size_t i = 0; i < sList.size(); ++i )
    {
        String line = sList[ i ].simplified();

        if ( line.startsWith ( "VmData:" ) )
        {
            StringList args = line.split ( " " );

            if ( args.size() < 3 )
                break;

            int memSize = args[ 1 ].toInt32();

            if ( args[ 2 ].toLower().simplified() == "b" )
            {
                printf ( "VmData size used: %d B\n", memSize );
                break;
            }

            if ( memSize > 1024 )
            {
                double fMemSize = ( ( double ) memSize ) / 1024.0;

                if ( fMemSize > 1024.0 )
                {
                    printf ( "VmData size used: %.2f GB\n", fMemSize / 1024.0 );
                }
                else
                {
                    printf ( "VmData size used: %.2f MB\n", fMemSize );
                }
            }
            else
            {
                printf ( "VmData size used: %d KB\n", memSize );
            }

            break;
        }
    }

    List<Tester *> testers;

    for ( HashMap<String, String>::Iterator it ( optTimerDelays.value() ); it.isValid(); it.next() )
    {
        bool ok = false;
        const uint32_t delay = it.value().toUInt32 ( &ok );

        String id = it.key().substr ( strlen ( "test.timer." ) );
        id = id.substr ( 0, id.length() - strlen ( ".delay" ) );

        const String ttId = String ( "test.timer." ).append ( id ).append ( ".use_t-time" );

        bool useTimerTime = optTimerUseT.value().value ( ttId ).toBool();

        if ( !ok )
        {
            printf ( "Ignoring timer with ID '%s' - incorrect delay set\n", id.c_str() );
            continue;
        }

        printf ( "Creating timer '%s' with delay %u [ms] and using t-times: %d\n", id.c_str(), delay, useTimerTime );

        testers.append ( new Tester ( id, delay, useTimerTime ) );
    }

    if ( testers.size() < 1 )
    {
        printf ( "No timers set, adding a simple 1 second timer\n" );
        testers.append ( new Tester ( "example", 1000, false ) );
    }

    printf ( "Start: %u.%03u\n",
             EventManager::getCurrentTime().getSeconds(), EventManager::getCurrentTime().getMilliSeconds() );

    EolTester * eolTester = 0;

    if ( optEndOfLoopRepeats.value() > 0 )
    {
        eolTester = new EolTester ( optEndOfLoopRepeats.value() );
    }

    EventManager::run();

    for ( size_t i = 0; i < testers.size(); ++i )
    {
        delete testers[ i ];
        testers[ i ] = 0;
    }

    testers.clear();

    delete eolTester;
    eolTester = 0;

    return EXIT_SUCCESS;
}
