#include <librlxpkg/librlxpkg.hh>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <curl/curl.h>

using namespace libapp;

std::string
libapp::hash(const std::string& fname)
{
    std::ifstream file(fname);
    std::string it((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());

    unsigned mask;
    unsigned crc = 0xFFFFFFFF;

    for (auto byte : it) {
        crc ^= byte;
        for (int j = 0; j < 8; ++j) {
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }

    return std::to_string(~crc);
}

size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* fptr)
{
    return fwrite(ptr, size, nmemb, fptr);
}

err::obj
ctl::obj::download_file(const std::string& url, const std::string& file, bool progress, bool debug)
{
    CURL* curl;
    FILE* fp;
    CURLcode resp;
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(file.c_str(), "wb");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        if (progress) {
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        }
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        resp = curl_easy_perform(curl);
        fclose(fp);
        long respcode;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &respcode);
        if (debug) io::info("response code: ",respcode);
        if (resp != CURLE_OK) {
            curl_easy_cleanup(curl);
            return err::obj(respcode, " failed to download file from url " + url);
        } else {
            curl_easy_cleanup(curl);
            return err::obj(respcode);
        }
        curl_easy_cleanup(curl);

    }
    return err::obj(0x12, "failed to init curl, download failed");
}

err::obj
ctl::obj::sync_modules(bool debug)
{
    for(auto a : modules)
    {
        io::process("checking updates for ",a.first);
        auto s = a.second(config);
        auto e = s->Sync(config, debug);
        if (e.status() != 0) {
            io::error("Status Code: ",e.status(), " Message: ", e.mesg());
        }
        delete s;
    }
    return err::obj(0);
}