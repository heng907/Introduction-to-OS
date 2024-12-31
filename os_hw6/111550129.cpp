/*
Student No.: 111550129
Student Name: 林彥亨
Email: yanheng.cs11@nycu.edu.tw
SE tag: xnxcxtxuxoxsx
Statement: I am fully aware that this program is not supposed to be posted to a public server, such as a public GitHub repository or a public web page.
*/


#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <string.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include <regex>

using namespace std;

static struct fuse_operations op = {};

struct header_posix_ustar {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char modify[12];
    char checksum[8];
    char linkflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];

};

uint64_t oct_dec(const char* data, size_t size) {
    uint64_t result = 0;
    while (size-- && isdigit(*data)) {
        result = (result << 3) | (*data++ - '0');
    }
    return result;
}


mode_t get_mode(const header_posix_ustar* header) {
    return oct_dec(header->mode, 8);
}

short get_uid(const header_posix_ustar* header) {
    return oct_dec(header->uid, 8);
}

short get_gid(const header_posix_ustar* header) {
    return oct_dec(header->gid, 8);
}

size_t get_size(const header_posix_ustar* header) {
    return oct_dec(header->size, 12);
}

time_t get_time(const header_posix_ustar* header) {
    return oct_dec(header->modify, 12);
}

bool check_checksum(const header_posix_ustar* header) {
    char original[8];
    memcpy(original, header->checksum, 8);
    memset(const_cast<char*>(header->checksum), ' ', 8);

    uint64_t unsigned_sum = 0;
    uint64_t signed_sum = 0;

    const unsigned char* data = reinterpret_cast<const unsigned char*>(header);
    for (size_t i = 0; i < sizeof(header_posix_ustar); ++i) {
        unsigned_sum += data[i];
        signed_sum += static_cast<int8_t>(data[i]);
    }

    memcpy(const_cast<char*>(header->checksum), original, 8);
    uint64_t stored_checksum = oct_dec(original, 8);
    return (stored_checksum == unsigned_sum || stored_checksum == signed_sum);
}


map<string, set<string>> file_dir;
map<string, struct stat *> file_attri;
map<string, char *> file_content;
map<string, string> file_symlink;


int my_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) 
{
     /*do something*/
    string file_path(path);

    if (file_path[file_path.size() - 1] != '/') {
        file_path += '/';
    }

    filler(buffer, ".", nullptr, 0); 
    filler(buffer, "..", nullptr, 0); 


    if (file_dir.count(file_path) == 1) {
        for (string file : file_dir[file_path]) {
            filler(buffer, file.c_str(), nullptr, 0);
        }
        return 0;
    }

    return -ENOENT;
}

int my_getattr(const char *path, struct stat *st) 
{
    /*do something*/
  memset(st, 0, sizeof(struct stat));

  string file_path(path);

  if (file_path == "/") {
    st->st_mode = S_IFDIR | 0755;
    st->st_nlink = 0;
    return 0;
  }

  if (file_attri.count(file_path) == 1) {
    memcpy(st, file_attri[file_path], sizeof(struct stat));
    if (file_symlink.count(file_path)){
      st->st_mode = S_IFLNK | (st->st_mode & 0777);
      st->st_nlink = 1;
    }
    st->st_nlink = 0;
    return 0;
  }

  if (file_dir.count(file_path + "/") == 1) {
    st->st_mode = S_IFDIR | 0755;
    st->st_nlink = 0;
    return 0;
  }

  return -ENOENT;
}

int my_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) 
{ /*do something*/ 
    string file_path(path);

    if (file_symlink.count(file_path) == 1) {
        return -EACCES;
    }

    if (file_content.count(file_path)) {
        size_t f_size = file_attri[file_path]->st_size;

        if (offset >= f_size) {
            return 0;
        }

        if (offset + size > f_size) {
            size = f_size - offset;
        }

        memcpy(buffer, file_content[file_path] + offset, size);
        return size;
    }

    return -ENOENT;
}

int my_readlink(const char *path, char *buffer, size_t size) 
{/* do something */ 
    string file_path(path);

    if (file_symlink.count(file_path) == 1) {
        string& link_target = file_symlink[file_path];
        if (link_target.size() >= size) {
            return -ENAMETOOLONG;
        }
        strcpy(buffer, link_target.c_str());
        return 0;
    }

    return -ENOENT;
}



int main(int argc, char* argv[]) {
    fstream file("test.tar", ios::in | ios::binary);

    if (!file.is_open()) {
        cerr << "Error: Could not open 'test.tar'" << endl;
        return 1;
    }

    char tar_end[512] = {0};

    while (!file.eof()) {
        header_posix_ustar tar_header{};
        file.read(reinterpret_cast<char*>(&tar_header), 512);

        if (memcmp(&tar_header, tar_end, 512) == 0) {
            break;
        }

        if (!check_checksum(&tar_header)) {
            cerr << "Warning: Checksum mismatch for file '" << tar_header.name << "'" << endl;
            continue;
        }

        string filename = "/" + string(tar_header.name);
        if (filename.back() == '/') {
            filename.pop_back();
        }
        string dir = filename.substr(0, filename.find_last_of('/') + 1);
        string base = filename.substr(filename.find_last_of('/') + 1);

        file_dir[dir].insert(base);

        struct stat* st = new struct stat;
        memset(st, 0, sizeof(struct stat));
        st->st_uid = get_uid(&tar_header);
        st->st_gid = get_gid(&tar_header);
        st->st_mtime = get_time(&tar_header);

        switch (tar_header.linkflag) {
            case '0':
            case '\0':
                st->st_mode = S_IFREG | get_mode(&tar_header);
                st->st_size = get_size(&tar_header);

                if (st->st_size > 0) {
                    char* buffer = new char[st->st_size];
                    file.read(buffer, st->st_size);
                    file_content[filename] = buffer;
                }
                break;

            case '2': 
                st->st_mode = S_IFLNK | 0777;
                st->st_size = 0;
                file_symlink[filename] = string(tar_header.linkname);
                break;

            case '5':
                st->st_mode = S_IFDIR | get_mode(&tar_header);
                st->st_size = 0;
                break;

            default:
                delete st;
                continue;
        }

        
        if (file_attri.count(filename)) {
            delete file_attri[filename];
        }
        file_attri[filename] = st;

        size_t file_size = get_size(&tar_header);
        size_t padding = (512 - (file_size % 512)) % 512;
        if (file_size > 0 && padding > 0) {
            file.ignore(padding);
        }
    }

    memset(&op, 0, sizeof(op));
    op.getattr = my_getattr;
    op.readdir = my_readdir;
    op.read = my_read;
    op.readlink = my_readlink;

    return fuse_main(argc, argv, &op, NULL);
}
