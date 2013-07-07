// entry.reg@gmail.com :: https://bitbucket.org/3F/flightsdc/
// draft

// version of the RPC-xhr: 2b5357ba70b9fe04f76e74877ee1a4cb15908193

#include "stdafx.h"
#include "RpcServiceSearch.h"
#include "RpcServices.h"
#include "../../windows/SearchFrm.h"
#include "../../windows/MainFrm.h" //onSpeaker., TODO:~

namespace reg {
namespace p2p {
namespace rpc { using namespace ServicesTypes::ServiceSearch;
namespace Service {

std::string Search::result(int count)
{
    if(SearchFrame::searchResults.empty()){
        return string();
    }

    std::string ret;
    for(int i = 0; !SearchFrame::searchResults.empty() && i < count; ++i){
        const SearchResultPtr &res = SearchFrame::searchResults.back();
        SearchFrame::searchResults.pop_back();

        ret += "[\"" + safeString(res->getFile()) 
            + "\",\"" + Util::toString(res->getSize()) 
            + "\",\"" + (res->getType() == SearchResult::TYPE_FILE ? res->getTTH().toBase32() : "")
            + "\",\"" + safeString(res->getUser()->getFirstNick()) 
            + "\",\"" + res->getSlotString() 
            + "\",\"" + safeString(res->getHubName()) 
            + "\","   + Util::toString(LabelFile::WITHOUT_LABEL) +"],"; //TODO: Пометки к файлу
    }
    if(ret.length() > 0){
        //return "[" + ret.substr(0, ret.length() - 1) + "]";
        return ret.substr(0, ret.length() - 1);
    }
    return string();
}

bool Search::simpleSearch(const std::string &query, bool isHash)
{
    //StringList clients, extList;
    //SearchManager::getInstance()->search(clients, query, 0, SearchManager::TypeModes::TYPE_ANY, SearchManager::SizeModes::SIZE_DONTCARE, Util::toString(Util::rand()), extList, NULL);
    MainFrame::getMainFrame()->PostMessage(WM_SPEAKER, MainFrame::SEARCH_SIMPLE, (LPARAM)new std::string(query));
    return true;
}

bool Search::command(const json_spirit::Object &data)
{
    return false;
}

inline std::string Search::safeString(std::string str)
{
    Util::replace("\\", "\\\\", str);
    //Util::replace("'", "\\'", str); // ['...']
    //TODO: т.к. RCF имеет реализацию ответа только с json_spirit::mObject, то для wmObject необходимо очень много чего переопределять
    //поэтому пока так, но вообще следует озаботиться другой lib либо реализовать свой
    return Text::fromUtf8(str, Text::g_code1251);
}

}}}};