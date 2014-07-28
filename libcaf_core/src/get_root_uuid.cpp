/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright (C) 2011 - 2014                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENCE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#include "caf/config.hpp"
#include "caf/detail/get_root_uuid.hpp"

#if !defined(CAF_MACOS) && !defined(CAF_BSD) // not needed on Mac OS X
namespace {
constexpr char uuid_format[] = "FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF";
} // namespace <anonmyous>
#endif // CAF_MACOS

#if defined(CAF_MACOS) || defined(CAF_BSD)

namespace {

inline void erase_trailing_newline(std::string& str) {
  while (!str.empty() && (*str.rbegin()) == '\n') {
    str.resize(str.size() - 1);
  }
}

#if defined(CAF_MACOS)
constexpr const char* s_get_uuid =
  "/usr/sbin/diskutil info / | "
  "/usr/bin/awk '$0 ~ /UUID/ { print $3 }'";
#else
constexpr const char* s_get_uuid =
  "/sbin/sysctl -a | /usr/bin/grep uuid | "
  "/usr/bin/awk '$0 ~ /kern.hostuuid:/ { print $2 }'";
} // namespace <anonymous>
#endif

namespace caf {
namespace detail {

std::string get_root_uuid() {
  char cbuf[100];
  // fetch hd serial
  std::string uuid;
  FILE* get_uuid_cmd = popen(s_get_uuid, "r");
  while (fgets(cbuf, 100, get_uuid_cmd) != 0) {
    uuid += cbuf;
  }
  pclose(get_uuid_cmd);
  erase_trailing_newline(uuid);
  return uuid;
}

} // namespace detail
} // namespace caf

#elif defined(CAF_LINUX)

#include <vector>
#include <string>
#include <cctype>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <cstring>
#include <unistd.h>
#include <iostream>

#include "caf/string_algorithms.hpp"

using std::vector;
using std::string;
using std::ifstream;

namespace caf {
namespace detail {

namespace {

struct columns_iterator
  : std::iterator<std::forward_iterator_tag, vector<string>> {

  columns_iterator(ifstream* s = nullptr) : fs(s) {}

  vector<string>& operator*() { return cols; }

  columns_iterator& operator++() {
    string line;
    if (!std::getline(*fs, line))
      fs = nullptr;
    else {
      split(cols, line, is_any_of(" "), token_compress_on);
    }
    return *this;
  }

  ifstream* fs;
  vector<string> cols;

};

bool operator==(const columns_iterator& lhs, const columns_iterator& rhs) {
  return lhs.fs == rhs.fs;
}

bool operator!=(const columns_iterator& lhs, const columns_iterator& rhs) {
  return !(lhs == rhs);
}

} // namespace <anonymous>

std::string get_root_uuid() {
  int sck = socket(AF_INET, SOCK_DGRAM, 0);
  if (sck < 0) {
    perror("socket");
    return "";
  }
  // query available interfaces
  char buf[1024];
  ifconf ifc;
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  if (ioctl(sck, SIOCGIFCONF, &ifc) < 0) {
    perror("ioctl(SIOCGIFCONF)");
    return "";
  }
  vector<string> hw_addresses;
  auto ctoi = [](char c)->unsigned {
    return static_cast<unsigned char>(c);

  };
  // iterate through interfaces.
  auto ifr = ifc.ifc_req;
  size_t num_interfaces = ifc.ifc_len / sizeof(ifreq);
  for (size_t i = 0; i < num_interfaces; i++) {
    auto& item = ifr[i];
    // get MAC address
    if (ioctl(sck, SIOCGIFHWADDR, &item) < 0) {
      perror("ioctl(SIOCGIFHWADDR)");
      return "";
    }
    // convert MAC address to standard string representation
    std::ostringstream oss;
    oss << std::hex;
    oss.width(2);
    oss << ctoi(item.ifr_hwaddr.sa_data[0]);
    for (size_t i = 1; i < 6; ++i) {
      oss << ":";
      oss.width(2);
      oss << ctoi(item.ifr_hwaddr.sa_data[i]);
    }
    auto addr = oss.str();
    if (addr != "00:00:00:00:00:00") {
      hw_addresses.push_back(std::move(addr));
    }
  }
  string uuid;
  ifstream fs;
  fs.open("/etc/fstab", std::ios_base::in);
  columns_iterator end;
  auto i =
    find_if(columns_iterator{&fs}, end, [](const vector<string>& cols) {
      return cols.size() == 6 && cols[1] == "/";
    });
  if (i != end) {
    uuid = move((*i)[0]);
    const char cstr[] = {"UUID="};
    auto slen = sizeof(cstr) - 1;
    if (uuid.compare(0, slen, cstr) == 0) uuid.erase(0, slen);
    // UUIDs are formatted as 8-4-4-4-12 hex digits groups
    auto cpy = uuid;
    replace_if(cpy.begin(), cpy.end(), ::isxdigit, 'F');
    // discard invalid UUID
    if (cpy != uuid_format) uuid.clear();
    //   "\\?\Volume{5ec70abf-058c-11e1-bdda-806e6f6e6963}\"
  }
  return uuid;
}

} // namespace detail
} // namespace caf

#elif defined(CAF_WINDOWS)

#include <string>
#include <iostream>
#include <algorithm>

#include <windows.h>
#include <tchar.h>

using namespace std;

namespace caf {
namespace detail {

namespace {
constexpr size_t max_drive_name = MAX_PATH;
}

// if TCHAR is indeed a char, we can simply move rhs
void mv(std::string& lhs, std::string&& rhs) { lhs = std::move(rhs); }

// if TCHAR is defined as WCHAR, we have to do unicode conversion
void mv(std::string& lhs, const std::basic_string<WCHAR>& rhs) {
  auto size_needed = WideCharToMultiByte(CP_UTF8, 0, rhs.c_str(),
                       static_cast<int>(rhs.size()),
                       nullptr, 0, nullptr, nullptr);
  lhs.resize(size_needed);
  WideCharToMultiByte(CP_UTF8, 0, rhs.c_str(), static_cast<int>(rhs.size()),
            &lhs[0], size_needed, nullptr, nullptr);
}

std::string get_root_uuid() {
  using tchar_str = std::basic_string<TCHAR>;
  string uuid;
  TCHAR buf[max_drive_name];    // temporary buffer for volume name
  tchar_str drive = TEXT("c:\\"); // string "template" for drive specifier
  // walk through legal drive letters, skipping floppies
  for (TCHAR i = TEXT('c'); i < TEXT('z'); i++) {
    // Stamp the drive for the appropriate letter.
    drive[0] = i;
    if (GetVolumeNameForVolumeMountPoint(drive.c_str(), buf,
                       max_drive_name)) {
      tchar_str drive_name = buf;
      auto first = drive_name.find(TEXT("Volume{"));
      if (first != std::string::npos) {
        first += 7;
        auto last = drive_name.find(TEXT("}"), first);
        if (last != std::string::npos && last > first) {
          mv(uuid, drive_name.substr(first, last - first));
          // UUIDs are formatted as 8-4-4-4-12 hex digits groups
          auto cpy = uuid;
          replace_if(cpy.begin(), cpy.end(), ::isxdigit, 'F');
          // discard invalid UUID
          if (cpy != uuid_format)
            uuid.clear();
          else
            return uuid; // return first valid UUID we get
        }
      }
    }
  }
  return uuid;
}

} // namespace detail
} // namespace caf

#endif // CAF_WINDOWS
