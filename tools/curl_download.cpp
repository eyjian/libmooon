// Writed by yijian on 2018/11/14
#include <libgen.h>
#include <mooon/sys/curl_wrapper.h>

static void usage(const char* argv0)
{
    char* s = strdup(argv0);
    fprintf(stderr, "Usage: %s <-k> local_filepath url\n", basename(s));
    free(s);
}

int main(int argc, char* argv[])
{
    bool enable_insecure = false;

    if (argc < 3)
    {
        usage(argv[0]);
        exit(1);
    }
    if (0 == strcmp(argv[1], "-k"))
    {
        enable_insecure = true;
    }
    if (enable_insecure && argc!=4)
    {
        usage(argv[0]);
        exit(1);
    }

    try
    {
        const std::string& local_filepath = enable_insecure? argv[2]: argv[1];
        const std::string& url = enable_insecure? argv[3]: argv[2];
        mooon::sys::CCurlWrapper curl;
        std::string response_header;
        curl.http_download(response_header, local_filepath, url, enable_insecure);

        return 0;
    }
    catch (mooon::sys::CSyscallException& ex)
    {
        fprintf(stderr, "%s\n", ex.str().c_str());
        exit(1);
    }
    catch (mooon::utils::CException& ex)
    {
        fprintf(stderr, "%s\n", ex.str().c_str());
        exit(1);
    }
}
