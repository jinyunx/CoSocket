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

void HttpDispatch::ResponseOk(HttpResponser &resp)
{
    resp.SetStatusCode(HttpResponser::StatusCode_200Ok);
    resp.SetBody(RSP_OK);
}

void HttpDispatch::ResponseError(HttpResponser &resp)
{
    resp.SetStatusCode(HttpResponser::StatusCode_400BadRequest);
    resp.SetBody(RSP_ERROR);
    resp.SetCloseConnection(true);
}

void HttpDispatch::operator()(TcpServer::ConnectorPtr connectorPtr)
{
    std::string buffer;
    buffer.resize(kBufferSize);
    std::size_t sizeInBuffer = 0;
    HttpRequester request;
    while (1)
    {
        int ret = connectorPtr->Read(&buffer[0] + sizeInBuffer,
                                     buffer.size() - sizeInBuffer,
                                     kKeepAliveMs);
        if (ret <= 0)
        {
            SIMPLE_LOG("read failed, error: %d", ret);
            return;
        }

        sizeInBuffer += ret;

        if (!request.Parse(buffer.data(), sizeInBuffer))
        {
            SIMPLE_LOG("http parse error");
            return;
        }

        if (request.IsComplete())
        {
            HttpResponser response(false);
            Dispatch(request, response);
            if (!Response(connectorPtr, response))
                return;

            request.Reset();
            sizeInBuffer = 0;
        }

        if (sizeInBuffer >= buffer.size())
        {
            SIMPLE_LOG("http too long");
            HttpResponser response(true);
            response.SetStatusCode(HttpResponser::StatusCode_500ServerErr);
            response.SetBody(RSP_SERVER_ERR);

            Response(connectorPtr, response);
            return;
        }
    }
}

void HttpDispatch::Dispatch(const HttpRequester &request, HttpResponser &response)
{
    SIMPLE_LOG("Request: %s", request.ToString().c_str());

    HanderMap::iterator it = m_handlers.find(request.GetUrl().c_str());
    if (it == m_handlers.end())
    {
        response.SetStatusCode(HttpResponser::StatusCode_404NotFound);
        response.SetBody(RSP_NOTFOUND);
        response.SetCloseConnection(true);
    }
    else
    {
        it->second(request, response);
    }
}

bool HttpDispatch::Response(TcpServer::ConnectorPtr &connectorPtr,
                            HttpResponser &response)
{
    std::string resBuffer;
    response.AppendToBuffer(resBuffer);

    std::size_t hasWrite = 0;
    while (hasWrite < resBuffer.size())
    {
        int ret = connectorPtr->Write(resBuffer.c_str() + hasWrite,
                                      resBuffer.size() - hasWrite, kKeepAliveMs);
        if (ret < 0)
        {
            SIMPLE_LOG("write failed, error: %d", ret);
            return false;
        }
        hasWrite += ret;
    }
    return true;
}
