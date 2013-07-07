// entry.reg@gmail.com :: https://bitbucket.org/3F/flightsdc/
//draft

#include "stdafx.h"
#include "Rpc.hpp"

#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Net/NetException.h"

using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::HTTPServerParams;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPResponse;
using Poco::Net::WebSocket;
using Poco::Net::WebSocketException;

namespace reg {
namespace p2p {
namespace rpc {

    ServiceMethods Server::_serviceMethods;

    class WebSocketsHandler: public HTTPRequestHandler
    {
    public:

        /**
         * main transfer
         */
	    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
	    {
		    try{
                WebSocket ws(request, response);
                //TODO: ws.setReceiveTimeout / Poco::Timespan(0,0); etc.,

                int flags = 0, count;
                char buf[1024];
                do{
                    //TODO: size of buffer
                    // -_- не понравилась реализация приема(см. WebSocketImpl::receiveBytes), по сути length как пятое колесо, не туда и не сюда...
                    count = ws.receiveFrame(buf, sizeof(buf), flags); // notice: receive->waiting->receive...
                    //count = ws.receiveBytes(buf, 128, flags);

                    std::string data(buf, count);
                    std::string resultJson = handleRequest(data);

                    ws.sendFrame(resultJson.c_str(), resultJson.length(), flags);

                }while((flags & WebSocket::FRAME_OP_BITMASK) != WebSocket::FRAME_OP_CLOSE);
                //connection closed!
		    }
		    catch(WebSocketException& exc){
                switch(exc.code()){
			        case WebSocket::WS_ERR_NO_HANDSHAKE:
			        case WebSocket::WS_ERR_HANDSHAKE_NO_KEY:{
				        response.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
				        response.setContentLength(0);
				        response.send();
                        return;
                    }
                    case WebSocket::WS_ERR_HANDSHAKE_UNSUPPORTED_VERSION:
                    case WebSocket::WS_ERR_HANDSHAKE_NO_VERSION:{
                        response.set("Sec-WebSocket-Version", WebSocket::WEBSOCKET_VERSION);
                        return;
                    }
                }
                //TODO: throw custom
		    }
	    };

        /**
         * call service methods
         */
        std::string handleRequest(const std::string& request)
        {
            boost::scoped_ptr<JsonResponse> result;
            result.reset(new JsonResponse(0));

            try{
                JsonRequest json(request);

                ServiceMethods methods      = Server::services();
                std::string called          = json.getMethod();
                result->getResponse()["id"] = json.getId();
                ServiceMethod serviceMethod;

                Lock l(cs);
                ServiceMethods::iterator it = methods.find(called);
                if(it != methods.end()){
                    serviceMethod = it->second;
                }

                if(serviceMethod){
                    serviceMethod(json, *result);
                }
                else{
                    throw JsonRequestException("Unknown method: " + called); // TODO: customized exception with specifics of the protocol
                    result->getResponse()["error"] = "Unknown method: " + called;
                }
            }
            catch(...){
                result->getResponse()["result"] = json_spirit::mValue();

                //TODO: change to protocol specification
                try{
                    throw;
                }
                catch(const JsonRequestException& e){
                    result->getResponse()["error"] = e.getMessage(); // TODO: customized
                }
                catch(const std::string& eJsonSpirit){
                    result->getResponse()["error"] = eJsonSpirit;
                }
                catch(const std::exception& e){
                    result->getResponse()["error"] = e.what();
                }
                catch(...){
                    result->getResponse()["error"] = "unknown type";
                }
            }

            //std::string utf8String("");
            return json_spirit::write_string(json_spirit::mValue(result->getResponse()), json_spirit::pretty_print);
        };


    private:
        CriticalSection cs;
    };

    class RequestHandler: public HTTPRequestHandlerFactory
    {
    public:
	    HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request)
	    {
		    return new WebSocketsHandler(); // Smart pointer: std::auto_ptr<HTTPRequestHandler> pHandler(_pFactory->createRequestHandler(request));
	    }
    };

    bool Server::_run()
    {
        try{
            Poco::Net::ServerSocket ssock(_port);
            _server = new HTTPServer(new RequestHandler(), ssock, new HTTPServerParams()); // Smart pointer: RequestHandler -> SharedPtr  & HTTPServerParams -> AutoPtr
            _server->start();
            return true;
        }
        catch(...){
            //TODO:
        }
        return false;
    };

    bool Server::_shutdown()
    {
        Server::instance()->stop();
        delete Server::_server;
        //TODO: services
        return true;
    }

    HTTPServer* Server::instance()
    {
        return Server::_server;
    }

    void Server::bindMethod(const std::string& name, ServiceMethod method)
    {
        Lock l(cs);
        _serviceMethods[name] = method;
    }

    void Server::unbindMethod(const std::string& name)
    {
        Lock l(cs);
        ServiceMethods::iterator it = _serviceMethods.find(name);
        if(it != _serviceMethods.end()){
            _serviceMethods.erase(it);
        }
    }

}}};