#include <librlxpkg/librlxpkg.hh>

using namespace librlxpkg;
using namespace std;

err::obj
obj::Remove(conf::obj & _conf, bool debug)
{
    
    string libsh = config.get("dir","libexec",LIBEXEC_DIR);
    string data_dir = config.get("dir","data", DATA_DIR);
    string root_dir = config.get("dir", "roots", ROOT_DIR);

    libsh += "/rlxpkg.sh";

    if (!fs::is_exist(libsh)) return err::obj(err::file_missing, libsh);

    if (! fs::is_exist(data_dir + "/" + __name + "/info")) {
        return err::obj(0x14, __name + " is not already installed");
    }
    
    io::process("removing ", __name);
    string _cmd = io::sprint("bash -c '",
        " source ",
        libsh, "; ",
        " ROOT_DIR=",root_dir,
        " DATA_DIR=",data_dir,
        " CONFIG_FILE=",config.filename,
        " ",
        " rlxpkg_remove ", __name, "'"
        );
    
    if (debug) DEBUG("executing ", _cmd);

    int ret = system(_cmd.c_str());

    switch (WEXITSTATUS(ret)) {
        case 101:
            return err::obj(0x101, "internal error, invalid arguments provided to rlxpkg");
        
        case 65:
            return err::obj(0x65, "appctl database is locked");
        
        case 15:
            return err::obj(0x15, "data file for " + __name + " is missing");

        case 16:
            return err::obj(0x16, "files list data is missing for " + __name);

        case 99:
            return err::obj(0x99, "configuration file is missing : " + config.filename);

        default:
            if (WEXITSTATUS(ret) != 0) {
                return err::obj(WEXITSTATUS(ret), "unknown error");
            }

    }

    io::success("removed " + __name + " successfully");
    return err::obj(0);
}