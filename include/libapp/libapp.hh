#ifndef __PKGCTL_HH__
#define __PKGCTL_HH__

#include <memory>
#include <rlx/rlx.hh>
#include "../default.hh"


#define DEBUG(...) if (debug) io::colored_title(color::blue,"DEBUG",__VA_ARGS__);

using namespace rlx;
using namespace std;

namespace libapp {

    struct app_db_t {
        std::string name, version, release, description;
        std::vector<std::string> depends;
        std::string size;
        std::string build_time, installed_time;
        bool installed = false;
    };

    class obj {
        protected:
            std::string __name,
            __ver, __desc;

            int __rel;

            std::vector<std::string> __depends;
        public:
            obj() {}
            virtual ~obj() {}
            virtual std::string name() { return __name;}
            virtual void load(const std::string&) {}
            virtual std::string ver()  { return __ver; }
            virtual std::string desc() { return __desc;}
            virtual int         rel()  { return __rel; }
            virtual std::vector<std::string> depends() {return __depends;}

            virtual std::string type() {return "invalid";}

            virtual bool __can_handle(const std::string&) {return false;}

            virtual rlx::err::obj Install(conf::obj & config, bool debug) { return err::obj(0x1245, "not implemented");}
            virtual rlx::err::obj Remove(conf::obj& config, bool debug) { return err::obj(0x1245, "not implemented");}
            virtual rlx::err::obj Sync(conf::obj& config, bool debug) { return err::obj(0x1245, "failed to sync repository");}

    };

    typedef std::vector<libapp::obj*> app_list_t;
    std::string hash(std::string f);

    namespace ctl {
        class obj {
            conf::obj config;
            bool debug = false;
        public:
            bool reinstall, redownload, repack, update, skip_dep, skip_pre, skip_post;
            std::string flags;
            obj(const std::string& __c)
            {
                try {
                    config = conf::obj(__c);
                } catch (err::obj e) {
                    if (e.status() != err::file_missing) {
                        throw e;
                    }
                }
                load_modules();
            }
            libapp::obj* get_app(const std::string& a, bool debug = false);

            typedef libapp::obj* (*module_t)(conf::obj);

            std::map<std::string, module_t> modules;

            void load_modules();
            void load_modules(std::string, std::string);

            err::obj Install(const std::string & app, bool debug);
            app_db_t is_installed(const std::string& a, bool debug);
            err::obj Remove(const std::string& app, bool debug);

            err::obj sync_modules(bool debug);

            err::obj download_file(const std::string&, const std::string&, bool prgs = false, bool debug = true);

            app_list_t cal_dep(libapp::obj* app, bool debug);
            std::vector<std::string> list_files(app_db_t& app_data, bool debug);

            err::obj register_data(app_db_t, bool debug);
            //std::vector<std::shared_ptr<rlxpkg::obj>>
            //    calculate_depends(std::shared_ptr<rlxpkg::obj>)

        
            err::obj lock_appctl(string id);

            err::obj unlock_appctl(string id);

        };
    }
}

#endif