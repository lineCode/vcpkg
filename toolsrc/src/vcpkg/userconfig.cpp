#include "pch.h"

#include <vcpkg/base/files.h>
#include <vcpkg/base/lazy.h>
#include <vcpkg/paragraphs.h>
#include <vcpkg/userconfig.h>

namespace
{
    static vcpkg::Lazy<fs::path> s_localappdata;

    static const fs::path& get_localappdata()
    {
        return s_localappdata.get_lazy([]() {
            fs::path localappdata;
            {
                // Config path in AppDataLocal
                wchar_t* localappdatapath = nullptr;
                if (S_OK != SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localappdatapath)) __fastfail(1);
                localappdata = localappdatapath;
                CoTaskMemFree(localappdatapath);
            }
            return localappdata;
        });
    }
}

namespace vcpkg
{
    UserConfig UserConfig::try_read_data(const Files::Filesystem& fs)
    {
        UserConfig ret;

        try
        {
            auto maybe_pghs = Paragraphs::get_paragraphs(fs, get_localappdata() / "vcpkg" / "config");
            if (const auto p_pghs = maybe_pghs.get())
            {
                const auto& pghs = *p_pghs;

                std::unordered_map<std::string, std::string> keys;
                if (pghs.size() > 0) keys = pghs[0];

                for (size_t x = 1; x < pghs.size(); ++x)
                {
                    for (auto&& p : pghs[x])
                        keys.insert(p);
                }

                ret.user_id = keys["User-Id"];
                ret.user_time = keys["User-Since"];
                ret.user_mac = keys["Mac-Hash"];
                ret.last_completed_survey = keys["Survey-Completed"];
            }
        }
        catch (...)
        {
        }

        return ret;
    }

    void UserConfig::try_write_data(Files::Filesystem& fs) const
    {
        try
        {
            std::error_code ec;
            fs.create_directory(get_localappdata() / "vcpkg", ec);
            fs.write_contents(get_localappdata() / "vcpkg" / "config",
                              Strings::format("User-Id: %s\n"
                                              "User-Since: %s\n"
                                              "Mac-Hash: %s\n"
                                              "Survey-Completed: %s\n",
                                              user_id,
                                              user_time,
                                              user_mac,
                                              last_completed_survey));
        }
        catch (...)
        {
        }
    }
}
