#ifndef WEB_SOCKET_ENCODER_H
#define WEB_SOCKET_ENCODER_H

#include "WebSocketParser.h"
#include <inttypes.h>

class WebSocketEncoder : private NonCopyable
{
public:
    explicit WebSocketEncoder(bool isClient)
        : m_isClient(isClient),
          m_opCode(WebSocketParser::OpCodeType_Binary),
          m_frameSize(kFrameSize),
          m_data(0),
          m_len(0),
          m_offset(0)
    {
    }

    ~WebSocketEncoder()
    {
    }

    void SetPayload(const char *data, uint64_t len)
    {
        m_data = data;
        m_len = len;
    }

    void SetOpCode(WebSocketParser::OpCodeType code)
    {
        m_opCode = code;
    }

    void SetFrameSize(uint64_t frameSize)
    {
        m_frameSize = frameSize;
    }

    bool IsComplete() const
    {
        return m_offset == m_len;
    }

    const std::string &GetFrame()
    {
        Encode();
        return m_outputBuf;
    }

private:

    void Encode()
    {
        m_outputBuf.clear();

        // Not the first frame
        if (m_offset)
            m_opCode = WebSocketParser::OpCodeType_Continue;

        char fin = 0x00;
        uint64_t frameSize = m_frameSize;
        if (m_frameSize > m_len - m_offset)
        {
            frameSize = m_len - m_offset;
            fin = 0x80;
        }

        // Fin and op code
        m_outputBuf.push_back(fin | m_opCode);

        // Mask and payload len
        if (frameSize > 0xFFFF)
        {
            m_outputBuf.push_back(static_cast<int>(m_isClient) << 7 | 127);
            uint64_t netFrameSize = htonll(frameSize);
            m_outputBuf.append(reinterpret_cast<char *>(&netFrameSize), 4);
        }
        else if (frameSize > 126)
        {
            m_outputBuf.push_back(static_cast<int>(m_isClient) << 7 | 126);
            uint16_t netFrameSize = htons(static_cast<uint16_t>(frameSize));
            m_outputBuf.append(reinterpret_cast<char *>(&netFrameSize), 2);
        }
        else
        {
            m_outputBuf.push_back(static_cast<int>(m_isClient) << 7 | frameSize);
        }

        if (!m_isClient)
        {
            m_outputBuf.append(m_data, frameSize);
        }
        else
        {
            uint32_t mask = random();
            char *maskPtr = reinterpret_cast<char *>(&mask);
            m_outputBuf.append(maskPtr, 4);
            for (uint64_t i = 0; i < frameSize; ++i)
                m_outputBuf.push_back(m_data[i] ^ maskPtr[i % sizeof(mask)]);
        }

        m_data += frameSize;
        m_offset += frameSize;
    }

    const static int kFrameSize = 1446;

    bool m_isClient;
    int m_opCode;
    uint64_t m_frameSize;

    const char *m_data;
    uint64_t m_len;
    uint64_t m_offset;

    std::string m_outputBuf;
};

#endif // WEB_SOCKET_ENCODER_H
