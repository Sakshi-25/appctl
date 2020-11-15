#include <libapp/libapp.hh>
#include <dlfcn.h>

using namespace libapp;

std::vector<std::string>
ctl::obj::list_files(app_db_t& app_data, bool debug)
{
    std::vector<std::string> files_list;
    if (!app_data.installed) return files_list;

    auto files_f = config.get("dir","data",DATA_DIR) + app_data.name + "/files";

    std::ifstream fptr(files_f);
    if (!fptr.good()) {
        DEBUG("failed to load ",files_f);
        return files_list;
    }
    std::string line;
    while(std::getline(fptr,line)) {
        if (line.substr(0,6) == ".data/") continue;
        files_list.push_back(line);
    }

    return files_list;
}

app_db_t
ctl::obj::is_installed(const std::string& app_name, bool debug)
{
    auto data_dir = config.get("dir","data",DATA_DIR);
    DEBUG("checking in ",data_dir);

    auto app_data_dir = data_dir+"/"+app_name;
    if (!fs::is_exist(app_data_dir+"/info")) {
        DEBUG(data_dir+"/"+app_name+"/info file not found");
        return app_db_t{};
    }

    DEBUG("found ",app_data_dir);
    app_db_t app_data;

    app_data.installed = true;

    std::ifstream fptr(app_data_dir+"/info");
    if (!fptr.good()) {
        DEBUG("failed to open ",app_data_dir +"/info");
        app_data.installed = false;
        return app_data;
    }

    std::string line;
    std::string depends = "";

    while(std::getline(fptr, line)) {
        if (line.size() == 0) continue;
        size_t rdx = line.find_first_of(':');
        if (rdx == std::string::npos) continue;

        auto var = line.substr(0, rdx);
        auto val = line.substr(rdx + 2 , line.length() - (rdx + 2));

        if (var == "name") app_data.name = val;
        else if (var == "version") app_data.version = val;
        else if (var == "release") app_data.release = val;
        else if (var == "description" ) app_data.description = val;
        else if (var == "depends") depends = val;
        else if (var == "size") app_data.size = val;
        else if (var == "build") app_data.build_time = val;
        else if (var == "installed") app_data.installed_time = val;
    }

    std::stringstream ss(depends);
    std::string l;

    while( ss >> l) {
        app_data.depends.push_back(l);
    }

    fptr.close();

    return app_data;
}

void
ctl::obj::load_modules()
{
    bool found = false;
    for(auto a : config.sections) {
        if (a.first == "modules") {
            for(auto m : a.second) {
                load_modules(m.first, m.second);
            }
            found = true;
            break;
        }
    }
    if (!found) {
        load_modules("recipe",MODULES_RECIPE);
    }
}

void
ctl::obj::load_modules(std::string id, std::string path)
{
    void *handler = dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    
    if (handler == nullptr) {
        io::error("failed to load module ", id, " from ", path);
        io::error(dlerror());
        return;
    }
    module_t mod = (module_t) dlsym(handler, "module");
    if (mod == nullptr) {
        io::error("invalid module ", id, " from ", path);
        io::error(dlerror());
        return;
    }

    modules[id] = mod;

}


err::obj
ctl::obj::lock_appctl(string id)
{
    string cache_dir = config.get("dir","cache",CACHE_DIR);
    string lock_file = cache_dir + "/lock";

    if (fs::is_exist(lock_file)) {
        ifstream fptr(lock_file);
        string _lock_id;
        fptr >> _lock_id;
        fptr.close();

        return err::obj(err::already_exist, "appctl database is locked by " + _lock_id);
    }

    if (debug) io::log("locking appctl");
    try {
        if (!fs::is_exist(cache_dir)) fs::make_dir(cache_dir);
        // locking database
        fs::write(lock_file, id);
    } catch(err::obj e) {
            return e;
    }
    

    string work_dir = config.get("dir","work", io::sprint(cache_dir,"/work"));
    string pkg_dir  = config.get("dir", "pkg", io::sprint(cache_dir,"/pkg"));
    string src_dir  = config.get("dir", "src", io::sprint(cache_dir,"/src"));


    return err::obj(0);
}

err::obj
ctl::obj::unlock_appctl(string id)
{
    string cache_dir = config.get("dir", "cache", CACHE_DIR);
    string lock_file = cache_dir + "/lock";

    if (!fs::is_exist(lock_file)) {
        return err::obj(err::file_missing, "appctl is not locked");
    }

    ifstream fptr(lock_file);
    string _lock_id;
    fptr >> _lock_id;
    fptr.close();

    if (id != _lock_id) {
        return err::obj(err::invalid_permission, "appctl is locked by " + _lock_id);
    }

    remove(lock_file.c_str());

    return err::obj(0);
}