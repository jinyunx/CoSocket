#ifndef HTTP_RESPONSER_H
#define HTTP_RESPONSER_H

#include "NonCopyable.h"
#include <stdio.h>
#include <string>
#include <map>

class HttpResponser : private NonCopyable
{
public:
    enum StatusCode
    {
        StatusCode_Unknown,
        StatusCode_200Ok = 200,
        StatusCode_301MovedPermanently = 301,
        StatusCode_400BadRequest = 400,
        StatusCode_404NotFound = 404,
        StatusCode_500ServerErr = 500,
    };

    explicit HttpResponser(bool close)
        : m_statusCode(StatusCode_Unknown),
          m_closeConnection(close)
    { }

    void SetStatusCode(StatusCode code)
    {
        m_statusCode = code;
    }

    void SetStatusMessage(const std::string &message)
    {
        m_statusMessage = message;
    }

    void SetCloseConnection(bool on)
    {
        m_closeConnection = on;
    }

    bool CloseConnection() const
    {
        return m_closeConnection;
    }

    void SetContentType(const std::string &contentType)
    {
        AddHeader("Content-Type", contentType);
    }

    void AddHeader(const std::string &key, const std::string &value)
    {
        m_headers[key] = value;
    }

    void SetBody(const std::string &body)
    {
        m_body = body;
    }

    // FIXME: not only support string
    void AppendToBuffer(std::string &output) const
    {
        char buf[32];
        snprintf(buf, sizeof buf, "HTTP/1.1 %d ", m_statusCode);
        output.append(buf);
        output.append(m_statusMessage);
        output.append("\r\n");

        snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", m_body.size());
        output.append(buf);

        if (m_closeConnection)
            output.append("Connection: close\r\n");
        else
            output.append("Connection: Keep-Alive\r\n");

        for (std::map<std::string, std::string>::const_iterator it = m_headers.begin();
             it != m_headers.end(); ++it)
        {
            output.append(it->first);
            output.append(": ");
            output.append(it->second);
            output.append("\r\n");
        }

        output.append("\r\n");
        output.append(m_body);
    }

private:
    std::map<std::string, std::string> m_headers;
    StatusCode m_statusCode;
    std::string m_statusMessage;
    bool m_closeConnection;
    std::string m_body;
};

#endif // HTTP_RESPONSER_H
