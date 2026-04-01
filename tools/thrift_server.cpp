#include "TestService.h"
#include <mooon/net/thrift_helper.h>
#include <mooon/net/utils.h>
#include <mooon/sys/utils.h>
#include <signal.h>
namespace test {

class CTestHandler: public TestServiceIf
{
    virtual void hello(std::string& _return)
    {
        _return = "Hello, world";
        fprintf(stdout, "%s\n", _return.c_str());
    }
};

struct ThriftServerContext
{
    std::string peer;
    std::string self;
};

class MyServerEventHandler: public apache::thrift::server::TServerEventHandler
{
private:
    virtual void* createContext(
            std::shared_ptr<apache::thrift::server::TProtocol> input,
            std::shared_ptr<apache::thrift::server::TProtocol> output);
    virtual void deleteContext(
            void* serverContext,
            std::shared_ptr<apache::thrift::server::TProtocol>input,
            std::shared_ptr<apache::thrift::server::TProtocol>output);
    virtual void processContext(void* serverContext, std::shared_ptr<apache::thrift::server::TTransport> transport);
};

class MyProcessorEventHandler: public apache::thrift::TProcessorEventHandler
{
private:
    virtual void* getContext(const char* fn_name, void* serverContext);
};

static std::shared_ptr<MyServerEventHandler> _server_event_handler(new MyServerEventHandler);
static std::shared_ptr<MyProcessorEventHandler> _processor_event_handler(new MyProcessorEventHandler);
static mooon::net::CThriftServerHelper<CTestHandler, TestServiceProcessor> thrift_server(_server_event_handler, _processor_event_handler);

static void sighandler(int signo)
{
    if (signo == SIGTERM)
        thrift_server.stop();
}

extern "C" int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        const std::string myname = mooon::sys::CUtils::get_program_short_name();
        fprintf(stderr, "Usage: %s port\n", myname.c_str());
        fprintf(stderr, "Example: %s 2020\n", myname.c_str());
        return 1;
    }
    try
    {
        signal(SIGPIPE, SIG_IGN);
        signal(SIGTERM, sighandler);

        uint16_t port = mooon::utils::CStringUtils::string2int<uint16_t>(argv[1]);
        thrift_server.serve("0.0.0.0", port, 1, 1);
        return 0;
    }
    catch (apache::thrift::TException& ex)
    {
        fprintf(stderr, "%s\n", ex.what());
        return 2;
    }
}

//
// MyServerEventHandler
//

void* MyServerEventHandler::createContext(
        std::shared_ptr<apache::thrift::server::TProtocol> input,
        std::shared_ptr<apache::thrift::server::TProtocol> output)
{
    // 以下针对TNonblockingServer
    // in_transport和out_transport实际为apache::thrift::server::TMemoryBuffer
    //std::shared_ptr<apache::thrift::server::TTransport> in_transport = input->getTransport();
    //std::shared_ptr<apache::thrift::server::TTransport> output_transport = output->getTransport();
    return new ThriftServerContext;
}

void MyServerEventHandler::deleteContext(
        void* serverContext,
        std::shared_ptr<apache::thrift::server::TProtocol>input,
        std::shared_ptr<apache::thrift::server::TProtocol>output)
{
    delete (ThriftServerContext*)serverContext;
}

void MyServerEventHandler::processContext(void* serverContext, std::shared_ptr<apache::thrift::server::TTransport> transport)
{
    ThriftServerContext* ctx = (ThriftServerContext*)serverContext;

    if (ctx->peer.empty())
    {
        // 以下针对TNonblockingServer有效
        apache::thrift::server::TSocket* socket = dynamic_cast<apache::thrift::server::TSocket*>(transport.get());

        if (socket != NULL)
        {
            const int sockfd = socket->getSocketFD();

            // TSocket::getPeerAddress返回的是IP地址，
            // 如果调用TSocket::getPeerHost()，则返回的可能是IP对应的hostname
            fprintf(stdout, "Called from %s:%d\n", socket->getPeerAddress().c_str(), socket->getPeerPort());
            ctx->peer = mooon::utils::CStringUtils::format_string("%s:%d", socket->getPeerAddress().c_str(), socket->getPeerPort());
            ctx->self = mooon::net::get_self(sockfd);
        }
    }
}

//
// MyProcessorEventHandler
//

void* MyProcessorEventHandler::getContext(const char* fn_name, void* serverContext)
{
    ThriftServerContext* ctx = (ThriftServerContext*)serverContext;
    fprintf(stdout, "[IFCALLED][%s] %s called by %s\n", ctx->self.c_str(), fn_name, ctx->peer.c_str());
    return NULL;
}

} // namespace test {
