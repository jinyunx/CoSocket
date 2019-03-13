#include "HttpDispatch.h"
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
    m_handlers[url].httpHandler = handler;
}

void HttpDispatch::AddHandler(const std::string &url,
                              const WebSocketHandler &handler)
{
    m_handlers[url].webSocketHandler = handler;
}

void HttpDispatch::ResponseHttpOk(HttpEncoder &httpResponse)
{
    httpResponse.SetStatusCode(HttpEncoder::StatusCode_200Ok);
    httpResponse.SetBody(RSP_OK);
}

void HttpDispatch::ResponseHttpError(HttpEncoder &httpResponse)
{
    httpResponse.SetStatusCode(HttpEncoder::StatusCode_400BadRequest);
    httpResponse.SetBody(RSP_ERROR);
    httpResponse.SetCloseConnection(true);
}

void HttpDispatch::ResponseHttpNotFound(HttpEncoder &httpResponse)
{
    httpResponse.SetStatusCode(HttpEncoder::StatusCode_404NotFound);
    httpResponse.SetBody(RSP_NOTFOUND);
    httpResponse.SetCloseConnection(true);
}

void HttpDispatch::ResponseWebSocketHandshake(HttpEncoder &httpResponse, std::string key)
{
    key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char webSockAccept[SHA_DIGEST_LENGTH + 1] = { 0 };
    SHA1(reinterpret_cast<const unsigned char *>(key.c_str()),
         key.size(), webSockAccept);

    std::string acceptBase64 =
        base64_encode(reinterpret_cast<const char *>(webSockAccept));

    httpResponse.SetStatusCode(HttpEncoder::StatusCode_101WebSocket);
    httpResponse.SetStatusMessage("Switching Protocols");
    httpResponse.AddHeader("Upgrade", "websocket");
    httpResponse.AddHeader("Connection", "Upgrade");
    httpResponse.AddHeader("Sec-WebSocket-Accept", acceptBase64);
}

void HttpDispatch::operator()(TcpServer::ConnectorPtr connectorPtr)
{
    m_connectorPtr = connectorPtr;

    std::string buffer;
    buffer.resize(kBufferSize);
    HttpDecoder httpRequest;
    WebSocketDecoder wsDecoder;
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
            if (!ParseHttpToDispatch(buffer.data(), ret, httpRequest))
                return;
        }
        else
        {
            if (!ProcessWebSocketData(buffer.data(), ret, wsDecoder))
                return;
        }
    }
}

bool HttpDispatch::ParseHttpToDispatch(const char *data, size_t len,
                                       HttpDecoder &httpRequest)
{
    if (!httpRequest.Parse(data, len))
    {
        SIMPLE_LOG("http parse error");
        return false;
    }

    if (httpRequest.IsComplete())
    {
        HttpEncoder httpResponse(false);
        Dispatch(httpRequest, httpResponse);
        if (!Response(httpResponse.GetReponseString().data(),
                      httpResponse.GetReponseString().size()))
            return false;

        httpRequest.Reset();
    }

    return true;
}

void HttpDispatch::Dispatch(const HttpDecoder &httpRequest, HttpEncoder &httpResponse)
{
    SIMPLE_LOG("Request: %s", httpRequest.ToString().c_str());

    std::string webSocketKey;
    if (m_enableWebSocket)
    {
        webSocketKey = httpRequest.GetHeader("Sec-WebSocket-Key");
        m_isWebSocketReq = !webSocketKey.empty();
    }

    HanderMap::iterator it = m_handlers.find(httpRequest.GetUrl().c_str());
    if (it == m_handlers.end() ||
        (m_isWebSocketReq && !it->second.webSocketHandler) ||
        (!m_isWebSocketReq && !it->second.httpHandler))
    {
        ResponseHttpNotFound(httpResponse);
    }
    else if (m_isWebSocketReq)
    {
        ResponseWebSocketHandshake(httpResponse, webSocketKey);
        m_wsHandler = it->second.webSocketHandler;
    }
    else
    {
        it->second.httpHandler(httpRequest, httpResponse);
    }
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
                                        WebSocketDecoder &wsDecoder)
{
    if (!wsDecoder.Parse(data, len))
    {
        SIMPLE_LOG("Web socket parse error");
        return false;
    }

    if (wsDecoder.IsComplete())
    {
        WebSocketEncoder wsEncoder(false);
        m_wsHandler(wsDecoder, wsEncoder);
        if (!ResponseWebSocketData(wsEncoder))
            return false;

        wsDecoder.Reset();
    }

    return true;
}

bool HttpDispatch::ResponseWebSocketData(WebSocketEncoder &wsEncoder)
{
    while (!wsEncoder.IsComplete())
    {
        const std::string &frame = wsEncoder.GetFrame();
        if (!Response(frame.data(), frame.size()))
            return false;
    }
    return true;
}
