#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "NonCopyable.h"
#include "http-parser/http_parser.h"
#include <string>
#include <map>

class HttpParser : private NonCopyable
{
public:
    typedef std::map<std::string, std::string> HeaderMap;
    typedef std::map<std::string, std::string> Parameters;

    HttpParser();

    bool Parse(const char *data, size_t len);
    bool IsComplete() const;
    void Reset();
    const char * GetErrorDetail() const;

    const std::string & GetUrl() const;
    const std::string & GetMethod() const;
    const std::string & GetBody() const;
    const HeaderMap & GetHeaders() const;
    const Parameters & GetParameters() const;
    const char * GetHeader(const std::string &name) const;
    const char * GetParameter(const std::string &name) const;
    std::string ToString() const;

private:
    enum HeaderState
    {
        HeaderState_None = 0,
        HeaderState_HasName = 0x01,
        HeaderState_HasVaule = 0x10,
        HeaderState_HasAll = 0x11,
    };

    bool HeaderReady() const;

    static int OnUrl(http_parser* parser, const char* at, size_t length);
    static int OnHeaderName(http_parser* parser, const char* at, size_t length);
    static int OnHeaderValue(http_parser* parser, const char* at, size_t length);
    static int OnBody(http_parser* parser, const char* at, size_t length);
    static int OnComplete(http_parser* parser);

    static std::string GetStringBefore(
        char flag, const std::string urlAndParams, std::size_t offset);
    static void ParseUrlAndParam(HttpParser *httpParser);

    static http_parser_settings m_settings;

    std::string m_url;
    std::string m_urlAndParams;
    std::string m_method;
    HeaderState m_headerState;
    std::string m_headerName;
    std::string m_headerValue;
    HeaderMap m_headers;
    Parameters m_parameters;
    std::string m_body;
    bool m_complete;

    http_parser m_httpParser;
};

#endif // HTTP_PARSER_H
