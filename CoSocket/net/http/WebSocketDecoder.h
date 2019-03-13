#ifndef WEB_SOCKET_DECODER_H
#define WEB_SOCKET_DECODER_H

#include "WebSocketParser.h"

class WebSocketDecoder : private NonCopyable
{
public:
    WebSocketDecoder()
    {
    }

    ~WebSocketDecoder()
    {
    }

    bool Parse(const char *data, size_t len)
    {
        return m_wsParser.Parse(data, len);
    }

    bool IsComplete() const
    {
        return m_wsParser.IsComplete();
    }

    const char *GetPayLoad() const
    {
        return m_wsParser.GetPayLoad();
    }

    size_t GetPayloadLen() const
    {
        return m_wsParser.GetPayloadLen();
    }

    void Reset()
    {
        m_wsParser.Reset();
    }

private:
    WebSocketParser m_wsParser;
};

#endif // WEB_SOCKET_DECODER_H
