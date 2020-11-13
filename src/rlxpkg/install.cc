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

    string pkgdir  = config.get("dir","pkgdir", PACKAGES_DIR);
    string pkgfile = io::sprint(pkgdir, "/", pkgname);
    string libsh   = config.get("dir","libexec", LIBEXEC_DIR);
    libsh += "/rlxpkg.sh";


    if (!fs::is_exist(libsh)) return err::obj(err::file_missing, libsh);

    if (fs::is_exist(pkgfile)) {
        if (debug) DEBUG ("found ", pkgname, " in cache");
    } else {

        string _cmd = io::sprint("bash -c '",
            " source ",
            libsh,"; ",
            "RECIPE_FILE=",__rcp_file," "
            "CONFIG_FILE=",config.filename,"; "
            " rlxpkg_compile ",
            __name, " ",
            __ver,  " ",
            __rel,  " ",
            pkgfile, "'");
        
        if (debug) DEBUG("executing ", _cmd );
        int ret = system(
            _cmd.c_str()
        );

        if (WEXITSTATUS(ret) != 0)  return err::obj(err::execution, "failed to compile recipe " + __name);
        if (!fs::is_exist(pkgfile)) return err::obj(err::file_missing, "build sucess but not pkgfile generated ");
        io::success("successfully compiler ", __name);
    }

    io::process("installing ", __name);
    if (debug) DEBUG("package file ", pkgfile);

    int ret = system(
        io::sprint("bash -c 'source ",
        libsh," ; ",
        " ROOT_DIR=", config.get("dir","roots","/"),
        " SKIP_EXECS=",config.get("local","skip_execs","0"),
        " DATA_DIR=",config.get("dir","data","/var/lib/app/index"),
        " rlxpkg_install ", pkgfile, "'").c_str()
    );

    switch (WEXITSTATUS(ret)) {
        case 0:
            io::success("installed ", __name);
            return err::obj(0);
    }

    return err::obj(WEXITSTATUS(ret));
}