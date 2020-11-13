#include <librlxpkg/librlxpkg.hh>
#include <libapp/libapp.hh>
#include <unistd.h>
#include <libgen.h>
#include <math.h>

using namespace librlxpkg;
using namespace std;

void sync_recipe(libapp::ctl::obj& appctl, string url, vector<string> files, string loc) {
    int i = 0;

    for(auto file : files) {
        string file_url = url + "/" + file;
        string out_file = loc + "/" + file;

        string __dir = out_file;
        __dir = string(dirname((char*)__dir.c_str()));
        fs::make_dir(__dir);

        err::obj e = appctl.download_file(file_url, out_file, false);
        if (e.status() != 200) {
            io::warn("failed to sync ", out_file, " from ", file_url, " ", e.mesg());
        } else {
            io::print("\r completed: ", ((float)++i/(float) files.size()) * 100);
        }
    }
    io::print("\n");
}

map<string, string>
get_hash_file(string file)
{
    ifstream fptr(file);
    if (!fptr.good()) {
        io::error("failed to load meta file '", file, "'");
        return map<string, string>();
    }

    map<string, string> hash_data;
    string _hash, file_addr;
    string line;
    while (!fptr.eof()) {
        fptr >> _hash >> file_addr;
        hash_data.insert(make_pair(_hash, file_addr));
    }

    return hash_data;
}

vector<string>
get_outdated_files(libapp::ctl::obj & appctl, string url, string loc)
{
    string meta_file_url = url + "/rcp.meta";
    string meta_file_loc = "/tmp/.rcp.meta";
    auto e = appctl.download_file(meta_file_url, meta_file_loc);
    if (e.status() != 200) {
        io::error("failed to download meta data from ", meta_file_url);
        return vector<string>();
    }

    auto hash_data = get_hash_file(meta_file_loc);
    vector<string> to_update;
    for(auto a : hash_data) {
        string file_loc = loc + "/" + a.second;
        string _hash = libapp::hash(file_loc);
        if (a.first != _hash) {
            to_update.push_back(a.second);
        }
    }

    return to_update;
}

err::obj
obj::Sync(conf::obj& conf, bool debug)
{
    auto rcp_dir = conf.get("dir","recipes",RECIPES_DIR);

    libapp::ctl::obj appctl(conf.filename);
    for (auto sec : conf.sections) {
        string sec_id = sec.first;
        if (sec_id == "url.src") {
            for(auto data : sec.second) {
                string repo_id = data.first;
                string repo_url = data.second;

                io::process("syncing ",repo_id);
                auto to_update = get_outdated_files(appctl, repo_url, rcp_dir);
                if (to_update.size()) {
                    io::info(to_update.size(), " new recipes found");
                    sync_recipe(appctl, repo_url, to_update, rcp_dir);
                }
                
            }
        }
    }

    return err::obj(0);
}