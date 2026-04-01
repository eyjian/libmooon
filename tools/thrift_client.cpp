#include "TestService.h"
#include <mooon/net/thrift_helper.h>
#include <mooon/sys/utils.h>
namespace test {

extern "C" int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        const std::string myname = mooon::sys::CUtils::get_program_short_name();
        fprintf(stderr, "Usage: %s ip port\n", myname.c_str());
        fprintf(stderr, "Example: %s 127.0.0.1 2020\n", myname.c_str());
        return 1;
    }
    try
    {
        const std::string ip = argv[1];
        uint16_t port = mooon::utils::CStringUtils::string2int<uint16_t>(argv[2]);
        mooon::net::CThriftClientHelper<TestServiceClient> client(ip, port);
        std::string str;
        client.connect();
        client->hello(str);
        fprintf(stdout, "%s\n", str.c_str());
        return 0;
    }
    catch (apache::thrift::TException& ex)
    {
        fprintf(stderr, "%s\n", ex.what());
        return 2;
    }
}

} // namespace test {
