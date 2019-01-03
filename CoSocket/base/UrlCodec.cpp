#include "UrlCodec.h"

namespace url_codec
{

unsigned char ToHex(unsigned char x)
{
    return  x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(unsigned char x)
{
    unsigned char y = 0;
    if (x >= 'A' && x <= 'Z')
        y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z')
        y = x - 'a' + 10;
    else if (x >= '0' && x <= '9')
        y = x - '0';
    return y;
}

std::string UrlEncode(const std::string & str)
{
    std::string encodeStr;
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (isalnum((unsigned char)str[i]) ||
            (str[i] == '-') ||
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~'))
        {
            encodeStr += str[i];
        }
        else if (str[i] == ' ')
        {
            encodeStr += "+";
        }
        else
        {
            encodeStr += '%';
            encodeStr += ToHex((unsigned char)str[i] >> 4);
            encodeStr += ToHex((unsigned char)str[i] % 16);
        }
    }
    return encodeStr;
}

std::string UrlDecode(const std::string & str)
{
    std::string decodeStr;
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (str[i] == '+')
        {
            decodeStr += ' ';
        }
        else if (str[i] == '%')
        {
            if (i + 2 >= str.size())
                continue;

            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            decodeStr += high * 16 + low;
        }
        else
        {
            decodeStr += str[i];
        }
    }
    return decodeStr;
}

}
