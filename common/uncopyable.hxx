#ifndef UNCOPYABLE_HXX
#define UNCOPYABLE_HXX
class Uncopyable {
public:
    Uncopyable() = default;
    virtual ~Uncopyable() = default;
private:
    Uncopyable(const Uncopyable&) = delete;
    Uncopyable& operator=(const Uncopyable&) = delete;
};
#endif