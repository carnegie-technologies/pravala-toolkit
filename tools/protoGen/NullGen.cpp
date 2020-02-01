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

#include "Error.hpp"
#include "NullGen.hpp"

using namespace Pravala;

NullGenerator::NullGenerator ( ProtocolSpec & proto ): LanguageGenerator ( proto )
{
}

void NullGenerator::procRegularSymbol ( Symbol * /* symbol */ )
{
}

void NullGenerator::run()
{
    generateFlagFiles();
}

LanguageGenerator::SetOptResult NullGenerator::setOption (
        char shortName, const String & longName, const String & value )
{
    return setBasicOption ( shortName, longName, value );
}

String NullGenerator::getHelpText()
{
    String text
        = "    A 'null' generator. It can be used to parse and check the correctness of protocol files,\n"
          "    without generating any output. It can, however, generate flag files to indicate\n"
          "    that parsing has been successful.\n"
          "    Options:\n";

    text.append ( getBasicHelpText() );

    return text;
}
