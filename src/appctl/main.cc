#include <libapp/libapp.hh>
#include <default.hh>

#define DETAIL(x,z) io::colored_title(color::cyan, (x), (z))

int hash_func(cli::data_t& data)
{
    if (data.args.size() == 0) {
        io::error("input file");
        return -2;
    }
    std::string _file = data.args.at(0);
    io::fprint(std::cout, libapp::hash(_file));
    return 0;
}
int sync_func(cli::data_t& data)
{
    libapp::ctl::obj appctl(data.value_of("config",CONFIG_FILE));
    appctl.sync_modules(data.is_flag_set("debug"));
    return 0;
}

int download_func(cli::data_t& data)
{
    if (data.args.size() < 2)
    {
        io::error("need arguments");
        return 1;
    }
    libapp::ctl::obj appctl(data.value_of("config",CONFIG_FILE));
    auto e = appctl.download_file(data.args[0], data.args[1], data.is_flag_set("show-progress"));
    if (e.status() != 200) {
        io::error(e.mesg());
        return -1;
    }
    return 0;
}

int
remove_func(cli::data_t& data)
{
    if (data.args.size() == 0) {
        io::error("specify app name");
        return 1;
    }
    std::string app_name = data.args[0];
    libapp::ctl::obj appctl(data.value_of("config",CONFIG_FILE));
    auto e = appctl.Remove(app_name, data.is_flag_set("debug"));
    if (e.status() != 0) {
        io::error(e.mesg(), " (",e.status(), ")");
    }
    return e.status();
}
int
cal_dep_func(cli::data_t& data)
{
    if (data.args.size() == 0) {
        io::error("specify app name");
        return 1;
    }
    std::string app_name = data.args[0];
    libapp::ctl::obj appctl(data.value_of("config",CONFIG_FILE));

    auto app = appctl.get_app(app_name, data.is_flag_set("debug"));
    try {
        auto deplist = appctl.cal_dep(app, data.is_flag_set("debug"));
        for(auto a : deplist) {
            io::print(a->name(), "\n");
        }
    } catch(err::obj e) {
        io::error(e.mesg());
    }

    return 0;
}

int
list_func(cli::data_t& data)
{
    if (data.args.size() == 0) {
        io::error("specify app name");
        return 1;
    }
    std::string app_name = data.args[0];
    libapp::ctl::obj appctl(data.value_of("config",CONFIG_FILE));

    auto app_data = appctl.is_installed(app_name, data.is_flag_set("debug"));
    if (!app_data.installed) {
        io::error(app_name, " is not installed");
        return 2;
    }

    auto files_list = appctl.list_files(app_data, data.is_flag_set("debug"));
    if (files_list.size() == 0) {
        io::error("no files is installed in system ",app_name);
        return 0;
    }

    for(auto a : files_list) {
        io::print(a,"\n");
    }

    return 0;
}

int
info_func(cli::data_t& data)
{
    if (data.args.size() == 0) {
        io::error("specify app name");
        return 1;
    }
    std::string app_name = data.args[0];
    libapp::ctl::obj appctl(data.value_of("config",CONFIG_FILE));

    auto app_data = appctl.is_installed(app_name, data.is_flag_set("debug"));

    if (app_data.installed) {
        DETAIL("Name         ",app_data.name);
        DETAIL("Version      ",app_data.version);
        DETAIL("Release      ",app_data.release);
        DETAIL("Description  ",app_data.description);
        DETAIL("Installed    ", color::gencode(color::green)+"Yes");
        DETAIL("Size         ",app_data.size);
        DETAIL("Installed on ",app_data.installed_time);
    } else {
        auto app = appctl.get_app(app_name, data.is_flag_set("debug"));
        if (app == nullptr) {
            io::error("no app with name ",app_name, " in database");
            return -1;
        }

        DETAIL("Name         ",app->name());
        DETAIL("Version      ",app->ver());
        DETAIL("Release      ",app->rel());
        DETAIL("Description  ",app->desc());
        DETAIL("Installed    ",color::gencode(color::red)+"No");
        DETAIL("Found in     ",app->type()+"s");
    }

    return 0;
}

int
install_func(cli::data_t& data)
{
    if (data.args.size() == 0) {
        io::error("specify app name");
        return 1;
    }

    libapp::ctl::obj appctl(data.value_of("config",CONFIG_FILE));

    appctl.reinstall = data.is_flag_set("reinstall");
    appctl.redownload = data.is_flag_set("redownload");
    appctl.update = data.is_flag_set("update");
    appctl.repack = data.is_flag_set("repack");

    appctl.skip_dep = data.is_flag_set("skip-dep");
    appctl.skip_post = data.is_flag_set("skip-post");
    appctl.skip_pre = data.is_flag_set("skip-pre");

    std::string app_name = data.args[0];
    auto e = appctl.Install(app_name,data.is_flag_set("debug"));
    if (e.status() != 0) {
        io::error(e.mesg());
    }
    return e.status();
}

int
verify_config_func(cli::data_t& data)
{
    conf::obj config(data.value_of("config",CONFIG_FILE));

    if (data.args.size() == 0) {
        for(auto s : config.sections) {
            for(auto v : s.second) {
                io::print(s.first,".",v.first," = ",v.second,"\n");
            }
        }
    } else {
        auto val = data.args[0];
        int rdx = val.rfind('.');
        if (rdx == std::string::npos) {
            io::error("invalid format, use section.variable");
            return 1;
        }

        auto sec = val.substr(0, rdx);
        auto var = val.substr(rdx + 1, val.length() - (rdx +1));

        if(data.is_flag_set("debug")) {
            io::info("found var = ", var, " sec = ",sec,"\n");
        }
        io::print(config.get(sec,var,"unknown"),"\n");
    }
    return 0;
}

int main(int ac, char** av) {
    cli::obj app(ac, av);

    app.id("appctl")
       .ver("0.1.1")
       .about("an advance and extensible package manager for releax os")
       .sub(cli::sub_t{
           .id = "install",
           .desc = "install specified application from recipe, name, compressed package",
           .usage = "app-name | .rlx file | recipe-loc",
           .func = install_func
       })
       
       .sub(cli::sub_t{
           .id = "remove",
           .desc = "remove app from root directory",
           .usage = "app-name",
           .func = remove_func
       })

       .sub(cli::sub_t{
           .id = "info",
           .desc = "print information of <app>",
           .usage = "app-name",
           .func = info_func
       })

       .sub(cli::sub_t{
           .id = "sync",
           .desc = "sync data from modules",
           .usage = "",
           .func = sync_func
       })

       .sub(cli::sub_t{
           .id = "_lsfiles",
           .desc = "list content of install <app>",
           .usage = "",
           .func = list_func
       })

       .sub(cli::sub_t{
           .id = "depends",
           .desc = "calculate required dependencies of <app>",
           .usage = "app-name",
           .func = cal_dep_func
       })
       .sub(cli::sub_t{
           .id = "_vrfyconf",
           .desc = "print configuration file",
           .usage = "section.variable",
           .func = verify_config_func
       })
       .sub(cli::sub_t{
           .id = "_dwnld",
           .desc = "download file specified",
           .usage = "<url> <file>",
           .func = download_func
       })
       
       .sub(cli::sub_t{
           .id = "_genhash",
           .desc = "generate hash sum of input file",
           .usage = "file",
           .func = hash_func
       });

    try {
        return app.run().status();
    } catch(err::obj e) {
        io::error(e.mesg(), "(",e.status(),")");
    }
    return -1;
}