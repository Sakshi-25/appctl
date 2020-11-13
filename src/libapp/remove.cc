#include <libapp/libapp.hh>


using namespace libapp;

err::obj
ctl::obj::Remove(const std::string & app, bool debug)
{   

    auto _app_ptr = get_app(app, debug);
    if (_app_ptr == nullptr)
        return err::obj(0x10, "no app found in database with name " + app);
    
    return _app_ptr->Remove(config, debug);
}