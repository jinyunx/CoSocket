#ifndef HTTP_CLIENT_H

#include "HttpDecoder.h"
#include "HttpEncoder.h"
#include "NonCopyable.h"
#include "TcpClient.h"

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

        std::size_t writedSize = 0;
        while (writedSize < requestStr.size())
        {
            ssize_t ret = m_tcpClient.Write(requestStr.data() + writedSize,
                                            requestStr.size() - writedSize,
                                            kKeepAliveMs);
            if (ret < 0)
                return ret;
            writedSize += ret;
        }

        std::string buffer;
        buffer.resize(kBufferSize);
        std::size_t sizeInBuffer = 0;
        m_httpDecoder.Reset();
        while (1)
        {
            ssize_t ret = m_tcpClient.Read(&buffer[0] + sizeInBuffer,
                                           buffer.size() - sizeInBuffer,
                                           kKeepAliveMs);
            if (ret <= 0)
                return ret;

            sizeInBuffer += ret;

            if (!m_httpDecoder.Parse(buffer.data(), sizeInBuffer))
                return -EINVAL;

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
    const static int kBufferSize = 1024;

    HttpDecoder m_httpDecoder;
    HttpEncoder m_httpEncoder;
    TcpClient &m_tcpClient;
};

#endif // HTTP_CLIENT_H
