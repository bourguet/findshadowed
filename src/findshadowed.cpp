/*
  Copyright (c) 2009, 2017  Jean-Marc Bourguet

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

  * Neither the name of Jean-Marc Bourguet nor the names of the other
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <algorithm>
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

typedef std::map<std::string, std::string> StringStringMap;
typedef std::pair<std::string, std::string> StringsPair;
typedef std::vector<std::string> Strings;
typedef std::vector<std::pair<std::string, std::string> > StringsPairs;

StringStringMap theFiles;
bool theShowFirst = false;
bool theShowShadowed = true;

// -----------------------------------------------------------------------

void usage(std::ostream& os)
{
    os << "Usage: findshadowed [-hfs -Ppath -Idir -Ldir] dir...\n";
}

// -----------------------------------------------------------------------

void help(std::ostream& os)
{
    usage(os);
    os <<
       "   -h     this help\n"
       "   -f     show files not shadowed\n"
       "   -s     don't show shadowed theFiles\n"
       "   -Ppath look in directory of path, path is split at :\n"
       "   -Idir  look in dir\n"
       "   -Ldir  look in dir\n"
       "   dir    look in dir\n";
}

// -----------------------------------------------------------------------

bool lexicalBefore(StringsPair left, StringsPair right)
{
    if (left.second != right.second)
        return left.second < right.second;
    return left.first < right.first;
}

// -----------------------------------------------------------------------

bool isdir(char const* path)
{
    struct stat buf;
    if (stat(path, &buf) != 0) {
        if (errno != ENOENT) {
            perror(path);
            throw EXIT_FAILURE;
        }
        errno = 0;
        return false;
    }
    return S_ISDIR(buf.st_mode);
}

// -----------------------------------------------------------------------

void printString(std::string s) {
    std::cout << "  " << s << '\n';
}

// -----------------------------------------------------------------------

void printPair(StringsPair p) {
    std::cout << "  " << p.first << " (" << p.second << ")\n";
}

// -----------------------------------------------------------------------

void handleDir(std::string const& dirname) {
    DIR* dir = opendir(dirname.c_str());
    if (dir == 0) {
        perror(("Failed to open " + dirname).c_str());
    } else {
        Strings notShadowed;
        StringsPairs shadowed;
        struct dirent* entry;
        while ((entry = readdir(dir)) != 0) {
            if (strcmp(entry->d_name, ".") != 0
                && strcmp(entry->d_name, "..") != 0
                && !isdir((dirname+'/'+entry->d_name).c_str()))
            {
                StringStringMap::iterator previous = theFiles.find(entry->d_name);
                if (previous != theFiles.end()) {
                    shadowed.push_back
                                    (std::make_pair(std::string(entry->d_name), previous->second));
                } else {
                    theFiles[entry->d_name] = dirname;
                    notShadowed.push_back(std::string(entry->d_name));
                }
            }
        }
        std::sort(notShadowed.begin(), notShadowed.end());
        std::sort(shadowed.begin(), shadowed.end(), lexicalBefore);
        if (theShowFirst && notShadowed.begin() != notShadowed.end()) {
            std::cout << "First in " << dirname << '\n';
            std::for_each(notShadowed.begin(), notShadowed.end(), printString);
        } else if (notShadowed.begin() == notShadowed.end()) {
            std::cout << "No files not shadowed in " << dirname << '\n';
        }
        if (theShowShadowed && shadowed.begin() != shadowed.end()) {
            std::cout << "Shadowed in " << dirname << '\n';
            std::for_each(shadowed.begin(), shadowed.end(), printPair);
        }
        closedir(dir);
    }
}

// -----------------------------------------------------------------------

void addPathTo(std::string path, Strings& dirs)
{
    std::string::size_type p = 0;
    std::string::size_type next = 0;
    while (next != std::string::npos) {
        next = path.find(":", p);
        if (next - p != 0)
            dirs.push_back(path.substr(p, next-p));
        p = next+1;
    }
}

// -----------------------------------------------------------------------

int main(int argc, char* argv[])
{
    int c;
    Strings dirs;
    while ((c = getopt(argc, argv, "hfsP:I:L:")) != EOF) {
        switch (c) {
            case 'h':
                help(std::cout);
                throw 0;
                break;
            case 'f':
                theShowFirst = true;
                break;
            case 's':
                theShowShadowed = false;
                break;
            case 'I':
                dirs.push_back(optarg);
                break;
            case 'L':
                dirs.push_back(optarg);
                break;
            case 'P':
                addPathTo(optarg, dirs);
                break;
            case '?':
                usage(std::cerr);
                throw EXIT_FAILURE;
        }
    }
    std::for_each(dirs.begin(), dirs.end(), handleDir);
    std::for_each(argv+optind, argv+argc, handleDir);
    return EXIT_SUCCESS;
}

// -----------------------------------------------------------------------
