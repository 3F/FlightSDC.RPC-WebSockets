// entry.reg@gmail.com :: https://bitbucket.org/3F/flightsdc/
// draft

// version of the RPC-xhr: 2b5357ba70b9fe04f76e74877ee1a4cb15908193

#include "stdafx.h"
#include "RpcServices.h"

#include "RpcServiceHub.h"
#include "RpcServiceSearch.h"

namespace reg {
namespace p2p {
namespace rpc {

void Services::transfers(const JsonRequest& request,  JsonResponse& response)
{

}

/**
 * -
 * Request params: array("type", {object})
 */
void Services::hub(const JsonRequest& request,  JsonResponse& response)
{
    prepareFailure(ServicesTypes::ErrorCodes::ERR_UNKNOWN_ERROR, response);
    const json_spirit::Array &params = request.getMethodParams();    
    
    if(params.size() != 2){
        //throw Exception("..."); - catch in RcfServer:938
        prepareFailure(ServicesTypes::ErrorCodes::ERR_PARAM_COUNT_DIFFERENT, response);
        return;
    }

    if(params[0].type() != json_spirit::int_type || params[1].type() != json_spirit::obj_type){
        prepareFailure(ServicesTypes::ErrorCodes::ERR_PARAM_TYPE_INCORRECT, response);
        return;
    }

    const json_spirit::Object &data = params[1].get_obj();
    
    switch(params[0].get_int())
    {
        case ServicesTypes::ServiceHub::USED:
        {
            handlerStringResult(Service::Hub::used(data), response);
            return;
        }
        case ServicesTypes::ServiceHub::CONNECT:
        {
            handlerBooleanResult(Service::Hub::connect(data), response);
            return;
        }
        case ServicesTypes::ServiceHub::CLOSE:
        {
            handlerBooleanResult(Service::Hub::close(data), response);
            return;
        }
        case ServicesTypes::ServiceHub::LIST:
        {
            handlerStringResult(Service::Hub::list(data), response);
            return;
        }
        case ServicesTypes::ServiceHub::CREATE:
        {
            handlerBooleanResult(Service::Hub::create(data), response);
            return;
        }
        case ServicesTypes::ServiceHub::UPDATE:
        {
            handlerBooleanResult(Service::Hub::update(data), response);
            return;
        }
        case ServicesTypes::ServiceHub::REMOVE:
        {
            handlerBooleanResult(Service::Hub::remove(data), response);
            return;
        }
        case ServicesTypes::ServiceHub::MENU:
        {
            prepareFailure(ServicesTypes::ErrorCodes::ERR_UNSUPPORTED_FUNCTION, response); //TODO:
            return;
        }
    }
    prepareFailure(ServicesTypes::ErrorCodes::ERR_OPERATION_TYPE_INCORRECT, response);
}

/**
 * -
 * Request params by type: 
 *       - {object} array(RESPONSE [, count])
 *       - bool     array(DEFAULT, "query string")
 *       - bool     array(TTH, "base32-encoded string")
 *       - bool     array(COMMAND, {object})
 */
void Services::search(const JsonRequest& request,  JsonResponse& response)
{
    prepareFailure(ServicesTypes::ErrorCodes::ERR_UNKNOWN_ERROR, response);
    const json_spirit::Array &params = request.getMethodParams();

    if(params[0].type() != json_spirit::int_type){
        prepareFailure(ServicesTypes::ErrorCodes::ERR_PARAM_TYPE_INCORRECT, response);
        return;
    }

    const int type = params[0].get_int();

    switch(type)
    {
        case ServicesTypes::ServiceSearch::RESPONSE:
        {
            if(params.size() == 2){
                if(params[1].type() == json_spirit::int_type){
                    handlerStringResult(Service::Search::result(params[1].get_int()), response);
                    return;
                }
                prepareFailure(ServicesTypes::ErrorCodes::ERR_PARAM_TYPE_INCORRECT, response);
                return;
            }
            handlerStringResult(Service::Search::result(), response);
            return;
        }
        case ServicesTypes::ServiceSearch::DEFAULT: case ServicesTypes::ServiceSearch::TTH:
        {
            if(params[1].type() != json_spirit::str_type){
                prepareFailure(ServicesTypes::ErrorCodes::ERR_PARAM_TYPE_INCORRECT, response);
                return;
            }
            handlerBooleanResult(Service::Search::simpleSearch(params[1].get_str(), (type == ServicesTypes::ServiceSearch::TTH)? true : false), response);
            return;
        }
        case ServicesTypes::ServiceSearch::COMMAND:
        {
            if(params[1].type() != json_spirit::obj_type){
                prepareFailure(ServicesTypes::ErrorCodes::ERR_PARAM_TYPE_INCORRECT, response);
                return;
            }
            handlerBooleanResult(Service::Search::command(params[1].get_obj()), response);
            return;
        }
    }
    prepareFailure(ServicesTypes::ErrorCodes::ERR_OPERATION_TYPE_INCORRECT, response);
}

void Services::queue(const JsonRequest& request,  JsonResponse& response)
{

}

void Services::share(const JsonRequest& request,  JsonResponse& response)
{

}

void Services::settings(const JsonRequest& request,  JsonResponse& response)
{

}

void Services::execute(const JsonRequest& request,  JsonResponse& response)
{

}

void Services::hashing(const JsonRequest& request,  JsonResponse& response)
{
    
}

inline void Services::prepareSuccess(const std::string &result, JsonResponse& response)
{
    json_spirit::mObject &ret = response.getResponse();
    ret["error"]  = json_spirit::mValue();
    if(!result.empty()){
        ret["result"] = result;
    }
    else{
        ret["result"] = json_spirit::mValue();
    }
}

inline void Services::prepareFailure(int error, JsonResponse& response)
{
    json_spirit::mObject &ret = response.getResponse();
    ret["error"]  = error;
    ret["result"] = json_spirit::mValue();
}

inline void Services::handlerBooleanResult(bool result, JsonResponse& response)
{
    if(!result){
        prepareSuccess("false", response);
        return;
    }
    prepareSuccess("true", response);
    return;
}

inline void Services::handlerStringResult(const std::string &result, JsonResponse& response)
{
    prepareSuccess(result, response);
}

}}};