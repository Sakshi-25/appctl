#include <libapp/libapp.hh>


using namespace libapp;

err::obj
ctl::obj::Install(const std::string & app, bool debug)
{
    try {
        auto app_ptr = get_app(app, debug);
        if (app_ptr == nullptr) return err::obj(err::file_missing, "failed to get " +app);

        auto app_data = is_installed(app, debug);
        if (app_data.installed) return err::obj(112, app + " is already installed");

        if (!skip_dep) {
            auto deps = cal_dep(app_ptr, debug);
            err::obj e(0);
            if (deps.size() > 0) {
                io::print("install dependencies [");
                for(auto a : deps) io::print(" ",a->name());
                io::print(" ]\n");
            }

            for(auto a : deps) {
                io::process("install dependency ",a->name());
                e = a->Install(config, debug);
                if (e.status() != 0) return e;
            }
            return e;
        } else {
            return app_ptr->Install(config, debug);
        }
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

