#ifndef CO_SOCKET_H
#define CO_SOCKET_H

#include <functional>
#include <memory>
#include <map>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>

struct schedule;
class Epoller;

class CoSocket
{
public:
    typedef std::function<void(void)> CoFunction;

    CoSocket();
    ~CoSocket();

    void Run();
    void NewCoroutine(const CoFunction &func);

    int Connect(int fd, const struct sockaddr *addr,
                socklen_t addrlen);
    int Accept(int fd, struct sockaddr *addr,
               socklen_t *addrlen);
    ssize_t Read(int fd, char *buffer, size_t size);
    ssize_t Write(int fd, const char *buffer, size_t size);

private:
    typedef std::map<int, int> CoIdMap;
    typedef std::list<CoFunction> CoFuncList;

    int AddEventAndYield(int fd, uint32_t events);
    void DeleteEvent(int fd);
    static void InterCoFunc(struct schedule *s, void *ud);
    bool SaveToCoIdMap(int fd);
    void EraseCoIdMap(int fd);
    void EventHandler(int fd, uint32_t events);

    CoFuncList m_coFuncList;
    CoIdMap m_coIdMap;
    struct schedule *m_schedule;
    std::unique_ptr<Epoller> m_epoller;
};

#endif // CO_SOCKET_H
