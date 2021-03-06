//
// NetException.cpp
//
// $Id: //poco/1.4/Net/src/NetException.cpp#4 $
//
// Library: Net
// Package: NetCore
// Module:  NetException
//
// Copyright (c) 2005-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "Poco/Net/NetException.h"
#include <typeinfo>


using Poco::IOException;


namespace Poco {
namespace Net {


POCO_IMPLEMENT_EXCEPTION(NetException, IOException, "Net Exception")
POCO_IMPLEMENT_EXCEPTION(InvalidAddressException, NetException, "Invalid address")
POCO_IMPLEMENT_EXCEPTION(InvalidSocketException, NetException, "Invalid socket")
POCO_IMPLEMENT_EXCEPTION(ServiceNotFoundException, NetException, "Service not found")
POCO_IMPLEMENT_EXCEPTION(ConnectionAbortedException, NetException, "Software caused connection abort")
POCO_IMPLEMENT_EXCEPTION(ConnectionResetException, NetException, "Connection reset by peer")
POCO_IMPLEMENT_EXCEPTION(ConnectionRefusedException, NetException, "Connection refused")
POCO_IMPLEMENT_EXCEPTION(DNSException, NetException, "DNS error")
POCO_IMPLEMENT_EXCEPTION(HostNotFoundException, DNSException, "Host not found")
POCO_IMPLEMENT_EXCEPTION(NoAddressFoundException, DNSException, "No address found")
POCO_IMPLEMENT_EXCEPTION(InterfaceNotFoundException, NetException, "Interface not found")
POCO_IMPLEMENT_EXCEPTION(NoMessageException, NetException, "No message received")
POCO_IMPLEMENT_EXCEPTION(MessageException, NetException, "Malformed message")
POCO_IMPLEMENT_EXCEPTION(MultipartException, MessageException, "Malformed multipart message")
POCO_IMPLEMENT_EXCEPTION(HTTPException, NetException, "HTTP Exception")
POCO_IMPLEMENT_EXCEPTION(NotAuthenticatedException, HTTPException, "No authentication information found")
POCO_IMPLEMENT_EXCEPTION(UnsupportedRedirectException, HTTPException, "Unsupported HTTP redirect (protocol change)")
POCO_IMPLEMENT_EXCEPTION(FTPException, NetException, "FTP Exception")
POCO_IMPLEMENT_EXCEPTION(SMTPException, NetException, "SMTP Exception")
POCO_IMPLEMENT_EXCEPTION(POP3Exception, NetException, "POP3 Exception")
POCO_IMPLEMENT_EXCEPTION(ICMPException, NetException, "ICMP Exception")
POCO_IMPLEMENT_EXCEPTION(HTMLFormException, NetException, "HTML Form Exception")
POCO_IMPLEMENT_EXCEPTION(WebSocketException, NetException, "WebSocket Exception")


} } // namespace Poco::Net
