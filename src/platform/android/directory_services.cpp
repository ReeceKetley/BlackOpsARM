#include "file_services.h"
#include <universal/mem_userhunk.h>
#include <universal/q_shared.h>
#include <universal/com_shared.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>

namespace {
[[noreturn]] void fail(const char *message) {
    std::fprintf(stderr,"Native directory services: %s\n",message);std::abort();
}
bool excludedDirectory(const char *name) {
    return !I_stricmp(name,".") || !I_stricmp(name,"..") || !I_stricmp(name,"CVS");
}
}

HunkUser **Sys_ListFiles(char *directory,char *extension,char *filter,int *numfiles,int wantsubs) {
    if(!directory || !numfiles)fail("invalid enumeration arguments");
    *numfiles=0;
    const auto path=bo1::android::ResolveEngineFilePath(directory);
    if(path.empty())return nullptr;
    DIR *handle=opendir(path.c_str());
    if(!handle)return nullptr; // Reference returns an empty list for inaccessible/missing directories.
    std::vector<std::string> names;
    const bool directoryExtension=extension && !std::strcmp(extension,"/");
    const std::string suffix=(!extension || directoryExtension)?"":extension;
    try {
        while(auto *entry=readdir(handle)) {
            struct stat info{};
            const auto child=path+"/"+entry->d_name;
            if(stat(child.c_str(),&info))continue;
            const bool isDirectory=S_ISDIR(info.st_mode);
            if(isDirectory && excludedDirectory(entry->d_name))continue;
            std::string name=entry->d_name;
            if(filter) {
                // The validated Windows reader lists this directory only and
                // prefixes the empty subdirs argument with a backslash.
                name="\\"+name;
                if(!Com_FilterPath(filter,name.c_str(),0))continue;
            } else {
                const bool directories=wantsubs || directoryExtension;
                if(isDirectory!=directories)continue;
                if(!suffix.empty() && I_stricmpwild(("*."+suffix).c_str(),name.c_str()))continue;
            }
            names.push_back(std::move(name));
            if(names.size()==0x3fff)break; // Original maximum enumeration policy.
        }
    } catch (...) {closedir(handle);throw;}
    closedir(handle);
    if(names.empty())return nullptr;
    HunkUser *owner=Hunk_UserCreate(0x20000,HU_SCHEME_DEFAULT,0,0,"Sys_ListFiles",3);
    // Owner prefix + N names + terminating null. Use native pointer alignment.
    auto **list=static_cast<char **>(Hunk_UserAlloc(owner,
        static_cast<int>((names.size()+2)*sizeof(void *)),alignof(void *),nullptr));
    list[0]=reinterpret_cast<char *>(owner);
    for(std::size_t i=0;i<names.size();++i)
        list[i+1]=Hunk_CopyString(owner,names[i].c_str());
    list[names.size()+1]=nullptr;
    *numfiles=static_cast<int>(names.size());
    return reinterpret_cast<HunkUser **>(list+1);
}
