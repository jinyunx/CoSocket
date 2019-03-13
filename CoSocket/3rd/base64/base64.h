#ifndef BASE64_H
#define BASE64_H

#include <string>

std::string base64_encode(const ::std::string &bindata);
std::string base64_decode(const ::std::string &ascdata);

#endif // BASE64_H
