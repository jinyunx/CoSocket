#ifndef URL_CODEC_H
#define URL_CODEC_H

#include <string>

namespace url_codec
{

std::string UrlEncode(const std::string & str);
std::string UrlDecode(const std::string & str);

}

#endif // URL_CODEC_H
