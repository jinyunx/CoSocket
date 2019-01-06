#ifndef HTTP_CLIENT_H

#include "HttpDecoder.h"
#include "HttpEncoder.h"
#include "NonCopyable.h"
#include "TcpClient.h"
#include "SimpleLog.h"

class HttpClient : private NonCopyable
{
public:
    HttpClient(TcpClient &tcpClient)
        : m_httpEncoder(false),
          m_tcpClient(tcpClient)
    {
    }

    ~HttpClient()
    {
    }

    ssize_t Request()
    {
        std::string requestStr = m_httpEncoder.GetRequestString();

        // Write to http server
        ssize_t ret = m_tcpClient.WriteAll(
            requestStr.data(), requestStr.size(), kKeepAliveMs);
        if (ret < 0)
            return ret;

        // Read data and parse
        std::string buffer;
        buffer.resize(kBufferSize);
        m_httpDecoder.Reset();
        while (1)
        {
            ssize_t ret = m_tcpClient.Read(&buffer[0], buffer.size(), kKeepAliveMs);
            if (ret < 0)
                return ret;

            // Peek close too early
            if (ret == 0)
                return -EPIPE;

            if (!m_httpDecoder.Parse(buffer.data(), ret))
            {
                std::string p(buffer.data(), ret);
                SIMPLE_LOG("%s", p.c_str());
                SIMPLE_LOG("%s", m_httpDecoder.GetErrorDetail().c_str());
                return -EINVAL;
            }

            if (m_httpDecoder.IsComplete())
                return 0;
        }
    }

    unsigned int GetStatus() const
    {
        return m_httpDecoder.GetStatus();
    }

    const std::string & GetBody() const
    {
        return m_httpDecoder.GetBody();
    }

    const HttpParser::HeaderMap & GetHeaders() const
    {
        return m_httpDecoder.GetHeaders();
    }

    const char * GetHeader(const std::string &name) const
    {
        return m_httpDecoder.GetHeader(name);
    }

    void AddHeader(const std::string &key, const std::string &value)
    {
        m_httpEncoder.AddHeader(key, value);
    }

    void SetBody(const std::string &body)
    {
        m_httpEncoder.SetBody(body);
    }

    void SetMethod(const std::string &method)
    {
        m_httpEncoder.SetMethod(method);
    }

    void SetUrl(const std::string &url)
    {
        m_httpEncoder.SetUrl(url);
    }

    void AddParameter(const std::string &key, const std::string &value)
    {
        m_httpEncoder.AddParameter(key, value);
    }

private:
    const static int kKeepAliveMs = 3000;
    const static int kBufferSize = 102400;

    HttpDecoder m_httpDecoder;
    HttpEncoder m_httpEncoder;
    TcpClient &m_tcpClient;
};

#endif // HTTP_CLIENT_H
