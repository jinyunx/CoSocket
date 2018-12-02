#ifndef CO_SOCKET_H
#define CO_SOCKET_H

#include <functional>
#include <memory>
#include <map>

struct schedule;
class Epoller;

class CoSocket
{
public:
    typedef std::function<void(void)> CoFunction;

    CoSocket();
    ~CoSocket();

    void Run();
    int NewCoroutine(const CoFunction &func);
    ssize_t Read(int fd, char *buffer, size_t size);

private:
    struct CoKey
    {
        int fd;
        uint32_t events;

        CoKey(int f = 0, uint32_t e = 0)
            : fd(f), events(e)
        {
        }

        bool operator < (const CoKey &other) const
        {
            if (fd < other.fd)
                return true;
            return events < other.events;
        }
    };

    typedef std::map<CoKey, int> CoIdMap;

    static void InterCoFunc(struct schedule *s, void *ud);
    bool SaveToCoIdMap(const CoKey &coKey);
    void EventHandler(int fd, uint32_t events);

    CoIdMap m_coIdMap;
    struct schedule *m_schedule;
    std::unique_ptr<Epoller> m_epoller;
};

#endif // CO_SOCKET_H
