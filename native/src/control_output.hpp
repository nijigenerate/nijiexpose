#pragma once
#include <filesystem>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#endif

namespace nijiexpose::control {
// Exclusive creation is essential: checking symlink_status before opening
// would leave a race and would not reject hard links.
inline bool writeResponse(const std::filesystem::path& directory,
                          const std::filesystem::path& filename,
                          const std::string& text) {
    if(filename.empty() || filename != filename.filename()) return false;
    auto temporary=filename; temporary+=".tmp";
#ifdef _WIN32
    // Keep the inbox from being renamed/replaced while publishing the response.
    HANDLE dir=CreateFileW(directory.c_str(),FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(dir==INVALID_HANDLE_VALUE)return false;
    BY_HANDLE_FILE_INFORMATION info{};
    if(!GetFileInformationByHandle(dir,&info) ||
       (info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)) {
        CloseHandle(dir);return false;
    }
    auto path=directory/temporary;
    HANDLE file=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){CloseHandle(dir);return false;}
    DWORD written=0;
    bool ok=text.size()<=MAXDWORD &&
        WriteFile(file,text.data(),static_cast<DWORD>(text.size()),&written,nullptr) &&
        written==text.size();
    CloseHandle(file);
    // No replacement: an existing response (including a link) is never touched.
    if(ok)ok=MoveFileW(path.c_str(),(directory/filename).c_str())!=0;
    if(!ok)DeleteFileW(path.c_str());
    CloseHandle(dir);
    return ok;
#else
    int dir=open(directory.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
    if(dir<0)return false;
    int file=openat(dir,temporary.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
    if(file<0){close(dir);return false;}
    size_t offset=0;
    while(offset<text.size()) {
        auto n=write(file,text.data()+offset,text.size()-offset);
        if(n<0 && errno==EINTR)continue;
        if(n<=0)break;
        offset+=static_cast<size_t>(n);
    }
    // linkat publishes atomically without replacing any existing destination.
    // Keep the descriptor open until publication completes.
    bool ok=offset==text.size() &&
        linkat(dir,temporary.c_str(),dir,filename.c_str(),0)==0;
    close(file);
    unlinkat(dir,temporary.c_str(),0);
    close(dir);
    return ok;
#endif
}
}
