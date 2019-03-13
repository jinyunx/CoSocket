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
#include <arpa/inet.h>
#include <string>

#define ntohll(x) ((((uint64_t)ntohl(x&0xFFFFFFFF)) << 32) + ntohl(x >> 32))
#define htonll(x)   ((((uint64_t)htonl(x&0xFFFFFFFF)) << 32) + htonl(x >> 32))

class WebSocketParser : private NonCopyable
{
public:
    enum OpCodeType
    {
        OpCodeType_Continue = 0,
        OpCodeType_Text     = 1,
        OpCodeType_Binary   = 2,
        OpCodeType_Close    = 8,
        OpCodeType_Ping     = 9,
        OpCodeType_Pong     = 10,
    };

    WebSocketParser()
        : m_complete(false),
          m_headerComplete(false),
          m_fin(false),
          m_opCode(0),
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

        if (!IsValidFrame())
            return false;

        ProcessPayload();
        return true;
    }

    bool IsComplete() const
    {
        return m_complete;
    }

    const char *GetPayLoad() const
    {
        return m_payload.data();
    }

    size_t GetPayloadLen() const
    {
        return m_payload.size();
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

    bool HaveBytes(uint64_t len)
    {
        return m_inputCache.size() - m_offset >= len;
    }

    bool ParseHeader()
    {
        m_offset = 0;
        if (!HaveBytes(1))
            return false;
        m_opCode = 0x0F & *GetDataPtr();
        m_fin = 0x80 & *GetDataPtr();
        m_offset += 1;

        if (!HaveBytes(1))
            return false;
        bool mask = 0x80 & *GetDataPtr();
        m_payloadLen = 0x000007FU & (*GetDataPtr());
        m_offset += 1;

        if (m_payloadLen == 126)
        {
            if (!HaveBytes(2))
                return false;
            m_payloadLen = ntohs(*reinterpret_cast<const uint16_t *>(GetDataPtr()));
            m_offset += 2;
        }
        else if (m_payloadLen == 127)
        {
            if (!HaveBytes(8))
                return false;
            m_payloadLen = ntohll(*reinterpret_cast<const uint64_t *>(GetDataPtr()));
            m_offset += 8;
        }

        m_maskKey.clear();
        if (mask)
        {
            if (!HaveBytes(4))
                return false;
            m_maskKey.append(GetDataPtr(), 4);
            m_offset += 4;
        }

        return true;
    }

    bool IsValidFrame()
    {
        if (m_maskKey.empty())
        {
            SIMPLE_LOG("Not support unmask frame");
            return false;
        }

        if (!CheckOpCode())
        {
            SIMPLE_LOG("Invalid opcode: %d", m_opCode);
            return false;
        }

        if (m_payload.size() + m_payloadLen > kMaxLength)
        {
            SIMPLE_LOG("Too large payload size: %d",
                       m_payload.size() + m_payloadLen);
            return false;
        }

        return true;
    }

    bool CheckOpCode()
    {
        switch (m_opCode)
        {
            case OpCodeType_Continue:
                if (m_payload.empty())
                    return false;

            case OpCodeType_Text:
            case OpCodeType_Binary:
            case OpCodeType_Close:
            case OpCodeType_Ping:
            case OpCodeType_Pong:
                return true;

            default:
                return false;
        }
    }

    void ProcessPayload()
    {
        // Not enough data
        if (!HaveBytes(m_payloadLen))
            return;

        for (uint64_t i = 0; i < m_payloadLen; ++i, ++m_offset)
            m_payload.push_back(*GetDataPtr() ^ m_maskKey[i % m_maskKey.size()]);

        m_complete = m_fin;
        ResetCurrentFrame();
    }

    const static int kMaxLength = 10 * 1024 * 1024; // 10MB

    bool m_complete;
    bool m_headerComplete;
    bool m_fin;
    int m_opCode;
    uint64_t m_payloadLen;
    uint64_t m_offset;

    std::string m_maskKey;
    std::string m_inputCache;
    std::string m_payload;
};

#endif // WEB_SOCKET_PARSER_H
