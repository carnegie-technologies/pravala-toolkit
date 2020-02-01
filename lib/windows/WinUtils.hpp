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

#include <guiddef.h>

namespace Pravala
{
class WString;

/// @brief Maps Windows error code to ERRCODE
/// @param [in] err Windows error code
/// @return Standard error code
ERRCODE mapWindowsErrorToERRCODE ( long err );

/// @brief Returns a description of a Windows error code
/// @param [in] err Windows error code
/// @return Human readable description
String getWindowsErrorDesc ( long err );

/// @brief Returns a description of a last Windows error code
/// @return Human readable description
String getLastWindowsErrorDesc();

/// @brief Maps Windows COM return code to ERRCODE
/// @param [in] hresult Windows COM return code
/// @return Standard error code
ERRCODE mapWindowsComErrorToERRCODE ( unsigned long hresult );

/// @brief Returns a description of a Windows COM return code
/// @param [in] hresult Windows COM return code
/// @return Human readable description
String getWindowsComErrorDesc ( unsigned long hresult );

/// @brief Returns a description of a Windows RPC status code
/// @param [in] status Windows RPC status code
/// @return Human readable description
String getWindowsRpcStatusDesc ( unsigned long status );

/// @brief Convert a GUID from binary format to MS string format in lower case:
/// http://msdn.microsoft.com/en-us/library/windows/desktop/aa373931%28v=vs.85%29.aspx
/// @param [in] guid The GUID in binary format
/// @return The GUID as a lower case string
String getWindowsGuidString ( const GUID & guid );

/// @brief Convert a standard GUID string to a GUID struct
/// @param [in] str The GUID as a string
/// @param [out] guid The GUID in binary format
/// @return Standard error code
ERRCODE getWindowsGuidFromString ( const String & str, GUID & guid );

/// @brief Convert a standard GUID wide string to a GUID struct
/// @param [in] wstr The GUID as a wide string
/// @param [out] guid The GUID in binary format
/// @return Standard error code
ERRCODE getWindowsGuidFromString ( const WString & wstr, GUID & guid );

/// @brief Generates a new Windows GUID.
/// This method uses UuidCreateSequential method and may generate UUIDs unique to this computer only.
/// @param [out] guid Generated GUID
/// @return Standard error code
ERRCODE generateGuid ( GUID & guid );

/// @brief Gets the string representation of a Windows interface identifier
/// @param [in] iid The Windows interface identifier
/// @return The IID as a human readable string
String getWindowsIidString ( REFIID iid );

/// @brief Returns true if the OS is Windows 8 or newer
/// @return True if the OS is Windows 8 or newer, false otherwise
bool isWindows8OrNewer();
}
