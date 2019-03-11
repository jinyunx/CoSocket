#ifndef WEB_SOCKET_PARSER_H
#define WEB_SOCKET_PARSER_H

/*
      0               1               2               3
      0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+
*/

#include "NonCopyable.h"
#include "SimpleLog.h"
#include <string>

class WebSocketParser : private NonCopyable
{
public:
    WebSocketParser()
        : m_complete(false),
          m_headerComplete(false),
          m_fin(false),
          m_payloadLen(0),
          m_offset(0)
    {
    }

    ~WebSocketParser()
    {
    }

    bool Parse(const char *data, size_t len)
    {
        if (!data || !len)
            return true;

        m_inputCache.append(data, len);

        if (!m_headerComplete)
            m_headerComplete = ParseHeader();

        // Not enough data
        if (!m_headerComplete)
            return true;

        if (m_payload.size() + m_payloadLen > kMaxLength)
        {
            SIMPLE_LOG("Too large payload size: %"PRIu64,
                       m_payload.size() + m_payloadLen);
            return false;
        }

        // Not enough data
        if (!NeedBytes(m_payloadLen))
            return true;

        m_payload.append(GetDataPtr(), m_payloadLen);
        m_complete = m_fin;
        ResetCurrentFrame();
    }

    bool IsComplete() const
    {
        return m_complete;
    }

    const char *GetPayLoad() const
    {
        return m_data.data();
    }

    size_t GetPayloadLen() const
    {
        return m_data.size();
    }

    void Reset()
    {
        ResetCurrentFrame();
        m_complete = false;
        m_payload.clear();
    }

private:
    void ResetCurrentFrame()
    {
        m_headerComplete = false;
        m_fin = false;
        m_payloadLen = 0;
        m_offset = 0;
        m_inputCache.clear();
    }

    const char *GetDataPtr() const
    {
        return m_inputCache.data() + m_offset;
    }

    bool NeedBytes(uint64_t len)
    {
        return m_inputCache.size() - m_offset >= len;
    }

    bool ParseHeader()
    {
        m_offset = 0;
        if (!NeedBytes(1))
            return false;
        m_fin = 0x80 & *GetDataPtr();
        m_offset += 1;

        if (!NeedBytes(1))
            return false;
        bool mask = 0x80 & *GetDataPtr();
        m_payloadLen = 0x000007FU & (*GetDataPtr());
        m_offset += 1;

        if (m_payloadLen == 126)
        {
            if (!NeedBytes(2))
                return false;
            m_payloadLen = *reinterpret_cast<const uint16_t *>(GetDataPtr());
            m_offset += 2;
        }
        else if (m_payloadLen == 127)
        {
            if (!NeedBytes(8))
                return false;
            m_payloadLen = *reinterpret_cast<const uint64_t *>(GetDataPtr());
            m_offset += 8;
        }

        std::string maskKey;
        if (mask)
        {
            if (!NeedBytes(4))
                return false;
            maskKey.append(GetDataPtr(), 4);
            m_offset += 4;
        }

        return true;
    }

    const static int kMaxLength = 10 * 1024 * 1024; // 10MB

    bool m_complete;
    bool m_headerComplete;
    bool m_fin;
    uint64_t m_payloadLen;
    uint64_t m_offset;
    std::string m_inputCache;
    std::string m_payload;
};

#endif // WEB_SOCKET_PARSER_H
