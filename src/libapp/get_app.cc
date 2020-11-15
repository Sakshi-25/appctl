#include <libapp/libapp.hh>

using namespace libapp;

libapp::obj*
ctl::obj::get_app(const std::string & app_id, bool debug)
{
    for(auto a : modules) {
        auto m = a.second(config);
        if (m->__can_handle(app_id)) {
            m->load(app_id);
            return m;
        }
        delete m;
    }
    if (debug) io::error(11,"no module found to handle " + app_id);
    //throw err::obj(-176, "unable to find "+ app_id + " in repository");
    return nullptr;
}