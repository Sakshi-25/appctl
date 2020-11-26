#include <libapp/libapp.hh>


using namespace libapp;

err::obj
ctl::obj::Install(const std::string & app, bool debug)
{
    try {
        auto app_ptr = get_app(app, debug);
        if (app_ptr == nullptr) return err::obj(err::file_missing, "failed to get " +app);

        if (config.get("local","reinstall","0") == "0")
        {
            auto app_data = is_installed(app, debug);
            if (app_data.installed) return err::obj(112, app + " is already installed");
        }
        

        auto deps = cal_dep(app_ptr, debug);
        io::process("checking dependencies");
        
        err::obj e(0);
        if (deps.size() > 1) {
            io::print("require dependencies [");
            for(auto a : deps) io::print(" ",a->name());
            io::print(" ]\n");
        }

        if (deps.size() > 1 && !skip_dep) {
            
            
            for(auto a = deps.begin(); a != deps.end() - 1; a++) {
                auto _a = *a;
                io::process("installing dependency ",_a->name());
                e = _a->Install(config, debug);
                if (e.status() != 0) {
                    io::error("failed to install ",_a->name());
                    io::error(e.mesg(), " (",e.status(),")");
                    return e;
                }
            }

        } 
        
        return app_ptr->Install(config, debug);
        
    } catch (err::obj e) {
        switch (e.status()) {
            case err::file_missing: 
                io::error("[FileMissing] ",e.mesg());
                break;

            case err::execution:
                io::error("[ExecutionError] ", e.mesg());
                break;

            default:
                io::error("[UnknownError] ",e.mesg());
                break;
        }
        return e;
    }
    return err::obj(0);
}

