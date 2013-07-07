// entry.reg@gmail.com :: https://bitbucket.org/3F/flightsdc/
//draft

#pragma once

#include "Poco/Net/HTTPServer.h"
#include "json_spirit_reader_template.h"
#include "json_spirit_writer_template.h"
#include "Exceptions/JsonRequestException.hpp"
#include <boost/thread.hpp>

using Poco::Net::HTTPServer;

namespace reg {
namespace p2p {
namespace rpc {
        
    // * draft for compatibility [RPC-hxr] *
    //TODO: 2.0 Specification - http://www.jsonrpc.org/specification

    /**
     * Response Object
     */
    class JsonResponse
    {
    public:
        JsonResponse(boost::uint64_t id)
        {
            //_response["jsonrpc"]    = "2.0";
            //as the value of the id member in the Request Object
            _response["id"]         = id; //If there was an error in detecting the id in the Request object (e.g. Parse error/Invalid Request), it MUST be Null
            //is REQUIRED on success
            _response["result"]     = json_spirit::mValue();
            //is REQUIRED on error + MUST be an Object
            _response["error"]      = json_spirit::mValue();
        };

        json_spirit::mObject& getResponse()
        {
            return _response;
        };

    private:
        json_spirit::mObject _response;

    };

    /**
     * Request Object
     */
    class JsonRequest
    {
    public:
        const std::string& getMethod() const
        {
            return _method;
        };

        /**
         * -
         * TODO: json_spirit::Object for 2.0
         */
        const json_spirit::Array& getMethodParams() const
        {
            return _params;
        };

        boost::uint64_t getId() const
        {
            return _id;
        };
        
        JsonRequest(const std::string& data)
        {
            json_spirit::Object request;
            try{
                json_spirit::Value parsed;
                json_spirit::read_string_or_throw(data, parsed);
                request = parsed.get_obj(); // must be object
            }
            catch(...){
                throw JsonRequestException("incorrect type of request: " + data);
            }

            for(json_spirit::Object::size_type i = 0, n = request.size(); i < n; ++i){
                const json_spirit::Pair& pair   = request[i];
                const std::string& name         = pair.name_;
                const json_spirit::Value& value = pair.value_;

                if(name == "method"){
                    _method = value.get_str(); //TODO: protect
                    continue;
                }

                if(name == "params"){
                    _params = value.get_array(); //TODO: protect
                    continue;
                }

                if(name == "id"){
                    _id = (value.is_null())? 0 : value.get_uint64();
                    continue;
                }
            }
        };

    private:
        std::string _method;
        json_spirit::Array _params;
        boost::uint64_t _id;
    };

    typedef boost::recursive_mutex CriticalSection;
    typedef boost::lock_guard<boost::recursive_mutex> Lock;
    typedef boost::function<void(const JsonRequest&, JsonResponse&)> ServiceMethod;
    typedef std::map<std::string, ServiceMethod> ServiceMethods;

    class Server 
    {
    public:
        /**
         * instance of currently running server
         */
        HTTPServer* instance();

        void bindMethod(const std::string& name, ServiceMethod method);
        void unbindMethod(const std::string& name);
        /**
         * registered methods
         */
        static ServiceMethods services()
        {
            return _serviceMethods;
        };

        Server(unsigned short port = 1271):_port(port){ _run(); };
        ~Server(void){ _shutdown(); };

    private:
        bool _run();
        bool _shutdown();

        CriticalSection cs;
        unsigned short _port;
        HTTPServer* _server;
        static ServiceMethods _serviceMethods;
    };

}}};