#ifndef HTTP_ENCODER_H
#define HTTP_ENCODER_H

#include "NonCopyable.h"
#include <stdio.h>
#include <string>
#include <map>
#include <sstream>

class HttpEncoder : private NonCopyable
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
        StatusCode_101WebSocket = 101,
    };

    explicit HttpEncoder(bool close)
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

    void SetMethod(const std::string &method)
    {
        m_method = method;
    }

    void SetUrl(const std::string &url)
    {
        m_url = url;
    }

    void AddParameter(const std::string &key, const std::string &value)
    {
        m_parameters[key] = value;
    }

    std::string GetReponseString() const
    {
        std::ostringstream oss;
        oss << "HTTP/1.1 " << m_statusCode << " " << m_statusMessage << "\r\n";
        AppendToStream(oss);
        return oss.str();
    }

    std::string GetRequestString() const
    {
        std::ostringstream oss;
        oss << m_method << " " << m_url;
        std::map<std::string, std::string>::const_iterator it = m_parameters.begin();
        for (; it != m_parameters.end(); ++it)
        {
            oss << (it == m_parameters.begin() ? "?" : "&");
            oss << it->first << "=" << it->second;
        }
        oss << " HTTP/1.1\r\n";
        AppendToStream(oss);
        return oss.str();
    }

    void AppendToStream(std::ostringstream &oss) const
    {
        oss << "Content-Length: " << m_body.size() << "\r\n";

        if (m_closeConnection)
            oss << "Connection: close\r\n";
        else
            oss << "Connection: Keep-Alive\r\n";

        std::map<std::string, std::string>::const_iterator it = m_headers.begin();
        for (; it != m_headers.end(); ++it)
            oss << it->first << ": " << it->second << "\r\n";

        oss << "\r\n";
        oss << m_body;
    }

private:
    std::map<std::string, std::string> m_headers;
    StatusCode m_statusCode;
    std::string m_statusMessage;
    bool m_closeConnection;
    std::string m_body;

    // request
    std::string m_method;
    std::string m_url;
    std::map<std::string, std::string> m_parameters;
};

#endif // HTTP_ENCODER_H
