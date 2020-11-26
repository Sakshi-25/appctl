#ifndef __PKGCTL_RECIPE_HH__
#define __PKGCTL_RECIPE_HH__

#include <libapp/libapp.hh>
#include <rlx/rlx.hh>
#include "../default.hh"

using namespace rlx;

namespace librlxpkg {
    class obj : public libapp::obj {
        private:
            conf::obj config;
            std::string __rcp_file;
        public:
            obj(conf::obj);

            ~obj() {}

            void load(const std::string&);

            std::string type() { return "recipe"; }
            err::obj Install(conf::obj& config, bool debug = false);
            err::obj Remove(conf::obj& config, bool debug = false);
            err::obj Sync(conf::obj& config, bool debug = false);
            err::obj ReInstall(conf::obj& conf, bool debug = false);

            std::string get_recipe_path(const std::string&);

            bool __can_handle(const std::string&);
    };
}

#endif