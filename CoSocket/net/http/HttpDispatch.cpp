#include "HttpDispatch.h"
#include "WebSocketEncoder.h"
#include "SimpleLog.h"
#include "base64/base64.h"
#include <openssl/sha.h>

#define RSP_OK "{\"code\": 0, \"message\": \"\"}\n"
#define RSP_ERROR "{\"code\": 1, \"message\": \"Bad Method\"}\n"
#define RSP_NOTFOUND "{\"code\": 1, \"message\": \"Not Found\"}\n"
#define RSP_SERVER_ERR "{\"code\": 1, \"message\": \"Server Error\"}\n"

HttpDispatch::HttpDispatch(bool enableWebSocket)
    : m_isWebSocketReq(false),
      m_enableWebSocket(enableWebSocket)
{
}

void HttpDispatch::AddHandler(const std::string &url,
                             const HttpHandler &handler)
{
    m_handlers[url] = handler;
}

void HttpDispatch::ResponseOk(HttpEncoder &resp)
{
    resp.SetStatusCode(HttpEncoder::StatusCode_200Ok);
    resp.SetBody(RSP_OK);
}

void HttpDispatch::ResponseError(HttpEncoder &resp)
{
    resp.SetStatusCode(HttpEncoder::StatusCode_400BadRequest);
    resp.SetBody(RSP_ERROR);
    resp.SetCloseConnection(true);
}

void HttpDispatch::ResponseNotFound(HttpEncoder &resp)
{
    resp.SetStatusCode(HttpEncoder::StatusCode_404NotFound);
    resp.SetBody(RSP_NOTFOUND);
    resp.SetCloseConnection(true);
}

void HttpDispatch::ResponseWebSocketHandshake(HttpEncoder &resp, std::string key)
{
    key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char webSockAccept[SHA_DIGEST_LENGTH + 1] = { 0 };
    SHA1(reinterpret_cast<const unsigned char *>(key.c_str()),
         key.size(), webSockAccept);

    std::string acceptBase64 =
        base64_encode(reinterpret_cast<const char *>(webSockAccept));

    resp.SetStatusCode(HttpEncoder::StatusCode_101WebSocket);
    resp.SetStatusMessage("Switching Protocols");
    resp.AddHeader("Upgrade", "websocket");
    resp.AddHeader("Connection", "Upgrade");
    resp.AddHeader("Sec-WebSocket-Accept", acceptBase64);
}

void HttpDispatch::operator()(TcpServer::ConnectorPtr connectorPtr)
{
    m_connectorPtr = connectorPtr;

    std::string buffer;
    buffer.resize(kBufferSize);
    HttpDecoder request;
    WebSocketParser wsParser;
    while (1)
    {
        ssize_t ret = m_connectorPtr->Read(&buffer[0], buffer.size(), kKeepAliveMs);
        if (ret <= 0)
        {
            SIMPLE_LOG("read failed, error: %d", ret);
            return;
        }

        if (!m_isWebSocketReq)
        {
            if (!ParseToDispatch(buffer.data(), ret, request))
                return;
        }
        else
        {
            if (!ProcessWebSocketData(buffer.data(), ret, wsParser))
                return;
        }
    }
}

bool HttpDispatch::ParseToDispatch(const char *data, size_t len,
                                   HttpDecoder &request)
{
    if (!request.Parse(data, len))
    {
        SIMPLE_LOG("http parse error");
        return false;
    }

    if (request.IsComplete())
    {
        HttpEncoder response(false);
        Dispatch(request, response);
        if (!Response(response.GetReponseString().data(),
                      response.GetReponseString().size()))
            return false;

        // Keep the websocket handshake request
        if (!m_isWebSocketReq)
            request.Reset();
    }

    return true;
}

void HttpDispatch::Dispatch(const HttpDecoder &request, HttpEncoder &response)
{
    SIMPLE_LOG("Request: %s", request.ToString().c_str());

    std::string webSocketKey;
    if (m_enableWebSocket)
    {
        webSocketKey = request.GetHeader("Sec-WebSocket-Key");
        m_isWebSocketReq = !webSocketKey.empty();
    }

    HanderMap::iterator it = m_handlers.find(request.GetUrl().c_str());
    if (it == m_handlers.end())
        ResponseNotFound(response);
    else if (m_isWebSocketReq)
        ResponseWebSocketHandshake(response, webSocketKey);
    else
        it->second(request, response);
}

bool HttpDispatch::Response(const char *data, size_t len)
{
    ssize_t ret = m_connectorPtr->WriteAll(data, len, kKeepAliveMs);
    if (static_cast<size_t>(ret) != len)
    {
        SIMPLE_LOG("write failed, error: %d", ret);
        return false;
    }
    return true;
}

bool HttpDispatch::ProcessWebSocketData(const char *data, size_t len,
                                        WebSocketParser &wsParser)
{
    if (!wsParser.Parse(data, len))
    {
        SIMPLE_LOG("Web socket parse error");
        return false;
    }

    if (wsParser.IsComplete())
    {
        std::string data(wsParser.GetPayLoad(), wsParser.GetPayloadLen());
        SIMPLE_LOG("WebSocket data: %s", data.c_str());
        if (!ResponseWebSocketData(wsParser.GetPayLoad(), wsParser.GetPayloadLen()))
            return false;
        wsParser.Reset();
    }
    return true;
}

bool HttpDispatch::ResponseWebSocketData(const char *data, size_t len)
{
    WebSocketEncoder wsEncoder(false, data, len);
    wsEncoder.SetOpCode(WebSocketParser::OpCodeType_Text);
    while (!wsEncoder.IsComplete())
    {
        const std::string &frame = wsEncoder.GetFrame();
        if (!Response(frame.data(), frame.size()))
            return false;
    }
    return true;
}
