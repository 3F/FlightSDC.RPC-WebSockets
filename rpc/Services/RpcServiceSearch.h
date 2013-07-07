// entry.reg@gmail.com :: https://bitbucket.org/3F/flightsdc/
// draft

// version of the RPC-xhr: 2b5357ba70b9fe04f76e74877ee1a4cb15908193

#pragma once
#include "json_spirit.h"

namespace reg {
namespace p2p {
namespace rpc {
namespace Service {

class Search
{
public:
    static std::string result(int count = 500);
    static bool simpleSearch(const std::string &query, bool isHash);
    static bool command(const json_spirit::Object &data);

private:
    /**
     * Экрнирование и устранение нежелательных символов.
     */
    static std::string safeString(std::string str);

    Search(void){};
    ~Search(void){};
};

}}}};

