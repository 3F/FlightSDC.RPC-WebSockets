#pragma once

namespace reg {
namespace p2p {
namespace rpc {

    class JsonRequestException : public std::exception
    {
    public:
        JsonRequestException(const int code, const std::string message):_code(code), _message(message)
        {

        };

        JsonRequestException(const std::string message):_message(message)
        {

        };

        int getCode() const
        {
            return _code;
        };

        const std::string& getMessage() const
        {
            return _message;
        };

        ~JsonRequestException(void){};
 
    private:
        std::string _message;
        int _code;
    };

}}};