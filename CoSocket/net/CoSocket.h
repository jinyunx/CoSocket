#ifndef CO_SOCKET_H
#define CO_SOCKET_H

#include "Timestamp.h"

#include <functional>
#include <memory>
#include <map>
#include <set>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>

struct schedule;
class Epoller;

class CoSocket
{
public:
    class TimeoutSet;

    typedef std::function<void(void)> CoFunction;

    CoSocket();
    ~CoSocket();

    void Run();
    void NewCoroutine(const CoFunction &func);

    int Connect(int fd, const struct sockaddr *addr,
                socklen_t addrlen, int64_t timeoutMs);
    int Accept(int fd, struct sockaddr *addr,
               socklen_t *addrlen, int64_t timeoutMs);
    ssize_t Read(int fd, char *buffer, size_t size, int64_t timeoutMs);
    ssize_t Write(int fd, const char *buffer, size_t size, int64_t timeoutMs);

private:
    class FdWatcher
    {
    public:
        FdWatcher()
            : m_readCoId(-1), m_writeCoId(-1)
        {
        }

        bool HasReadCoroutine() const;
        bool HasWriteCoroutine() const;

        int ReadCoId() const;
        int WriteCoId() const;

        bool SetReadCoId(int coId);
        bool SetWriteCoId(int coId);

        void ResetReadCoId();
        void ResetWriteCoId();

    private:
        int m_readCoId;
        int m_writeCoId;
    };

    typedef std::map<int, FdWatcher> CoIdMap;
    typedef std::list<CoFunction> CoFuncList;

    int AddEventAndYield(int fd, uint32_t events);
    void DeleteEvent(int fd, uint32_t events);

    bool SaveToCoIdMap(int fd, uint32_t events);
    void ResetInCoIdMap(int fd, uint32_t events);
    uint32_t GetEvents(int fd);

    static void InterCoFunc(struct schedule *s, void *ud);
    void EventHandler(int fd, uint32_t events);

    bool m_handlingTimeout;
    CoFuncList m_coFuncList;
    std::unique_ptr<TimeoutSet> m_timeoutSet;
    CoIdMap m_coIdMap;
    struct schedule *m_schedule;
    std::unique_ptr<Epoller> m_epoller;
};

#endif // CO_SOCKET_H
