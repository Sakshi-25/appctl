#include <librlxpkg/librlxpkg.hh>
#include <fstream>
#include <unistd.h>
#include <cstring>
#include <rlx/io.hh>
#include <libgen.h>

using namespace librlxpkg;

std::string
obj::get_recipe_path(const std::string& a)
{
    std::string rcppath = config.get("dir","recipes",RECIPES_DIR);
    std::stringstream ss(config.get("default","repo","core"));
    std::string repo;
    while (ss >> repo) {
        std::string rcp_ = rcppath + "/" + repo + "/" + a + "/recipe";
        if (fs::is_exist(rcp_)) {
            return rcp_;
        }
    }

    return "";
}
bool
obj::__can_handle(const std::string& app_id)
{
    if (fs::is_exist(app_id) && !(std::strcmp(basename(app_id.c_str()),"recipe"))) {
        return true;
    }

    return (fs::is_exist(get_recipe_path(app_id)));
}

obj::obj(conf::obj __c)
: config(__c)
{

}

void
obj::load(const std::string& __r)
{
    __rcp_file = __r;
    if (!fs::is_exist(__rcp_file)) {
        auto rcp_path = get_recipe_path(__rcp_file);
        if (fs::is_exist(rcp_path)) {
            __rcp_file = rcp_path;
        }
    } 
    std::ifstream fptr(__rcp_file);
    if (!fptr.good()) {
        throw err::obj(err::file_missing, __rcp_file);
    }

    std::string __line;
    std::string __var, __val;
    while (std::getline(fptr, __line)) {
        if (__line.size() == 0) continue;
        if (__line[0] == '#') {
            uint64_t rdx = __line.find_first_of(':');
            if (rdx == std::string::npos) continue;
            __var = __line.substr(2, rdx - 2);
            __val = __line.substr((rdx + 2), __line.size() - (rdx + 2));
        } else {
            uint64_t rdx = __line.find_first_of('=');
            if (rdx == std::string::npos) continue;

            __var = __line.substr(0, rdx);
            __val = __line.substr(rdx + 1, __line.size() - (rdx + 1));
        }

        if (__var == "name") __name = __val;
        else if (__var == "version") __ver = __val;
        else if (__var == "release") __rel = atoi(__val.c_str()); 
        else if (__var == "Description") __desc = __val;
        else if (__var == "Depends on") {
            std::stringstream ss(__val);
            std::string l;
            while(ss >> l) {
                __depends.push_back(l);
            }
        }
    }
}

extern "C" obj* module(conf::obj __c)
{
    return new obj(__c);
}
