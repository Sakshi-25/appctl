#include <librlxpkg/librlxpkg.hh>
#include <libapp/libapp.hh>
#include <unistd.h>
#include <libgen.h>
#include <math.h>

using namespace librlxpkg;


err::obj
obj::Sync(conf::obj& conf, bool debug)
{
    auto rcp_dir = conf.get("dir","recipes",RECIPES_DIR);

    libapp::ctl::obj appctl(conf.filename);
    for (auto a : conf.sections)
    {
        if (a.first == "source.url")
        {
            for(auto b : a.second)
            {
                io::info("syncing ",b.first);
                auto data_file = io::sprint(b.second,"/rcp.meta");
                if (debug) io::info("downloading data file ",data_file);
                err::obj e = appctl.download_file(data_file, "/tmp/.appctl.meta", true);
                if (e.status() != 200) {
                    io::error("failed to sync ",b.first," ",e.mesg());
                    continue;
                }

                std::ifstream fptr("/tmp/.appctl.meta");

                std::string line;
                std::vector<std::string> fileslist;

                int new_rcps = 0;
                int updted_rcps = 0;

                if (fptr.good()) {
                    while(std::getline(fptr, line))
                    {
                        int rdx = line.find_first_of(' ');
                        auto file_addr = line.substr(rdx + 1, line.length() - (rdx + 1));
                        auto file_hash = line.substr(0, rdx);

                        if (file_addr[0] == '.' && file_addr[1] == '/');
                            file_addr = file_addr.substr(2, file_addr.length() - 2);

                        auto abs_file_addr = rcp_dir + "/" + b.first + "/" + file_addr;
                        std::string local_file_hash;
                        if (fs::is_exist(abs_file_addr)) {
                            local_file_hash = libapp::hash(abs_file_addr);
                            if (local_file_hash != file_hash) {
                                if (file_addr.length() >= 7 && file_addr.substr(file_addr.length() - 7, 7) == "/recipe")
                                {
                                    updted_rcps++;
                                }
                                fileslist.push_back(file_addr);
                            }
                        } else {
                            if (file_addr.length() >= 7 && file_addr.substr(file_addr.length() - 7, 7) == "/recipe")
                                new_rcps++;
                            fileslist.push_back(file_addr);
                        }

                    }
                }
                if (new_rcps)     io::info(color::gencode(color::green),new_rcps,color::reset(), color::bold(), " new recipes");
                if (updted_rcps)  io::info(color::gencode(color::green),updted_rcps,color::reset(), color::bold(), " recipes updates");


                int i = 1;
                for(auto file_addr : fileslist)
                {
                    auto abs_file_addr = rcp_dir + "/" + b.first + "/" + file_addr;
                    auto cpy_abs_file_addr = std::string(abs_file_addr);
                    auto abs_file_dir = dirname((char*)cpy_abs_file_addr.c_str());

                    fs::make_dir(abs_file_dir);

                    io::fprint(std::cerr,"updating ", b.first, " ", ceilf((((float)i++/ (float)fileslist.size()) * 100) * 100) / 100, "%      ");

                    auto file_url = b.second + "/" + file_addr;
                    if (debug) io::info("downloading data file ",file_url," -> ", file_addr);
                    err::obj e = appctl.download_file(file_url, abs_file_addr);
                    if (e.status() != 200) {
                        io::error("failed to get file ", file_addr, " ",e.mesg());
                        continue;
                    }
                    std::cout << "  \r";
                }
            }
        }
    }

    return err::obj(0);
}