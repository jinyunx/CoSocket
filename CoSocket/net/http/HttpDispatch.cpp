#include "HttpDispatch.h"
#include "SimpleLog.h"
#include "base64/base64.h"
#include <openssl/sha.h>

#define RSP_OK "{\"code\": 0, \"message\": \"\"}\n"
#define RSP_ERROR "{\"code\": 1, \"message\": \"Bad Method\"}\n"
#define RSP_NOTFOUND "{\"code\": 1, \"message\": \"Not Found\"}\n"
#define RSP_SERVER_ERR "{\"code\": 1, \"message\": \"Server Error\"}\n"

HttpDispatch::HttpDispatch(bool enableWebSocket)
    : m_webSocket(false),
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
    std::string buffer;
    buffer.resize(kBufferSize);
    HttpDecoder request;
    while (1)
    {
        ssize_t ret = connectorPtr->Read(&buffer[0], buffer.size(), kKeepAliveMs);
        if (ret <= 0)
        {
            SIMPLE_LOG("read failed, error: %d", ret);
            return;
        }

        if (!m_webSocket)
        {
            if (!ParseToDispatch(buffer.data(), ret, request, connectorPtr))
                return;
        }
        else
            ; // websocket handler
    }
}

bool HttpDispatch::ParseToDispatch(const char *data, size_t len,
                                   HttpDecoder &request,
                                   TcpServer::ConnectorPtr &connectorPtr)
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
        if (!Response(connectorPtr, response))
            return false;

        // Keep the websocket handshake request
        if (!m_webSocket)
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
        m_webSocket = !webSocketKey.empty();
    }

    HanderMap::iterator it = m_handlers.find(request.GetUrl().c_str());
    if (it == m_handlers.end())
        ResponseNotFound(response);
    else if (m_webSocket)
        ResponseWebSocketHandshake(response, webSocketKey);
    else
        it->second(request, response);
}

bool HttpDispatch::Response(TcpServer::ConnectorPtr &connectorPtr,
                            HttpEncoder &response)
{
    std::string resBuffer = response.GetReponseString();

    ssize_t ret = connectorPtr->WriteAll(
        resBuffer.c_str(), resBuffer.size(), kKeepAliveMs);
    if (static_cast<size_t>(ret) != resBuffer.size())
    {
        SIMPLE_LOG("write failed, error: %d", ret);
        return false;
    }
    return true;
}
