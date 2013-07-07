// entry.reg@gmail.com :: https://bitbucket.org/3F/flightsdc/
// draft

// version of the RPC-xhr: 2b5357ba70b9fe04f76e74877ee1a4cb15908193

#pragma once
#include "../Rpc.hpp"

namespace reg {
namespace p2p {
namespace rpc {

namespace ServicesTypes
{
    namespace ErrorCodes
    {
        enum ServiceErrorCodes
        {
            ERR_PARAM_TYPE_INCORRECT        = 100,
            ERR_PARAM_COUNT_DIFFERENT       = 101,
            ERR_PARAM_UNKNOWN_ARG           = 102,
            ERR_PARAM_KEYARG_NOTFOUND       = 103,
            ERR_OPERATION_TYPE_INCORRECT    = 200,
            ERR_UNKNOWN_ERROR               = 500,
            ERR_UNSUPPORTED_FUNCTION        = 700
        };
    };

    namespace ServiceSearch
    {
        enum TypeAllow
        {
            /* listen answer */
            RESPONSE,
            /*  simple query */
            DEFAULT,
            /* request by tth */
            TTH,
            /* commands after start */
            COMMAND
            /* additional features: ADC - SCH {AN, NO, EX}, separation of words & etc., */
            //SPECIFIC
        };

        namespace LabelFile
        {
            enum
            {
                /* default no label */
                WITHOUT_LABEL,
                WAITING_DOWNLOAD,
                DOWNLOADED_PART,
                DOWNLOADED_COMPLETE,
                ALREADY_IN_SHARE,
                /* manually of user */
                EXCLUDED
            };
        };

        namespace LabelUser
        {
            enum{};
        };
    };

    namespace ServiceHub
    {
        enum TypeAllow
        {
            /* list fav */
            LIST,
            /* current links */
            USED,
            /* quick / fav. */
            CONNECT,
            CLOSE,
            CREATE, UPDATE, REMOVE,
            /* getting custom menu from the connected hub */
            MENU
        };
    };
};


/**
 * Удволетворение основных потребностей.
 */
class Services
{
public:
  /* display of current transfers -> similar to the queue */
    void transfers(const JsonRequest& request,  JsonResponse& response);

  /* hub operation: CRUD of fav & list of connected with main operation */
    void hub(const JsonRequest& request,  JsonResponse& response);

  /* search operation */
    void search(const JsonRequest& request,  JsonResponse& response);

  /* queue operation: Uploads/Downloads, Complete/incomplete, CRUD op,  */
    void queue(const JsonRequest& request,  JsonResponse& response);
  
  /* viewing from users and sharing operation */
    void share(const JsonRequest& request,  JsonResponse& response);

  /* available settings: speed limit, connection settings, etc., */
    void settings(const JsonRequest& request,  JsonResponse& response);

  /* loader applications and libraries */
    void execute(const JsonRequest& request,  JsonResponse& response);

  /* hashing operation: calculate, status */
    void hashing(const JsonRequest& request,  JsonResponse& response);

    Services(void){};
    ~Services(void){};

private:
    void prepareSuccess(const std::string& result, JsonResponse& response);
    void prepareFailure(int error, JsonResponse& response);
    void handlerBooleanResult(bool result, JsonResponse& response);
    void handlerStringResult(const std::string& result, JsonResponse& response);
};

}}};