#ifndef NON_COPYABLE_H
#define NON_COPYABLE_H

class NonCopyable
{
public:
    NonCopyable(const NonCopyable &) = delete;
    void operator = (const NonCopyable &) = delete;

protected:
    NonCopyable() = default;
    virtual ~NonCopyable() = default;
};

#endif // NON_COPYABLE_H
