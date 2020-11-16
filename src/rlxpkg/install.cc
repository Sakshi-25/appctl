#include <librlxpkg/librlxpkg.hh>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>

using namespace std;

err::obj
librlxpkg::obj::Install(conf::obj& config, bool debug)
{
    string pkgname = io::sprint(
        __name, "-", __ver, "-", __rel, "-x86_64.rlx"
    );

    string root_dir   = config.get("dir","roots", ROOT_DIR);
    string _cache_dir = config.get("dir","cache", CACHE_DIR);
    string work_dir   = config.get("dir","work", io::sprint(_cache_dir,"/work"));
    string pkg_dir    = config.get("dir", "pkg", io::sprint(_cache_dir,"/pkg"));
    string src_dir    = config.get("dir", "src", io::sprint(_cache_dir,"/src"));
    string _xc        = config.get("local", "bash", "");

    string _s_dir = work_dir + "/" + __name + "/src";
    string _p_dir = work_dir + "/" + __name + "/pkg";

    string pkgfile = io::sprint(pkg_dir, "/", pkgname);

    string _bash_pre = "bash " + _xc + " -c ";

    string libsh   = config.get("dir","libexec", LIBEXEC_DIR);
    libsh += "/rlxpkg.sh";

    libapp::ctl::obj _appctl(config.filename);

    auto _e = _appctl.lock_appctl(__name);
    if (_e.status()) {
        return _e;
    }


    if (!fs::is_exist(libsh)) return err::obj(err::file_missing, libsh);

    if (fs::is_exist(pkgfile)) {
        if (debug) DEBUG ("found ", pkgname, " in cache");
    } else {

        // Download Source file

        {
            io::process("downloading source file");
            string _cmd = io::sprint(
                _bash_pre,"'",
                " source ",
                libsh, "; ",
                "REDOWNLOAD=",config.get("local","redownload","0"),
                "WGET_ARGS=",config.get("flags","wget",""),
                " rlxpkg_download ",
                __rcp_file, " ",
                src_dir,"'"
            );

            int ret = WEXITSTATUS(system(_cmd.c_str()));

            string _mesg = "unknown error while downloading source";

            switch (ret) {
                case 5:
                    _mesg = "Internal error, invalid argument provided to rxpkg.sh";
                    break;

                case 6:
                    _mesg = "failed to load recipe file " + __rcp_file;
                    break;

                case 7:
                    _mesg = "failed to download source file";
                    break;

                case 8:
                    _mesg = "failed to check downloaded file";
                    break;        

            }

            if (ret) {
                _appctl.unlock_appctl(__name);
                return err::obj(ret, _mesg);
            }
        }
        
        

        // Preparing source file

        {
            io::process("preparing source files");
            string _cmd = io::sprint(
                _bash_pre,"'",
                " source ",
                libsh, "; ",
                " rlxpkg_prepare ",
                __rcp_file, " ",
                src_dir, " ",
                _s_dir, "'"
                );

            int ret = WEXITSTATUS(system(_cmd.c_str()));
            string _mesg = "unknown error while preparing source";

            switch (ret) {
                case 5:
                    _mesg = "Internal error, invalid argument provided to rxpkg.sh";
                    break;

                case 6:
                    _mesg = "failed to load recipe file " + __rcp_file;
                    break;

                case 7:
                    _mesg = "specified source file missing ";
                    break;

                case 8:
                    _mesg = "failed to prepare source file";
                    break;

                case 9:
                    _mesg = "failed to copy file";
                    break;          

            }

            if (ret) {
                _appctl.unlock_appctl(__name);
                return err::obj(ret, _mesg);
            }

        }


        // Compile source file

        {
            io::process("compiling source file");
            string _cmd = io::sprint(
                _bash_pre,"'",
                " source ",
                libsh, "; ",
                " rlxpkg_build ",
                __rcp_file, " ",
                _s_dir, " ",
                _p_dir, "'"
                );

            int ret = WEXITSTATUS(system(_cmd.c_str()));
            string _mesg = "unknown error while compiling source";

            switch (ret) {
                case 5:
                    _mesg = "Internal error, invalid argument provided to rxpkg.sh";
                    break;

                case 6:
                    _mesg = "failed to load recipe file " + __rcp_file;
                    break;

                case 7:
                    _mesg = "failed to compile source";
                    break;          

            }

            if (ret != 0) {
                _appctl.unlock_appctl(__name);
                return err::obj(ret, _mesg);
            }
        }


        // compress package

        {
            io::process("compressing package");
            string _cmd = io::sprint(
                _bash_pre,"'",
                " source ",
                libsh, "; ",
                " rlxpkg_genpkg ",
                __rcp_file, " ",
                _p_dir, " ",
                pkg_dir, "'"
                );

            int ret = WEXITSTATUS(system(_cmd.c_str()));
            string _mesg = "unknown error while compressing package";

            switch (ret) {
                case 5:
                    _mesg = "Internal error, invalid argument provided to rxpkg.sh";
                    break;

                case 6:
                    _mesg = "failed to load recipe file " + __rcp_file;
                    break;

                case 7:
                    _mesg = "failed to compress package";
                    break;          

            }

            if (ret) {
                _appctl.unlock_appctl(__name);
                return err::obj(ret, _mesg);
            }
        }


    }

    io::process("installing ", __name);
    if (debug) DEBUG("package file ", pkgfile);

    string _cmd =
        io::sprint(
        _bash_pre,"'",
        " source ",
        libsh," ; ",
        " WORK_DIR=", work_dir,
        " ROOT_DIR=", config.get("dir","roots","/"),
        " SKIP_EXECS=",config.get("local","skip_execs","0"),
        " DATA_DIR=",config.get("dir","data","/var/lib/app/index"),
        " COMPRESS_ALOG=", config.get("config","compression", "zstd"),
        " rlxpkg_install ", pkgfile, "'");

    int ret = WEXITSTATUS(system(_cmd.c_str()));
    string _mesg = "unknown error while installing package";

    switch (ret) {
        case 5:
            _mesg = "Internal error, invalid argument provided to rxpkg.sh";
            break;

        case 6:
            _mesg = "pkg not found" + pkgfile;
            break;

        case 7:
            _mesg = "invalid package";
            break;

        case 8:
            _mesg = "information data missing";
            break;

        case 9:
            _mesg = "failed to execute pre install script";
            break;          

    }
    if (ret) {
        _appctl.unlock_appctl(__name);
        return err::obj(ret, _mesg);
    }

    io::success("sucessfully install ", __name);

    return _appctl.unlock_appctl(__name);
}