#ifndef HTTP_DECODER_H
#define HTTP_DECODER_H

#include "HttpParser.h"
#include "NonCopyable.h"

class HttpDecoder : private NonCopyable
{
public:
    HttpDecoder()
    { }

    ~HttpDecoder()
    { }

    bool Parse(const char *data, size_t len)
    {
        return m_httpParser.Parse(data, len);
    }

    bool IsComplete() const
    {
        return m_httpParser.IsComplete();
    }

    void Reset()
    {
        m_httpParser.Reset();
    }

    const std::string & GetUrl() const
    {
        return m_httpParser.GetUrl();
    }

    const std::string & GetMethod() const
    {
        return m_httpParser.GetMethod();
    }

    const std::string & GetBody() const
    {
        return m_httpParser.GetBody();
    }

    const HttpParser::HeaderMap & GetHeaders() const
    {
        return m_httpParser.GetHeaders();
    }

    const char * GetHeader(const std::string &name) const
    {
        return m_httpParser.GetHeader(name);
    }

    void SetPeerIp(const std::string &peerIp)
    {
        m_peerIp = peerIp;
    }

    void SetPeerPort(unsigned short peerPort)
    {
        m_peerPort = peerPort;
    }

    const std::string & GetPeerIp() const
    {
        return m_peerIp;
    }

    unsigned short GetPeerPort() const
    {
        return m_peerPort;
    }

    std::string ToString() const
    {
        return m_httpParser.ToString();
    }

    std::string GetErrorDetail() const
    {
        return m_httpParser.GetErrorDetail();
    }

private:
    HttpParser m_httpParser;
    std::string m_peerIp;
    unsigned short m_peerPort;
};

#endif // HTTP_DECODER_H
