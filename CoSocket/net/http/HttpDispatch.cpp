#include "HttpDispatch.h"
#include "SimpleLog.h"

#define RSP_OK "{\"code\": 0, \"message\": \"\"}\n"
#define RSP_ERROR "{\"code\": 1, \"message\": \"Bad Method\"}\n"
#define RSP_NOTFOUND "{\"code\": 1, \"message\": \"Not Found\"}\n"
#define RSP_SERVER_ERR "{\"code\": 1, \"message\": \"Server Error\"}\n"

HttpDispatch::HttpDispatch()
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

        if (!request.Parse(buffer.data(), ret))
        {
            SIMPLE_LOG("http parse error");
            return;
        }

        if (request.IsComplete())
        {
            HttpEncoder response(false);
            Dispatch(request, response);
            if (!Response(connectorPtr, response))
                return;

            request.Reset();
        }
    }
}

void HttpDispatch::Dispatch(const HttpDecoder &request, HttpEncoder &response)
{
    SIMPLE_LOG("Request: %s", request.ToString().c_str());

    HanderMap::iterator it = m_handlers.find(request.GetUrl().c_str());
    if (it == m_handlers.end())
    {
        response.SetStatusCode(HttpEncoder::StatusCode_404NotFound);
        response.SetBody(RSP_NOTFOUND);
        response.SetCloseConnection(true);
    }
    else
    {
        it->second(request, response);
    }
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
