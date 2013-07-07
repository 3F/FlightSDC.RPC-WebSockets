// entry.reg@gmail.com :: https://bitbucket.org/3F/flightsdc/
// draft

// version of the RPC-xhr: 2b5357ba70b9fe04f76e74877ee1a4cb15908193

#pragma once
#include "json_spirit.h"

#include "../../client/FavoriteManager.h"

namespace reg {
namespace p2p {
namespace rpc {
namespace Service {

class Hub
{
public:
    static bool create(const json_spirit::Object &data);
    static bool update(const json_spirit::Object &data);
    static bool remove(const json_spirit::Object &data);
    static bool connect(const json_spirit::Object &data);
    static bool close(const json_spirit::Object &data);
    static std::string used(const json_spirit::Object &data);
    static std::string list(const json_spirit::Object &data);
    
private:
    static void prepareHubFields(const json_spirit::Object &data, FavoriteHubEntry &entry);

    Hub(void){};
    ~Hub(void){};
};

}}}};