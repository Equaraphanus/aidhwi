#pragma once

#include <functional>
#include <list>

template <typename... Args>
class Event {
public:
    using delegate_t = std::function<void(Args...)>;

    Event() : m_delegates() {}
    ~Event() {}

    void operator+=(const delegate_t& delegate) { m_delegates.push_back(delegate); }

    void operator-=(const delegate_t& delegate) {
        auto it = std::find_if(m_delegates.begin(), m_delegates.end(), [&delegate](auto& d) {
            return delegate.target_type() == d.target_type();
        });
        if (it != m_delegates.end())
            m_delegates.erase(it);
    }

    void Invoke(Args... args) {
        for (auto& d : m_delegates)
            if (d)
                d(args...);
    }

private:
    std::list<delegate_t> m_delegates;

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
};
