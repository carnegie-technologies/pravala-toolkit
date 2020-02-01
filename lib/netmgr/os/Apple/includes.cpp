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

#ifdef PLATFORM_IOS
// We don't support IOS, but we want to be able to create a dummy network manager (for now).
#include "../shared/dummy/NetManagerImpl.cpp"
#else
#include "../shared/afroute/AfRouteControl.cpp"
#include "../shared/afroute/AfRouteMonitor.cpp"
#include "../shared/afroute/RouteParser.cpp"
#include "../shared/afroute/RoutePayload.cpp"
#include "../shared/afroute/NetManagerImpl.cpp"
#endif
