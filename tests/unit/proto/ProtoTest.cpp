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

#include <cmath>

#include "basic/Random.hpp"
#include "basic/FloatingPointUtils.hpp"
#include "json/Json.hpp"
#include "proto/ProtocolCodec.hpp"

#include "UnitTest.hpp"

#include "auto/Test/General/TestCode.hpp"
#include "auto/Test/Ctrl/ClientHello.hpp"
#include "auto/Test/Ctrl/ClientConfig.hpp"
#include "auto/Test/Ctrl/ClientRejected.hpp"
#include "auto/Test/Ctrl/PubSubReqIfaceState.hpp"
#include "auto/Test/Ctrl/PubSubRespIfaceState.hpp"
#include "auto/Test/Container.hpp"
#include "auto/Test/ValueStore.hpp"
#include "auto/Test/ValueMessage.hpp"

// Uncomment the following line to see the content of several
// messages generated during the test:

// #define DUMP_DATA 1

// This file can be modified as the content of that file should change, to allow for different versions to run properly
// Commenting it out disables use of actual files on disk.
// #define DAT_FILE "/tmp/._protocol_test_.dat.0"

using namespace Pravala;
using namespace Pravala::Protocol;
using namespace Pravala::Protocol::Test;
using namespace Pravala::Protocol::Test::General;
using namespace Pravala::Protocol::Test::Ctrl;

using std::isnan;

class ProtoTest: public ::testing::Test
{
};

TEST_F ( ProtoTest, EnumTest )
{
    TestCode eCode ( TestCode::code_b );

    // This should work, but GTest doesn't like it, as it's not really compatible with operator<<:
    // ASSERT_EQ ( eCode, TestCode::code_b );
    ASSERT_EQ ( eCode.value(), TestCode::code_b );

    ASSERT_EQ ( TestCode::code_b, eCode.value() );

    eCode = TestCode::code_c;

    ASSERT_EQ ( eCode.value(), TestCode::code_c );
    ASSERT_TRUE ( eCode == TestCode::code_c );
    ASSERT_FALSE ( eCode != TestCode::code_c );

    // We can't write 'TestCode::code_c == eCode' since there is no cast
    // operator. We don't want enum objects to be automatically casted
    // as this would mean automatic casting to integers!

    ASSERT_EQ ( TestCode::code_c, eCode.value() );
    ASSERT_TRUE ( TestCode::code_c == eCode.value() );
    ASSERT_FALSE ( TestCode::code_c != eCode.value() );

    eCode = TestCode::code_a;

    if ( eCode == TestCode::code_a )
    {
        eCode = TestCode::code_b;
    }

    switch ( eCode.value() )
    {
        case TestCode::code_a:
            ASSERT_EQ ( true, false );
            break;

        case TestCode::code_b:
            ASSERT_TRUE ( true );
            break;

        default:
            ASSERT_EQ ( true, false );
            break;
    }
}

TEST_F ( ProtoTest, BasicTest )
{
#ifdef DUMP_DATA
    Buffer dumpBuf;
    String ind;
#endif

    Json json;

    ClientHello clientHello;
    ClientConfig clientConfig;
    ClientRejected clientRejected;
    PubSubReqIfaceState reqIfaceState;
    PubSubRespIfaceState respIfaceState;

    // They all inherit BaseMsg, which has a required field.
    // BEfore we call setupDefines, none of them will be valid!
    EXPECT_ERRCODE_EQ ( ProtoError::RequiredFieldNotSet, clientHello.validate() );
    EXPECT_ERRCODE_EQ ( ProtoError::RequiredFieldNotSet, clientConfig.validate() );
    EXPECT_ERRCODE_EQ ( ProtoError::RequiredFieldNotSet, clientRejected.validate() );
    EXPECT_ERRCODE_EQ ( ProtoError::RequiredFieldNotSet, reqIfaceState.validate() );
    EXPECT_ERRCODE_EQ ( ProtoError::RequiredFieldNotSet, respIfaceState.validate() );

    clientHello.setupDefines();
    clientConfig.setupDefines();
    clientRejected.setupDefines();
    reqIfaceState.setupDefines();
    respIfaceState.setupDefines();

    // Let's make sure that all 'defined' values are now set:
    EXPECT_TRUE ( clientHello.hasType() );
    EXPECT_TRUE ( clientConfig.hasType() );
    EXPECT_TRUE ( clientRejected.hasType() );

    // These messages should not, however, be configured as 'control' messages
    EXPECT_FALSE ( clientHello.getIsCtrl() );
    EXPECT_FALSE ( clientConfig.getIsCtrl() );
    EXPECT_FALSE ( clientRejected.getIsCtrl() );

    EXPECT_TRUE ( reqIfaceState.hasType() );
    EXPECT_TRUE ( reqIfaceState.hasIsCtrl() );
    EXPECT_TRUE ( reqIfaceState.hasIsPubSub() );
    EXPECT_TRUE ( reqIfaceState.getIsCtrl() );
    EXPECT_TRUE ( reqIfaceState.getIsPubSub() );
    EXPECT_FALSE ( reqIfaceState.getIsResponse() );

    EXPECT_TRUE ( respIfaceState.hasType() );
    EXPECT_TRUE ( respIfaceState.hasIsCtrl() );
    EXPECT_TRUE ( respIfaceState.hasIsPubSub() );
    EXPECT_TRUE ( respIfaceState.getIsCtrl() );
    EXPECT_TRUE ( respIfaceState.getIsPubSub() );
    EXPECT_TRUE ( respIfaceState.getIsResponse() );

    // ClientHello requires cert_id to be set:
    EXPECT_ERRCODE_EQ ( ProtoError::RequiredFieldNotSet, clientHello.validate() );

    clientHello.setCertId ( "a" );

    // Now the cert_id is set, but it's too short:
    EXPECT_ERRCODE_EQ ( ProtoError::StringLengthOutOfRange, clientHello.validate() );

    clientHello.setCertId ( "abcdefghijklmnopqrstuwxabcdefghijklmnopqrstuwxabcdefghijklmnopqrstuwxabcdefghijklmn" );

    // And now it's too long:
    EXPECT_ERRCODE_EQ ( ProtoError::StringLengthOutOfRange, clientHello.validate() );

    clientHello.setCertId ( "abcdefghij" );

    // Now it should be just fine!
    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientHello.validate() );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientHello.serialize ( json ) );
    EXPECT_STREQ ( String ( "{\"type\":%1,\"certId\":\"abcdefghij\"}" )
                   .arg ( clientHello.DEF_TYPE ).c_str(),
                   json.toString().c_str() );

    // ClientConfig is not valid, because it has addrToUse list, with 'min size' set to 1
    EXPECT_ERRCODE_EQ ( ProtoError::ListSizeOutOfRange, clientConfig.validate() );

    clientConfig.modAddrToUse().append ( IpAddress ( "127.0.0.1" ) );

    // Now it should be fine!
    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientConfig.validate() );

    // But we can still break it! :)

    // ClientConfig has also a dnsToUse list, which could be empty, but when it's not,
    // each entry should have at least 7 characters:
    clientConfig.modDnsToUse().append ( "8.8.8." );

    // Now this should fail:
    EXPECT_ERRCODE_EQ ( ProtoError::StringLengthOutOfRange, clientConfig.validate() );

    // This just add an entry (which is correct), but the original wrong entry is still there:
    clientConfig.modDnsToUse().append ( "8.8.8.8" );

    EXPECT_ERRCODE_EQ ( ProtoError::StringLengthOutOfRange, clientConfig.validate() );

    // Let's try again!
    clientConfig.modDnsToUse().clear();

    EXPECT_EQ ( 0U, clientConfig.modDnsToUse().size() );

    clientConfig.modDnsToUse().append ( "8.8.8.8" );

    // Now we have only one entry and it has correct length
    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientConfig.validate() );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientConfig.serialize ( json ) );
    EXPECT_STREQ ( String ( "{\"type\":%1,\"addrToUse\":[\"127.0.0.1\"],\"dnsToUse\":[\"8.8.8.8\"]}" )
                   .arg ( clientConfig.DEF_TYPE ).c_str(),
                   json.toString().c_str() );

    // ClientRejected should have its 'errCode' set:
    EXPECT_ERRCODE_EQ ( ProtoError::RequiredFieldNotSet, clientRejected.validate() );

    // This should work, but GTest doesn't like it, as it's not really compatible with operator<<:
    // EXPECT_NE ( clientRejected.getErrCode(), General::TestCode::code_c );
    EXPECT_NE ( clientRejected.getErrCode().value(), General::TestCode::code_c );

    clientRejected.setErrCode ( General::TestCode::code_c );

    // And same as above
    EXPECT_EQ ( clientRejected.getErrCode().value(), General::TestCode::code_c );

    // And now it should be fine!
    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientRejected.validate() );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientRejected.serialize ( json ) );
    EXPECT_STREQ ( String ( "{\"type\":%1,\"errCode\":\"code_c\"}" )
                   .arg ( clientRejected.DEF_TYPE ).c_str(),
                   json.toString().c_str() );

    // PubSubReqIfaceState needs 'ifaceId' field set:
    EXPECT_ERRCODE_EQ ( ProtoError::RequiredFieldNotSet, reqIfaceState.validate() );

    reqIfaceState.setIfaceId ( 0 );

    // We set it (by using mod_*()):
    EXPECT_TRUE ( reqIfaceState.hasIfaceId() );

    // But wait, it also needs 'subType' field! (which is a part of PubSubReq message):
    EXPECT_ERRCODE_EQ ( ProtoError::RequiredFieldNotSet, reqIfaceState.validate() );

    // So let's set it:
    reqIfaceState.setSubType ( 15 );

    // But now the value of ifaceId is incorrect (it should be at least 1):
    EXPECT_ERRCODE_EQ ( ProtoError::FieldValueOutOfRange, reqIfaceState.validate() );

    // Let's fix it:
    reqIfaceState.setIfaceId ( 5 );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, reqIfaceState.validate() );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, reqIfaceState.serialize ( json ) );

    // This includes a number of fields we did NOT set.
    // Those are aliases that share the same bit storage field (not included in JSON output)
    // that are all considered "set" when at least one of them was set (and it was).

    EXPECT_STREQ ( String ( "{\"type\":%1,\"isCtrl\":true,\"isRemote\":false,\"isPubSub\":true,"
                            "\"isResponse\":false,\"srcAddr\":0,\"dstAddr\":0,\"isUnreliable\":false,"
                            "\"subType\":15,\"ifaceId\":5}" )
                   .arg ( reqIfaceState.DEF_TYPE ).c_str(),
                   json.toString().c_str() );

    // Now this one is fun! The PubSubRespIfaceState message needs at least
    // one entry in its ifaceDesc list. We need to set it up, and add it!
    IfaceDesc ifDesc;

    // IfaceDesc has required fields of its own:
    EXPECT_ERRCODE_EQ ( ProtoError::RequiredFieldNotSet, ifDesc.validate() );

    // We could, however, still add it!
    respIfaceState.modIfaceDesc().append ( ifDesc );

    // This should cause the isValid to stop complaining about the list size,
    // but it's still invalid - IfaceDesc still doesn't have those required fields set.
    EXPECT_ERRCODE_EQ ( ProtoError::RequiredFieldNotSet, respIfaceState.validate() );

    // Let's try to fix that!

    // Just in case...
    ASSERT_EQ ( 1U, respIfaceState.getIfaceDesc().size() );

    // One if ifaceId
    respIfaceState.modIfaceDesc()[ 0 ].setIfaceId ( 5 );

    // The other is ifaceStatus - this is an enum!
    respIfaceState.modIfaceDesc()[ 0 ].setIfaceStatus ( IfaceDesc::IfaceStatus::IfaceUp );

    // But... still no luck. It turns out, the IfaceDesc also defines a required field in the message it inherits.
    EXPECT_ERRCODE_EQ ( ProtoError::RequiredFieldNotSet, respIfaceState.validate() );

    // We need to fix this...
    // We can't set it directly though! We need to use setupDefines(). We have already done it for this message,
    // but at that time the ifaceDesc list was empty, so setupDefines was not call on the IfaceDesc that
    // is there right now. We could have done it on the variable before appending it to the list,
    // or now.

    respIfaceState.setupDefines();

    // Finally, we have a valid message:
    EXPECT_ERRCODE_EQ ( ProtoError::Success, respIfaceState.validate() );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, respIfaceState.serialize ( json ) );

    // This includes a number of fields we did NOT set.
    // Those are aliases that share the same bit storage field (not included in JSON output)
    // that are all considered "set" when at least one of them was set (and it was).

    EXPECT_STREQ ( String ( "{\"type\":%1,\"isCtrl\":true,\"isRemote\":false,\"isPubSub\":true,"
                            "\"isResponse\":true,\"srcAddr\":0,\"dstAddr\":0,\"isUnreliable\":false,"
                            "\"ifaceDesc\":[{\"isIfaceIdMsg\":1,\"ifaceId\":5,\"ifaceStatus\":\"IfaceUp\"}]}" )
                   .arg ( respIfaceState.DEF_TYPE ).c_str(),
                   json.toString().c_str() );

    // Just to make sure, let's try to break it for a moment.
    // The ifaceId in the IfaceDesc message also needs to be > 0:
    respIfaceState.modIfaceDesc()[ 0 ].setIfaceId ( 0 );

    EXPECT_ERRCODE_EQ ( ProtoError::FieldValueOutOfRange, respIfaceState.validate() );

    // So, let's set it to 7 this time...
    respIfaceState.modIfaceDesc()[ 0 ].setIfaceId ( 7 );

    // And it's valid again!
    EXPECT_ERRCODE_EQ ( ProtoError::Success, respIfaceState.validate() );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, respIfaceState.serialize ( json ) );

    // This includes a number of fields we did NOT set.
    // Those are aliases that share the same bit storage field (not included in JSON output)
    // that are all considered "set" when at least one of them was set (and it was).

    EXPECT_STREQ ( String ( "{\"type\":%1,\"isCtrl\":true,\"isRemote\":false,\"isPubSub\":true,"
                            "\"isResponse\":true,\"srcAddr\":0,\"dstAddr\":0,\"isUnreliable\":false,"
                            "\"ifaceDesc\":[{\"isIfaceIdMsg\":1,\"ifaceId\":7,\"ifaceStatus\":\"IfaceUp\"}]}" )
                   .arg ( respIfaceState.DEF_TYPE ).c_str(),
                   json.toString().c_str() );

    // Now let's do something harder - serializeWithLength all the messages to the buffer!

    Buffer buf;

    size_t bufSize = 0;

    EXPECT_EQ ( bufSize, buf.size() );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientHello.serializeWithLength ( buf ) );

    EXPECT_GT ( buf.size(), bufSize );

    bufSize = buf.size();

    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientConfig.serializeWithLength ( buf ) );

    EXPECT_GT ( buf.size(), bufSize );

    bufSize = buf.size();

    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientRejected.serializeWithLength ( buf ) );

    EXPECT_GT ( buf.size(), bufSize );

    bufSize = buf.size();

    EXPECT_ERRCODE_EQ ( ProtoError::Success, reqIfaceState.serializeWithLength ( buf ) );

    EXPECT_GT ( buf.size(), bufSize );

    bufSize = buf.size();

    EXPECT_ERRCODE_EQ ( ProtoError::Success, respIfaceState.serializeWithLength ( buf ) );

    EXPECT_GT ( buf.size(), bufSize );

#ifdef DUMP_DATA
    dumpBuf.append ( "\n***** START: BASIC_TEST A\n\n" );
    ind = " ";
    clientHello.dumpDataDesc ( dumpBuf, ind );
    dumpBuf.append ( "\n" );
    ind = " ";
    clientConfig.dumpDataDesc ( dumpBuf, ind );
    dumpBuf.append ( "\n" );
    ind = " ";
    clientRejected.dumpDataDesc ( dumpBuf, ind );
    dumpBuf.append ( "\n" );
    ind = " ";
    reqIfaceState.dumpDataDesc ( dumpBuf, ind );
    dumpBuf.append ( "\n" );
    ind = " ";
    respIfaceState.dumpDataDesc ( dumpBuf, ind );
    dumpBuf.append ( "\n" );
    dumpBuf.append ( "***** END: BASIC_TEST A\n\n" );
#endif

    bufSize = buf.size();

    // Now let's try to deserialize them back!
    ClientHello clientHello2;
    ClientConfig clientConfig2;
    ClientRejected clientRejected2;
    PubSubReqIfaceState reqIfaceState2;
    PubSubRespIfaceState respIfaceState2;

    size_t offset = 0;
    size_t prevOffset;
    size_t offPubSubReq = 0;
    size_t offPubSubResp = 0;

    prevOffset = offset;

    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientHello2.deserializeWithLength ( buf, offset ) );

    EXPECT_GT ( offset, prevOffset );

    prevOffset = offset;

    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientConfig2.deserializeWithLength ( buf, offset ) );

    EXPECT_GT ( offset, prevOffset );

    prevOffset = offset;

    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientRejected2.deserializeWithLength ( buf, offset ) );

    EXPECT_GT ( offset, prevOffset );

    prevOffset = offset;
    offPubSubReq = offset;

    EXPECT_ERRCODE_EQ ( ProtoError::Success, reqIfaceState2.deserializeWithLength ( buf, offset ) );

    EXPECT_GT ( offset, prevOffset );

    prevOffset = offset;
    offPubSubResp = offset;

    EXPECT_ERRCODE_EQ ( ProtoError::Success, respIfaceState2.deserializeWithLength ( buf, offset ) );

    EXPECT_GT ( offset, prevOffset );

    EXPECT_EQ ( offset, buf.size() );

#ifdef DUMP_DATA
    dumpBuf.append ( "\n***** START: BASIC_TEST B\n\n" );
    ind = " ";
    clientHello2.dumpDataDesc ( dumpBuf, ind );
    dumpBuf.append ( "\n" );
    ind = " ";
    clientConfig2.dumpDataDesc ( dumpBuf, ind );
    dumpBuf.append ( "\n" );
    ind = " ";
    clientRejected2.dumpDataDesc ( dumpBuf, ind );
    dumpBuf.append ( "\n" );
    ind = " ";
    reqIfaceState2.dumpDataDesc ( dumpBuf, ind );
    dumpBuf.append ( "\n" );
    ind = " ";
    respIfaceState2.dumpDataDesc ( dumpBuf, ind );
    dumpBuf.append ( "\n\n" );
    dumpBuf.append ( "\n***** END: BASIC_TEST B\n" );
#endif

    // And let's run all the tests that were valid in the original messages:
    EXPECT_TRUE ( clientHello2.hasType() );
    EXPECT_TRUE ( clientConfig2.hasType() );
    EXPECT_TRUE ( clientRejected2.hasType() );

    EXPECT_FALSE ( clientHello2.getIsCtrl() );
    EXPECT_FALSE ( clientConfig2.getIsCtrl() );
    EXPECT_FALSE ( clientRejected2.getIsCtrl() );

    EXPECT_TRUE ( reqIfaceState2.hasType() );
    EXPECT_TRUE ( reqIfaceState2.hasIsCtrl() );
    EXPECT_TRUE ( reqIfaceState2.hasIsPubSub() );
    EXPECT_TRUE ( reqIfaceState2.getIsCtrl() );
    EXPECT_TRUE ( reqIfaceState2.getIsPubSub() );
    EXPECT_FALSE ( reqIfaceState2.getIsResponse() );

    EXPECT_TRUE ( respIfaceState2.hasType() );
    EXPECT_TRUE ( respIfaceState2.hasIsCtrl() );
    EXPECT_TRUE ( respIfaceState2.hasIsPubSub() );
    EXPECT_TRUE ( respIfaceState2.getIsCtrl() );
    EXPECT_TRUE ( respIfaceState2.getIsPubSub() );
    EXPECT_TRUE ( respIfaceState2.getIsResponse() );

    EXPECT_TRUE ( clientRejected2.hasErrCode() );
    EXPECT_TRUE ( reqIfaceState2.hasIfaceId() );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientHello2.validate() );
    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientConfig2.validate() );
    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientRejected2.validate() );
    EXPECT_ERRCODE_EQ ( ProtoError::Success, reqIfaceState2.validate() );
    EXPECT_ERRCODE_EQ ( ProtoError::Success, respIfaceState2.validate() );

    // And compare some of the values:

    EXPECT_EQ ( clientHello.getType(), clientHello2.getType() );
    EXPECT_EQ ( clientHello.getConfig(), clientHello2.getConfig() );
    EXPECT_EQ ( clientHello.getIsCtrl(), clientHello2.getIsCtrl() );

    EXPECT_EQ ( clientConfig.getType(), clientConfig2.getType() );
    EXPECT_EQ ( clientConfig.getConfig(), clientConfig2.getConfig() );
    EXPECT_EQ ( clientConfig.getIsCtrl(), clientConfig2.getIsCtrl() );

    EXPECT_EQ ( clientRejected.getType(), clientRejected2.getType() );
    EXPECT_EQ ( clientRejected.getConfig(), clientRejected2.getConfig() );
    EXPECT_EQ ( clientRejected.getIsCtrl(), clientRejected2.getIsCtrl() );

    EXPECT_EQ ( reqIfaceState.getType(), reqIfaceState2.getType() );
    EXPECT_EQ ( reqIfaceState.getConfig(), reqIfaceState2.getConfig() );
    EXPECT_EQ ( reqIfaceState.getIsCtrl(), reqIfaceState2.getIsCtrl() );
    EXPECT_EQ ( reqIfaceState.getIsPubSub(), reqIfaceState2.getIsPubSub() );
    EXPECT_EQ ( reqIfaceState.getIsRemote(), reqIfaceState2.getIsRemote() );
    EXPECT_EQ ( reqIfaceState.getIsUnreliable(), reqIfaceState2.getIsUnreliable() );

    EXPECT_EQ ( respIfaceState.getType(), respIfaceState2.getType() );
    EXPECT_EQ ( respIfaceState.getConfig(), respIfaceState2.getConfig() );
    EXPECT_EQ ( respIfaceState.getIsCtrl(), respIfaceState2.getIsCtrl() );
    EXPECT_EQ ( respIfaceState.getIsPubSub(), respIfaceState2.getIsPubSub() );
    EXPECT_EQ ( respIfaceState.getIsRemote(), respIfaceState2.getIsRemote() );
    EXPECT_EQ ( respIfaceState.getIsUnreliable(), respIfaceState2.getIsUnreliable() );

    EXPECT_EQ ( clientHello.getCertId(), clientHello2.getCertId() );

    EXPECT_EQ ( clientConfig.getAddrToUse().size(), clientConfig2.getAddrToUse().size() );

    ASSERT_EQ ( 1U, clientConfig.getAddrToUse().size() );
    ASSERT_EQ ( 1U, clientConfig2.getAddrToUse().size() );

    EXPECT_EQ ( clientConfig.getAddrToUse()[ 0 ], clientConfig2.getAddrToUse()[ 0 ] );

    EXPECT_EQ ( clientConfig.getDnsToUse().size(), clientConfig2.getDnsToUse().size() );

    ASSERT_EQ ( 1U, clientConfig.getDnsToUse().size() );
    ASSERT_EQ ( 1U, clientConfig2.getDnsToUse().size() );

    EXPECT_EQ ( clientConfig.getDnsToUse()[ 0 ], clientConfig2.getDnsToUse()[ 0 ] );

    // This should work, but GTest doesn't like it, as it's not really compatible with operator<<:
    // EXPECT_EQ ( clientRejected.getErrCode(), clientRejected2.getErrCode() );
    EXPECT_EQ ( clientRejected.getErrCode().value(), clientRejected2.getErrCode().value() );

    EXPECT_EQ ( reqIfaceState.getSubType(), reqIfaceState2.getSubType() );
    EXPECT_EQ ( reqIfaceState.getIfaceId(), reqIfaceState2.getIfaceId() );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, reqIfaceState.validate() );

    EXPECT_EQ ( respIfaceState.getIfaceDesc().size(), respIfaceState2.getIfaceDesc().size() );

    ASSERT_EQ ( 1U, respIfaceState.getIfaceDesc().size() );
    ASSERT_EQ ( 1U, respIfaceState2.getIfaceDesc().size() );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, respIfaceState2.getIfaceDesc()[ 0 ].validate() );

    // This should work, but GTest doesn't like it, as it's not really compatible with operator<<:
    // EXPECT_EQ ( respIfaceState.getIfaceDesc() [0].getIfaceStatus(),

    EXPECT_EQ ( respIfaceState.getIfaceDesc()[ 0 ].getIfaceStatus().value(),
                respIfaceState2.getIfaceDesc()[ 0 ].getIfaceStatus().value() );

    EXPECT_EQ ( respIfaceState.getIfaceDesc()[ 0 ].getIfaceId(),
                respIfaceState2.getIfaceDesc()[ 0 ].getIfaceId() );

    // That was easy. Now lets try to deserialize some of the messages as their base types:

    CtrlMsg ctrlMsg;
    PubSubReq pubSubReq;

    offset = offPubSubReq;

    // We get an 'error' - unknown token, but it is to be expected - we will see tokens from the actual message,
    // that the base message part has no idea about!
    EXPECT_ERRCODE_EQ ( ProtoError::ProtocolWarning, ctrlMsg.deserializeWithLength ( buf, offset ) );

    // At the same time this message should already be valid:
    EXPECT_ERRCODE_EQ ( ProtoError::Success, ctrlMsg.validate() );

    // .. and have the correct type:
    EXPECT_EQ ( reqIfaceState.getType(), ctrlMsg.getType() );

    // .. and config:
    EXPECT_EQ ( reqIfaceState.getConfig(), ctrlMsg.getConfig() );

    offset = offPubSubReq;

    // The same for PubSubRequest message:
    EXPECT_ERRCODE_EQ ( ProtoError::ProtocolWarning, pubSubReq.deserializeWithLength ( buf, offset ) );

    // At the same time this message should already be valid:
    EXPECT_ERRCODE_EQ ( ProtoError::Success, pubSubReq.validate() );

    // .. and have the correct type:
    EXPECT_EQ ( reqIfaceState.getType(), pubSubReq.getType() );

    // .. and config:
    EXPECT_EQ ( reqIfaceState.getConfig(), pubSubReq.getConfig() );

    // Let's also check the 'is_pub_sub' bit:
    EXPECT_EQ ( reqIfaceState.getIsPubSub(), pubSubReq.getIsPubSub() );

    // And similar checks for the response:

    CtrlRespMsg ctrlRespMsg;
    PubSubResp pubSubResp;

    offset = offPubSubResp;

    // We get an 'error' - unknown token, but it is to be expected - we will see tokens from the actual message,
    // that the base message part has no idea about!
    EXPECT_ERRCODE_EQ ( ProtoError::ProtocolWarning, ctrlRespMsg.deserializeWithLength ( buf, offset ) );

    // At the same time this message should already be valid:
    EXPECT_ERRCODE_EQ ( ProtoError::Success, ctrlRespMsg.validate() );

    // .. and have the correct type:
    EXPECT_EQ ( respIfaceState.getType(), ctrlRespMsg.getType() );

    // .. and config:
    EXPECT_EQ ( respIfaceState.getConfig(), ctrlRespMsg.getConfig() );

    // .. and response bit
    EXPECT_EQ ( respIfaceState.getIsResponse(), ctrlRespMsg.getIsResponse() );

    offset = offPubSubResp;

    // The same for PubSubRequest message:
    EXPECT_ERRCODE_EQ ( ProtoError::ProtocolWarning, pubSubResp.deserializeWithLength ( buf, offset ) );

    // At the same time this message should already be valid:
    EXPECT_ERRCODE_EQ ( ProtoError::Success, pubSubResp.validate() );

    // .. and have the correct type:
    EXPECT_EQ ( respIfaceState.getType(), pubSubResp.getType() );

    // .. and config:
    EXPECT_EQ ( respIfaceState.getConfig(), pubSubResp.getConfig() );

    // .. and response bit
    EXPECT_EQ ( respIfaceState.getIsResponse(), pubSubResp.getIsResponse() );

    // Let's also check the 'is_pub_sub' bit:
    EXPECT_EQ ( respIfaceState.getIsPubSub(), pubSubResp.getIsPubSub() );

    // On the other hand, these should fail:

    offset = offPubSubReq;

    // deserializing request message as any of those should give an error.
    // the actual error value depends on the message, some of them will say that
    // defined value was incorrect, others will try to deserialize fields as something they are not,
    // resulting in incomplete data, or invalid data size errors.

    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, ctrlRespMsg.deserializeWithLength ( buf, offset ) );

    offset = offPubSubReq;

    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, pubSubResp.deserializeWithLength ( buf, offset ) );

    offset = offPubSubReq;

    EXPECT_ERRCODE_EQ ( ProtoError::IncompleteData, respIfaceState2.deserializeWithLength ( buf, offset ) );

    offset = offPubSubReq;

    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientConfig2.deserializeWithLength ( buf, offset ) );

    offset = offPubSubReq;

    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientHello2.deserializeWithLength ( buf, offset ) );

    offset = offPubSubReq;

    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientRejected2.deserializeWithLength ( buf, offset ) );

    // and this as well:

    offset = offPubSubResp;

    // deserializing pub sub response message as any of those should fail.

    EXPECT_ERRCODE_EQ ( ProtoError::InvalidDataSize, reqIfaceState2.deserializeWithLength ( buf, offset ) );

    offset = offPubSubResp;

    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientConfig2.deserializeWithLength ( buf, offset ) );

    offset = offPubSubResp;

    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientHello2.deserializeWithLength ( buf, offset ) );

    offset = offPubSubResp;

    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientRejected2.deserializeWithLength ( buf, offset ) );

    // Let's also test BaseMessage-based deserialization:

    ClientHello clientHello3;
    ClientConfig clientConfig3;
    ClientRejected clientRejected3;
    PubSubReqIfaceState reqIfaceState3;
    PubSubRespIfaceState respIfaceState3;

    ProtoError errCode;
    size_t usedData = 0;

    offset = 0;

    MemHandle tmpBuf = buf.getHandle ( offset );

    General::BaseMsg baseMsg;

    usedData = 0;
    errCode = baseMsg.deserializeWithLength ( tmpBuf, usedData );

    offset += usedData;

    EXPECT_ERRCODE_EQ ( ProtoError::ProtocolWarning, errCode );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientHello3.deserialize ( baseMsg ) );

    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, ctrlMsg.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, ctrlRespMsg.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, pubSubReq.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, pubSubResp.deserialize ( baseMsg ) );

    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientConfig3.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientRejected3.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, reqIfaceState3.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, respIfaceState3.deserialize ( baseMsg ) );

    tmpBuf = buf.getHandle ( offset );

    usedData = 0;
    errCode = baseMsg.deserializeWithLength ( tmpBuf, usedData );

    offset += usedData;

    EXPECT_ERRCODE_EQ ( ProtoError::ProtocolWarning, errCode );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientConfig3.deserialize ( baseMsg ) );

    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, ctrlMsg.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, ctrlRespMsg.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, pubSubReq.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, pubSubResp.deserialize ( baseMsg ) );

    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientHello3.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientRejected3.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, reqIfaceState3.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, respIfaceState3.deserialize ( baseMsg ) );

    tmpBuf = buf.getHandle ( offset );

    usedData = 0;
    errCode = baseMsg.deserializeWithLength ( tmpBuf, usedData );

    offset += usedData;

    EXPECT_ERRCODE_EQ ( ProtoError::ProtocolWarning, errCode );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientRejected3.deserialize ( baseMsg ) );

    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, ctrlMsg.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, ctrlRespMsg.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, pubSubReq.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, pubSubResp.deserialize ( baseMsg ) );

    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientHello3.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientConfig3.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, reqIfaceState3.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, respIfaceState3.deserialize ( baseMsg ) );

    tmpBuf = buf.getHandle ( offset );

    usedData = 0;
    errCode = baseMsg.deserializeWithLength ( tmpBuf, usedData );

    offset += usedData;

    EXPECT_ERRCODE_EQ ( ProtoError::ProtocolWarning, errCode );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, reqIfaceState3.deserialize ( baseMsg ) );

    EXPECT_ERRCODE_EQ ( ProtoError::ProtocolWarning, pubSubReq.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::ProtocolWarning, ctrlMsg.deserialize ( baseMsg ) );

    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientHello3.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientConfig3.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientRejected3.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, respIfaceState3.deserialize ( baseMsg ) );

    tmpBuf = buf.getHandle ( offset );

    usedData = 0;
    errCode = baseMsg.deserializeWithLength ( tmpBuf, usedData );

    offset += usedData;

    EXPECT_ERRCODE_EQ ( ProtoError::ProtocolWarning, errCode );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, respIfaceState3.deserialize ( baseMsg ) );

    EXPECT_ERRCODE_EQ ( ProtoError::ProtocolWarning, pubSubResp.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::ProtocolWarning, ctrlRespMsg.deserialize ( baseMsg ) );

    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientHello3.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientConfig3.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, clientRejected3.deserialize ( baseMsg ) );
    EXPECT_ERRCODE_EQ ( ProtoError::DefinedValueMismatch, reqIfaceState3.deserialize ( baseMsg ) );

    EXPECT_EQ ( offset, buf.size() );

    tmpBuf.clear();
    buf.clear();

#ifdef DUMP_DATA
    char nc = 0;
    dumpBuf.appendData ( &nc, 1 ); // (To add NULL at the end)
    fprintf ( stderr, "%s\n", dumpBuf.get() );
    dumpBuf.clear();
#endif
}

TEST_F ( ProtoTest, FloatingPoint )
{
    double d = -1;

    uint64_t u64 = FloatingPointUtils::pack754 ( d );

    EXPECT_EQ ( 0xBFF0000000000000ULL, u64 );
    EXPECT_EQ ( d, FloatingPointUtils::unpack754 ( u64 ) );

    float f = -1;

    uint32_t u32 = FloatingPointUtils::pack754 ( f );
    EXPECT_EQ ( 0xBF800000, u32 );
    EXPECT_EQ ( f, FloatingPointUtils::unpack754 ( u32 ) );

    f = 18446744100000000000.0;

    u32 = FloatingPointUtils::pack754 ( f );
    EXPECT_EQ ( 0x5F800000, u32 );
    EXPECT_EQ ( f, FloatingPointUtils::unpack754 ( u32 ) );

    f = 18446744000000000000.0;

    u32 = FloatingPointUtils::pack754 ( f );
    EXPECT_EQ ( 0x5F800000, u32 );
    EXPECT_EQ ( f, FloatingPointUtils::unpack754 ( u32 ) );
}

static List<ValueStore> generateValues ( bool javaComp = false )
{
    List<ValueStore> ret;

    ValueStore tmp;

    tmp.setSignedC ( 0x40800000 );
    tmp.setUnsignedC ( 0x40800000 );

    ret.append ( tmp );

    tmp.setSignedC ( 0xBF800000 );
    tmp.setUnsignedC ( 0xBF800000 );

    ret.append ( tmp );

    tmp.setSignedC ( 0x5F800000 );
    tmp.setUnsignedC ( 0x5F800000 );

    ret.append ( tmp );

    // 0 at the end!
    double dVals[] = { 3.5, -0.0001, -1234.0009, ( 1.0 / 0.0 ), ( -1.0 / 0.0 ), 0 };

    int idx = 0;

    do
    {
        const double d = dVals[ idx ];
        const float f = ( float ) d;

        ValueStore vStoreA;

        vStoreA.setFloatingA ( f );
        vStoreA.setFloatingB ( d );

        ret.append ( vStoreA );
    }
    while ( dVals[ idx++ ] != 0 );

    for ( int pattern = 0; pattern < 4; ++pattern )
    {
        uint64_t u64 = 0;

        // 70 ( > 64 ) which can make a difference for some patterns
        for ( int b = 0; b <= 70; ++b )
        {
            ValueStore vStoreA;

            const uint32_t u32 = ( uint32_t ) u64;
            const uint16_t u16 = ( uint16_t ) u64;
            const uint8_t u8 = ( uint8_t ) u64;

            const int64_t s64 = ( int64_t ) u64;
            const int32_t s32 = ( int32_t ) u32;
            const int16_t s16 = ( int16_t ) u16;
            const int8_t s8 = ( int8_t ) u8;

            float f;
            double d;

            if ( javaComp )
            {
                // We use signed versions for compatibility with java...
                f = ( float ) s64;
                d = ( double ) s64;
            }
            else
            {
                f = ( float ) u64;
                d = ( double ) u64;
            }

            vStoreA.setUnsignedA ( u8 );
            vStoreA.setUnsignedB ( u16 );
            vStoreA.setUnsignedC ( u32 );
            vStoreA.setUnsignedD ( u64 );

            vStoreA.setSignedA ( s8 );
            vStoreA.setSignedB ( s16 );
            vStoreA.setSignedC ( s32 );
            vStoreA.setSignedD ( s64 );

            vStoreA.setFloatingA ( f );
            vStoreA.setFloatingB ( d );

            ret.append ( vStoreA );

            switch ( pattern )
            {
                case 0:
                    // Adding '1' bits on the right (from 0 to all 1s)
                    u64 <<= 1;
                    u64 |= 1;
                    break;

                case 1:
                    // Moving a single '1' bit
                    if ( u64 == 0 )
                        u64 = 1;

                    u64 <<= 1;
                    break;

                case 2:
                    // Every other bit is '1'
                    u64 <<= 1;
                    if ( ( u64 & 0x02 ) == 0 )
                    {
                        u64 |= 1;
                    }
                    break;

                case 3:
                    // Every three bits is '1'
                    u64 <<= 1;
                    if ( ( u64 & 0x03 ) == 0 )
                    {
                        u64 |= 1;
                    }
                    break;
            }
        }
    }

    tmp.clear();

    tmp.setSignedD ( ( int64_t ) 0x8000000000000000ULL );

    ret.append ( tmp );

    tmp.setSignedD ( ( int64_t ) ( 0x8000000000000000ULL - 1 ) );

    ret.append ( tmp );

    tmp.setSignedD ( ( int64_t ) ( 0x8000000000000000ULL + 1 ) );

    ret.append ( tmp );

    tmp.setSignedD ( ( ( int64_t ) 0x8000000000000000ULL ) + 1 );

    ret.append ( tmp );

    // We cannot use ( ((int64_t) 0x8000000000000000ULL) - 1 ), since that constant
    // is the lowest possible negative value for int64_t type.

    return ret;
}

TEST_F ( ProtoTest, SignedOverflow )
{
    // This test was failing with GCC 8.1.1 (possibly others too, but that's the only now that I know of)...
    // The code in ProtocolCodec was updated, to handle large negative values properly.

    ValueStore vStoreA;

    vStoreA.setSignedD ( ( int64_t ) 0x8000000000000000ULL );

    ASSERT_EQ ( ( int64_t ) 0x8000000000000000ULL, vStoreA.getSignedD() );

    Buffer buf;

    ASSERT_ERRCODE_EQ ( ProtoError::Success, vStoreA.serialize ( buf ) );

    EXPECT_EQ ( 11, buf.size() );

    ValueStore vStoreB;

    ASSERT_ERRCODE_EQ ( ProtoError::Success, vStoreB.deserialize ( buf ) );

    ASSERT_EQ ( vStoreA.getSignedD(), vStoreB.getSignedD() );

    vStoreA.clear();
    vStoreB.clear();
    buf.clear();

    vStoreA.setSignedA ( ( int8_t ) 0x80U );
    vStoreA.setSignedB ( ( int16_t ) 0x8000U );
    vStoreA.setSignedC ( ( int32_t ) 0x80000000U );
    vStoreA.setSignedD ( ( int64_t ) 0x8000000000000000ULL );

    ASSERT_ERRCODE_EQ ( ProtoError::Success, vStoreA.serialize ( buf ) );

    EXPECT_EQ ( 24, buf.size() );

    ASSERT_ERRCODE_EQ ( ProtoError::Success, vStoreB.deserialize ( buf ) );

    ASSERT_EQ ( vStoreA.getSignedA(), vStoreB.getSignedA() );
    ASSERT_EQ ( vStoreA.getSignedB(), vStoreB.getSignedB() );
    ASSERT_EQ ( vStoreA.getSignedC(), vStoreB.getSignedC() );
    ASSERT_EQ ( vStoreA.getSignedD(), vStoreB.getSignedD() );
}

TEST_F ( ProtoTest, NegativeEncoding )
{
    // This tests encoding of negative values, to make sure it didn't change
    // during changes to fix signed overflow issues.

    ValueStore vStoreA;

    vStoreA.setSignedA ( -123 );
    vStoreA.setSignedB ( -31234 );
    vStoreA.setSignedC ( -2123456789 );
    vStoreA.setSignedD ( -9123456789012345678 );

    Buffer buf;

    ASSERT_ERRCODE_EQ ( ProtoError::Success, vStoreA.serialize ( buf ) );

    EXPECT_EQ ( 22, buf.size() );
    EXPECT_STREQ ( "0x0F 0x7B 0x17 0x82 0xF4 0x01 0x1F 0x95 0xC2 0xC5 0xF4 "
                   "0x07 0x27 0xCE 0xE6 0xD3 0xC5 0xC8 0xF3 0xC1 0xCE 0x7E",
                   String::hexDump ( buf.get(), buf.size() ).c_str() );

    ValueStore vStoreB;

    ASSERT_ERRCODE_EQ ( ProtoError::Success, vStoreB.deserialize ( buf ) );

    ASSERT_EQ ( vStoreA.getSignedA(), vStoreB.getSignedA() );
    ASSERT_EQ ( vStoreA.getSignedB(), vStoreB.getSignedB() );
    ASSERT_EQ ( vStoreA.getSignedC(), vStoreB.getSignedC() );
    ASSERT_EQ ( vStoreA.getSignedD(), vStoreB.getSignedD() );
}

TEST_F ( ProtoTest, ValueTest )
{
    List<ValueStore> values = generateValues();

    for ( size_t i = 0; i < values.size(); ++i )
    {
        ValueStore vStoreA = values.at ( i );
        Buffer buf;

        ASSERT_ERRCODE_EQ ( ProtoError::Success, vStoreA.serialize ( buf ) );

        ValueStore vStoreB;

        ASSERT_ERRCODE_EQ ( ProtoError::Success, vStoreB.deserialize ( buf ) );

        EXPECT_EQ ( vStoreA.getUnsignedA(), vStoreB.getUnsignedA() );
        EXPECT_EQ ( vStoreA.getUnsignedB(), vStoreB.getUnsignedB() );
        EXPECT_EQ ( vStoreA.getUnsignedC(), vStoreB.getUnsignedC() );
        EXPECT_EQ ( vStoreA.getUnsignedD(), vStoreB.getUnsignedD() );

        EXPECT_EQ ( vStoreA.getSignedA(), vStoreB.getSignedA() );
        EXPECT_EQ ( vStoreA.getSignedB(), vStoreB.getSignedB() );
        EXPECT_EQ ( vStoreA.getSignedC(), vStoreB.getSignedC() );
        EXPECT_EQ ( vStoreA.getSignedD(), vStoreB.getSignedD() );

        if ( isnan ( vStoreA.getFloatingA() ) )
        {
            EXPECT_TRUE ( isnan ( vStoreB.getFloatingA() ) );
            EXPECT_TRUE ( isnan ( vStoreB.getFloatingB() ) );
        }
        else
        {
            EXPECT_EQ ( vStoreA.getFloatingA(), vStoreB.getFloatingA() );
            EXPECT_EQ ( vStoreA.getFloatingB(), vStoreB.getFloatingB() );
        }
    }
}

TEST_F ( ProtoTest, BaseMessageFieldTest )
{
    Container cnt;
    Buffer buf;

    // Missing iface_desc and base_msg:
    EXPECT_ERRCODE_EQ ( ProtoError::RequiredFieldNotSet, cnt.serialize ( buf ) );
    EXPECT_EQ ( ( size_t ) 0, buf.size() );

    PubSubRespIfaceState msg;
    IfaceDesc ifDesc;

    ifDesc.setIfaceId ( 1 );
    ifDesc.setIfaceStatus ( IfaceDesc::IfaceStatus::IfaceNotPresent );

    cnt.setIfaceDesc ( ifDesc );

    // Missing base_msg:
    EXPECT_ERRCODE_EQ ( ProtoError::RequiredFieldNotSet, cnt.serialize ( buf ) );
    EXPECT_EQ ( ( size_t ) 0, buf.size() );

    cnt.setBaseMsg ( msg );

    // base_msg is set, but has no elements in its internal iface_desc list:
    EXPECT_ERRCODE_EQ ( ProtoError::ListSizeOutOfRange, cnt.serialize ( buf ) );
    EXPECT_EQ ( ( size_t ) 0, buf.size() );

    ifDesc.clearIfaceId();

    msg.modIfaceDesc().append ( ifDesc );

    cnt.setBaseMsg ( msg );

    // base_msg is set and has a single iface_desc in its internal list, but that iface_desc is missing iface_id:
    EXPECT_ERRCODE_EQ ( ProtoError::RequiredFieldNotSet, cnt.serialize ( buf ) );
    EXPECT_EQ ( ( size_t ) 0, buf.size() );

    msg.clearIfaceDesc();

    ifDesc.setIfaceId ( 2 );
    ifDesc.setIfaceStatus ( IfaceDesc::IfaceStatus::IfaceUp );
    msg.modIfaceDesc().append ( ifDesc );

    // Let's add another one, later we check if there are two of them.
    ifDesc.setIfaceId ( 3 );
    ifDesc.setIfaceStatus ( IfaceDesc::IfaceStatus::IfaceDown );
    msg.modIfaceDesc().append ( ifDesc );

    cnt.setBaseMsg ( msg );

    cnt.setupDefines();

    // Now all should be fine:
    EXPECT_ERRCODE_EQ ( ProtoError::Success, cnt.validate() );

    // Let's also add this message (with different iface IDs) to a list (twice):

    ASSERT_EQ ( 2, msg.modIfaceDesc().size() );

    msg.modIfaceDesc()[ 0 ].setIfaceId ( 4 );
    msg.modIfaceDesc()[ 1 ].setIfaceId ( 5 );

    cnt.modBaseMsg3().append ( msg );

    ASSERT_EQ ( 2, msg.modIfaceDesc().size() );

    msg.modIfaceDesc()[ 0 ].setIfaceId ( 6 );
    msg.modIfaceDesc()[ 1 ].setIfaceId ( 7 );

    cnt.modBaseMsg3().append ( msg );

    ClientHello cHello;

    cHello.setCertId ( "qwertyuiop" );

    // TODO: Enable once it's added to proto/tests/test.proto
    // cHello.setTimestamp ( Timestamp() );

    cnt.modBaseMsg3().append ( cHello );

    Timestamp tStamp;

    ASSERT_TRUE ( tStamp.setUtcTime ( Timestamp::TimeDesc ( 2017, 4, 30, 21, 47, 15, 906 ) ) );

    cHello.setCertId ( "asdfghjkl" );

    // TODO: Enable once it's added to proto/tests/test.proto
    // cHello.setTimestamp ( tStamp );

    cnt.modBaseMsg3().append ( cHello );

    EXPECT_ERRCODE_EQ ( ProtoError::Success, cnt.serialize ( buf ) );

    Json json;

    EXPECT_ERRCODE_EQ ( ProtoError::Success, cnt.serialize ( json ) );
    EXPECT_STREQ ( String ( "{"
                            "\"baseMsg\":{"
                            "\"type\":%1,\"isCtrl\":true,\"isRemote\":false,\"isPubSub\":true,"
                            "\"isResponse\":true,\"srcAddr\":0,\"dstAddr\":0,\"isUnreliable\":false,"
                            "\"ifaceDesc\":["
                            "{\"isIfaceIdMsg\":1,\"ifaceId\":2,\"ifaceStatus\":\"IfaceUp\"}," // ifaceDesc[0]
                            "{\"isIfaceIdMsg\":1,\"ifaceId\":3,\"ifaceStatus\":\"IfaceDown\"}" // ifaceDesc[1]
                            "]"     // ifaceDesc[]
                            "},"     // baseMsg
                            "\"baseMsg3\":["
                            "{\"type\":%1,\"isCtrl\":true,\"isRemote\":false,\"isPubSub\":true,"
                            "\"isResponse\":true,\"srcAddr\":0,\"dstAddr\":0,\"isUnreliable\":false,"
                            "\"ifaceDesc\":["
                            "{\"isIfaceIdMsg\":1,\"ifaceId\":4,\"ifaceStatus\":\"IfaceUp\"}," // ifaceDesc[0]
                            "{\"isIfaceIdMsg\":1,\"ifaceId\":5,\"ifaceStatus\":\"IfaceDown\"}" // ifaceDesc[1]
                            "]"     // ifaceDesc[]
                            "},"     // baseMsg3[0]
                            "{\"type\":%1,\"isCtrl\":true,\"isRemote\":false,\"isPubSub\":true,"
                            "\"isResponse\":true,\"srcAddr\":0,\"dstAddr\":0,\"isUnreliable\":false,"
                            "\"ifaceDesc\":["
                            "{\"isIfaceIdMsg\":1,\"ifaceId\":6,\"ifaceStatus\":\"IfaceUp\"}," // ifaceDesc[0]
                            "{\"isIfaceIdMsg\":1,\"ifaceId\":7,\"ifaceStatus\":\"IfaceDown\"}" // ifaceDesc[1]
                            "]"     // ifaceDesc[]
                            "},"     // baseMsg3[1]
                            "{\"type\":%2,"
                            "" // TODO: Re-Enable: "\"timestamp\":\"0000-01-01T00:00:00.000Z\","
                            "\"certId\":\"qwertyuiop\""
                            "},"     // baseMsg3[2]
                            "{\"type\":%2,"
                            "" // TODO: Re-Enable: "\"timestamp\":\"2017-04-30T21:47:15.906Z\","
                            "\"certId\":\"asdfghjkl\""
                            "}"     // baseMsg3[3]
                            "],"     // baseMsg3[]
                            "\"ifaceDesc\":{"
                            "\"isIfaceIdMsg\":1,\"ifaceId\":1,\"ifaceStatus\":\"IfaceNotPresent\""
                            "}"     // ifaceDesc
                            "}" )
                   .arg ( msg.DEF_TYPE ).arg ( cHello.DEF_TYPE ).c_str(),
                   json.toString().c_str() );

    cnt.clear();

    Container cnt2;

    EXPECT_ERRCODE_EQ ( ProtoError::Success, cnt2.deserialize ( buf ) );

    EXPECT_TRUE ( cnt2.hasIfaceDesc() );
    EXPECT_TRUE ( cnt2.getIfaceDesc().hasIfaceStatus() );
    EXPECT_EQ ( IfaceDesc::IfaceStatus::IfaceNotPresent, cnt2.getIfaceDesc().getIfaceStatus().value() );
    EXPECT_EQ ( 1, cnt2.getIfaceDesc().getIfaceId() );

    EXPECT_TRUE ( cnt2.hasBaseMsg() );
    EXPECT_FALSE ( cnt2.hasBaseMsg2() );

    EXPECT_TRUE ( cnt2.getBaseMsg().getIsCtrl() );

    EXPECT_EQ ( PubSubRespIfaceState::DEF_TYPE, cnt2.getBaseMsg().getType() );

    ASSERT_EQ ( 4, cnt2.getBaseMsg3().size() );

    EXPECT_EQ ( PubSubRespIfaceState::DEF_TYPE, cnt2.getBaseMsg3().at ( 0 ).getObject().getType() );
    EXPECT_EQ ( PubSubRespIfaceState::DEF_TYPE, cnt2.getBaseMsg3().at ( 1 ).getObject().getType() );
    EXPECT_EQ ( ClientHello::DEF_TYPE, cnt2.getBaseMsg3().at ( 2 ).getObject().getType() );
    EXPECT_EQ ( ClientHello::DEF_TYPE, cnt2.getBaseMsg3().at ( 3 ).getObject().getType() );

    CtrlMsg cm;

    EXPECT_ERRCODE_EQ ( ProtoError::ProtocolWarning, cm.deserialize ( cnt2.getBaseMsg() ) );
    EXPECT_TRUE ( cm.getIsResponse() );
    EXPECT_TRUE ( cm.getIsPubSub() );

    CtrlRespMsg crm;

    EXPECT_ERRCODE_EQ ( ProtoError::ProtocolWarning, crm.deserialize ( cnt2.getBaseMsg() ) );
    EXPECT_TRUE ( crm.getIsResponse() );
    EXPECT_TRUE ( crm.getIsPubSub() );

    PubSubResp psr;

    EXPECT_ERRCODE_EQ ( ProtoError::ProtocolWarning, psr.deserialize ( cnt2.getBaseMsg() ) );
    EXPECT_TRUE ( psr.getIsResponse() );
    EXPECT_TRUE ( psr.getIsPubSub() );

    PubSubRespIfaceState respIfSt;

    // Let's make a copy first (this one is more tricky than using it directly):
    BaseMsg baseMsg = cnt2.getBaseMsg();

    EXPECT_ERRCODE_EQ ( ProtoError::Success, respIfSt.deserialize ( baseMsg ) );
    EXPECT_TRUE ( respIfSt.getIsResponse() );
    EXPECT_TRUE ( respIfSt.getIsPubSub() );

    EXPECT_EQ ( PubSubRespIfaceState::DEF_TYPE, respIfSt.getType() );

    ASSERT_EQ ( ( size_t ) 2, respIfSt.getIfaceDesc().size() );

    EXPECT_TRUE ( respIfSt.getIfaceDesc().at ( 0 ).hasIfaceStatus() );
    EXPECT_EQ ( IfaceDesc::IfaceStatus::IfaceUp, respIfSt.getIfaceDesc().at ( 0 ).getIfaceStatus().value() );
    EXPECT_EQ ( 2, respIfSt.getIfaceDesc().at ( 0 ).getIfaceId() );

    EXPECT_TRUE ( respIfSt.getIfaceDesc().at ( 1 ).hasIfaceStatus() );
    EXPECT_EQ ( IfaceDesc::IfaceStatus::IfaceDown, respIfSt.getIfaceDesc().at ( 1 ).getIfaceStatus().value() );
    EXPECT_EQ ( 3, respIfSt.getIfaceDesc().at ( 1 ).getIfaceId() );

    ASSERT_EQ ( 4, cnt2.getBaseMsg3().size() );

    // First base msg in the list (iface ID 4 and 5):
    baseMsg = cnt2.getBaseMsg3().at ( 0 );
    EXPECT_ERRCODE_EQ ( ProtoError::Success, respIfSt.deserialize ( baseMsg ) );
    EXPECT_TRUE ( respIfSt.getIsResponse() );
    EXPECT_TRUE ( respIfSt.getIsPubSub() );

    EXPECT_EQ ( PubSubRespIfaceState::DEF_TYPE, respIfSt.getType() );

    ASSERT_EQ ( ( size_t ) 2, respIfSt.getIfaceDesc().size() );

    EXPECT_TRUE ( respIfSt.getIfaceDesc().at ( 0 ).hasIfaceStatus() );
    EXPECT_EQ ( IfaceDesc::IfaceStatus::IfaceUp, respIfSt.getIfaceDesc().at ( 0 ).getIfaceStatus().value() );
    EXPECT_EQ ( 4, respIfSt.getIfaceDesc().at ( 0 ).getIfaceId() );

    EXPECT_TRUE ( respIfSt.getIfaceDesc().at ( 1 ).hasIfaceStatus() );
    EXPECT_EQ ( IfaceDesc::IfaceStatus::IfaceDown, respIfSt.getIfaceDesc().at ( 1 ).getIfaceStatus().value() );
    EXPECT_EQ ( 5, respIfSt.getIfaceDesc().at ( 1 ).getIfaceId() );

    // Second base msg in the list (iface ID 6 and 7).
    // Also, we use the base message directly from the list, not through a copy.
    EXPECT_ERRCODE_EQ ( ProtoError::Success, respIfSt.deserialize ( cnt2.getBaseMsg3().at ( 1 ) ) );
    EXPECT_TRUE ( respIfSt.getIsResponse() );
    EXPECT_TRUE ( respIfSt.getIsPubSub() );

    EXPECT_EQ ( PubSubRespIfaceState::DEF_TYPE, respIfSt.getType() );

    ASSERT_EQ ( ( size_t ) 2, respIfSt.getIfaceDesc().size() );

    EXPECT_TRUE ( respIfSt.getIfaceDesc().at ( 0 ).hasIfaceStatus() );
    EXPECT_EQ ( IfaceDesc::IfaceStatus::IfaceUp, respIfSt.getIfaceDesc().at ( 0 ).getIfaceStatus().value() );
    EXPECT_EQ ( 6, respIfSt.getIfaceDesc().at ( 0 ).getIfaceId() );

    EXPECT_TRUE ( respIfSt.getIfaceDesc().at ( 1 ).hasIfaceStatus() );
    EXPECT_EQ ( IfaceDesc::IfaceStatus::IfaceDown, respIfSt.getIfaceDesc().at ( 1 ).getIfaceStatus().value() );
    EXPECT_EQ ( 7, respIfSt.getIfaceDesc().at ( 1 ).getIfaceId() );

    // Third base msg in the list (ClientHello)
    ClientHello tmpHello;

    EXPECT_ERRCODE_EQ ( ProtoError::Success, tmpHello.deserialize ( cnt2.getBaseMsg3().at ( 2 ) ) );
    EXPECT_EQ ( ClientHello::DEF_TYPE, tmpHello.getType() );
    EXPECT_STREQ ( "qwertyuiop", tmpHello.getCertId().c_str() );

    // TODO: Enable once it's added to common/tests/proto/test.proto
    // EXPECT_EQ ( Timestamp::MinBinValue, tmpHello.getTimestamp().getBinValue() );

    // Fourth base msg in the list (ClientHello)
    tmpHello.clear();

    EXPECT_ERRCODE_EQ ( ProtoError::Success, tmpHello.deserialize ( cnt2.getBaseMsg3().at ( 3 ) ) );
    EXPECT_EQ ( ClientHello::DEF_TYPE, tmpHello.getType() );
    EXPECT_STREQ ( "asdfghjkl", tmpHello.getCertId().c_str() );

    // TODO: Enable once it's added to common/tests/proto/test.proto
    // EXPECT_EQ ( tStamp.getBinValue(), tmpHello.getTimestamp().getBinValue() );
}

TEST_F ( ProtoTest, FileIOTest )
{
    List<ValueStore> values = generateValues ( true );

#ifndef DAT_FILE
    MemHandle fakeFile;
#endif

    for ( int encMethod = 0; encMethod < 2; ++encMethod )
    {
        // We try to read that file TWICE - once at the beginning (unless it doesn't exist)
        // to read whatever was created by any other test (Java version), and then again,
        // to read the version that we generated
        for ( int attempt = 0; attempt < 2; ++attempt )
        {
            MemHandle inBuf;

#ifdef DAT_FILE
            if ( inBuf.readFile ( DAT_FILE ) )
#else
            inBuf = fakeFile;

            if ( !inBuf.isEmpty() )
#endif
            {
                ClientHello clientHello;
                ClientConfig clientConfig;
                ClientRejected clientRejected;
                PubSubReqIfaceState reqIfaceState;
                PubSubRespIfaceState respIfaceState;
                Container container;
                ValueMessage valMessage;

                size_t off = 0;
                ASSERT_ERRCODE_EQ ( ProtoError::Success, clientHello.deserializeWithLength ( inBuf, off ) );
                ASSERT_ERRCODE_EQ ( ProtoError::Success, clientConfig.deserializeWithLength ( inBuf, off ) );
                ASSERT_ERRCODE_EQ ( ProtoError::Success, clientRejected.deserializeWithLength ( inBuf, off ) );
                ASSERT_ERRCODE_EQ ( ProtoError::Success, reqIfaceState.deserializeWithLength ( inBuf, off ) );
                ASSERT_ERRCODE_EQ ( ProtoError::Success, respIfaceState.deserializeWithLength ( inBuf, off ) );
                ASSERT_ERRCODE_EQ ( ProtoError::Success, container.deserializeWithLength ( inBuf, off ) );
                ASSERT_ERRCODE_EQ ( ProtoError::Success, valMessage.deserializeWithLength ( inBuf, off ) );

                ASSERT_ERRCODE_EQ ( ProtoError::Success, clientHello.validate() );
                ASSERT_ERRCODE_EQ ( ProtoError::Success, clientConfig.validate() );
                ASSERT_ERRCODE_EQ ( ProtoError::Success, clientRejected.validate() );
                ASSERT_ERRCODE_EQ ( ProtoError::Success, reqIfaceState.validate() );
                ASSERT_ERRCODE_EQ ( ProtoError::Success, respIfaceState.validate() );
                ASSERT_ERRCODE_EQ ( ProtoError::Success, container.validate() );
                ASSERT_ERRCODE_EQ ( ProtoError::Success, valMessage.validate() );

                PubSubRespIfaceState respIfSt;

                ProtoError eCode = respIfSt.deserialize ( container.getBaseMsg() );

                EXPECT_ERRCODE_EQ ( ProtoError::Success, eCode );

                ASSERT_EQ ( ( size_t ) 2, respIfSt.getIfaceDesc().size() );

                EXPECT_EQ ( 3, respIfSt.getIfaceDesc().at ( 0 ).getIfaceId() );
                EXPECT_EQ ( 10, respIfSt.getIfaceDesc().at ( 1 ).getIfaceId() );

                EXPECT_EQ ( IfaceDesc::IfaceStatus::IfaceUp,
                            respIfSt.getIfaceDesc().at ( 0 ).getIfaceStatus().value() );
                EXPECT_EQ ( IfaceDesc::IfaceStatus::IfaceDown,
                            respIfSt.getIfaceDesc().at ( 1 ).getIfaceStatus().value() );

                EXPECT_EQ ( IfaceDesc::IfaceStatus::IfaceNotPresent,
                            container.getIfaceDesc().getIfaceStatus().value() );
                EXPECT_EQ ( 15, container.getIfaceDesc().getIfaceId() );

                EXPECT_TRUE ( respIfSt.hasSettableBit() );
                EXPECT_FALSE ( respIfSt.hasSettableField() );

                EXPECT_EQ ( true, respIfSt.getSettableBit() );
                EXPECT_EQ ( 0, respIfSt.getSettableField() );

                EXPECT_EQ ( values.size(), valMessage.getValues().size() );

                for ( size_t i = 0; i < values.size() && i < valMessage.getValues().size(); ++i )
                {
                    ValueStore vStoreA = values.at ( i );
                    ValueStore vStoreB = valMessage.getValues().at ( i );

                    EXPECT_EQ ( vStoreA.getUnsignedA(), vStoreB.getUnsignedA() );
                    EXPECT_EQ ( vStoreA.getUnsignedB(), vStoreB.getUnsignedB() );
                    EXPECT_EQ ( vStoreA.getUnsignedC(), vStoreB.getUnsignedC() );
                    EXPECT_EQ ( vStoreA.getUnsignedD(), vStoreB.getUnsignedD() );

                    EXPECT_EQ ( vStoreA.getSignedA(), vStoreB.getSignedA() );
                    EXPECT_EQ ( vStoreA.getSignedB(), vStoreB.getSignedB() );
                    EXPECT_EQ ( vStoreA.getSignedC(), vStoreB.getSignedC() );
                    EXPECT_EQ ( vStoreA.getSignedD(), vStoreB.getSignedD() );

                    if ( isnan ( vStoreA.getFloatingA() ) )
                    {
                        EXPECT_TRUE ( isnan ( vStoreB.getFloatingA() ) );
                        EXPECT_TRUE ( isnan ( vStoreB.getFloatingB() ) );
                    }
                    else
                    {
                        EXPECT_EQ ( vStoreA.getFloatingA(), vStoreB.getFloatingA() );
                        EXPECT_EQ ( vStoreA.getFloatingB(), vStoreB.getFloatingB() );
                    }
                }

#ifdef DUMP_DATA
                Buffer dumpBuf;
                dumpBuf.append ( "\n***** START: FILE_IO_TEST (ENC_METHOD: " );
                dumpBuf.append ( String::number ( encMethod ) );
                dumpBuf.append ( "; PASS: " );
                dumpBuf.append ( String::number ( attempt ) );
                dumpBuf.append ( ")\n\n" );
                String ind = " ";
                clientHello.dumpDataDesc ( dumpBuf, ind );
                dumpBuf.append ( "\n" );
                ind = " ";
                clientConfig.dumpDataDesc ( dumpBuf, ind );
                dumpBuf.append ( "\n" );
                ind = " ";
                clientRejected.dumpDataDesc ( dumpBuf, ind );
                dumpBuf.append ( "\n" );
                ind = " ";
                reqIfaceState.dumpDataDesc ( dumpBuf, ind );
                dumpBuf.append ( "\n" );
                ind = " ";
                respIfaceState.dumpDataDesc ( dumpBuf, ind );
                dumpBuf.append ( "\n" );
                ind = " ";
                container.dumpDataDesc ( dumpBuf, ind );
                dumpBuf.append ( "\n" );

                dumpBuf.append ( "\n***** END: FILE_IO_TEST (ENC_METHOD: " );
                dumpBuf.append ( String::number ( encMethod ) );
                dumpBuf.append ( "; PASS: " );
                dumpBuf.append ( String::number ( attempt ) );
                dumpBuf.append ( ")\n" );

                char nc = 0;
                dumpBuf.appendData ( &nc, 1 ); // (To add NULL at the end)
                fprintf ( stderr, "%s\n", dumpBuf.get() );

                dumpBuf.clear();
#endif

                EXPECT_EQ ( off, inBuf.size() );
            }

            inBuf.clear();

            // We only want to write the data once!
            if ( attempt == 0 )
            {
                ClientHello clientHello;
                ClientConfig clientConfig;
                ClientRejected clientRejected;
                PubSubReqIfaceState reqIfaceState;
                PubSubRespIfaceState respIfaceState;
                Container container;
                ValueMessage valMessage;

                clientHello.setupDefines();
                clientConfig.setupDefines();
                clientRejected.setupDefines();
                reqIfaceState.setupDefines();
                respIfaceState.setupDefines();
                container.setupDefines();

                clientHello.setCertId ( "abcdefghij" );
                clientConfig.modAddrToUse().append ( IpAddress ( "127.0.0.1" ) );
                clientConfig.modDnsToUse().append ( "8.8.8.8" );
                clientRejected.setErrCode ( General::TestCode::code_c );

                reqIfaceState.setSubType ( 15 );
                reqIfaceState.setIfaceId ( 5 );

                IfaceDesc ifDesc;
                ifDesc.setupDefines();
                ifDesc.setIfaceId ( 5 );
                ifDesc.setIfaceStatus ( IfaceDesc::IfaceStatus::IfaceUp );

                respIfaceState.modIfaceDesc().append ( ifDesc );

                PubSubRespIfaceState msg;

                ifDesc.setIfaceId ( 15 );
                ifDesc.setIfaceStatus ( IfaceDesc::IfaceStatus::IfaceNotPresent );
                container.setIfaceDesc ( ifDesc );
                ifDesc.setIfaceId ( 3 );
                ifDesc.setIfaceStatus ( IfaceDesc::IfaceStatus::IfaceUp );
                msg.modIfaceDesc().append ( ifDesc );
                ifDesc.setIfaceId ( 10 );
                ifDesc.setIfaceStatus ( IfaceDesc::IfaceStatus::IfaceDown );
                msg.modIfaceDesc().append ( ifDesc );

                msg.setSettableBit ( true );

                EXPECT_TRUE ( msg.hasSettableBit() );
                EXPECT_FALSE ( msg.hasSettableField() );

                EXPECT_EQ ( true, msg.getSettableBit() );
                EXPECT_EQ ( 0, msg.getSettableField() );

                container.setBaseMsg ( msg );
                valMessage.modValues() = values;

                EXPECT_ERRCODE_EQ ( ProtoError::Success, clientHello.validate() );
                EXPECT_ERRCODE_EQ ( ProtoError::Success, clientConfig.validate() );
                EXPECT_ERRCODE_EQ ( ProtoError::Success, clientRejected.validate() );
                EXPECT_ERRCODE_EQ ( ProtoError::Success, reqIfaceState.validate() );
                EXPECT_ERRCODE_EQ ( ProtoError::Success, respIfaceState.validate() );

                container.setupDefines();
                EXPECT_ERRCODE_EQ ( ProtoError::Success, container.validate() );

                EXPECT_ERRCODE_EQ ( ProtoError::Success, valMessage.validate() );

                Buffer buf;

                if ( encMethod == 0 )
                {
                    ProtoError eCode = ProtoError::Unknown;

                    MemHandle handle;

                    handle = clientHello.serializeWithLength ( &eCode );
                    EXPECT_ERRCODE_EQ ( ProtoError::Success, eCode );
                    ASSERT_TRUE ( handle.size() > 0 );
                    buf.append ( handle );

                    handle = clientConfig.serializeWithLength ( &eCode );
                    EXPECT_ERRCODE_EQ ( ProtoError::Success, eCode );
                    ASSERT_TRUE ( handle.size() > 0 );
                    buf.append ( handle );

                    handle = clientRejected.serializeWithLength ( &eCode );
                    EXPECT_ERRCODE_EQ ( ProtoError::Success, eCode );
                    ASSERT_TRUE ( handle.size() > 0 );
                    buf.append ( handle );

                    handle = reqIfaceState.serializeWithLength ( &eCode );
                    EXPECT_ERRCODE_EQ ( ProtoError::Success, eCode );
                    ASSERT_TRUE ( handle.size() > 0 );
                    buf.append ( handle );

                    handle = respIfaceState.serializeWithLength ( &eCode );
                    EXPECT_ERRCODE_EQ ( ProtoError::Success, eCode );
                    ASSERT_TRUE ( handle.size() > 0 );
                    buf.append ( handle );

                    handle = container.serializeWithLength ( &eCode );
                    EXPECT_ERRCODE_EQ ( ProtoError::Success, eCode );
                    ASSERT_TRUE ( handle.size() > 0 );
                    buf.append ( handle );

                    handle = valMessage.serializeWithLength ( &eCode );
                    EXPECT_ERRCODE_EQ ( ProtoError::Success, eCode );
                    ASSERT_TRUE ( handle.size() > 0 );
                    buf.append ( handle );
                }
                else
                {
                    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientHello.serializeWithLength ( buf ) );
                    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientConfig.serializeWithLength ( buf ) );
                    EXPECT_ERRCODE_EQ ( ProtoError::Success, clientRejected.serializeWithLength ( buf ) );
                    EXPECT_ERRCODE_EQ ( ProtoError::Success, reqIfaceState.serializeWithLength ( buf ) );
                    EXPECT_ERRCODE_EQ ( ProtoError::Success, respIfaceState.serializeWithLength ( buf ) );
                    EXPECT_ERRCODE_EQ ( ProtoError::Success, container.serializeWithLength ( buf ) );
                    EXPECT_ERRCODE_EQ ( ProtoError::Success, valMessage.serializeWithLength ( buf ) );
                }

#ifdef DAT_FILE
                buf.writeToFile ( DAT_FILE );
#else
                fakeFile = buf;
#endif

                buf.clear();
            }
        }
    }
}
