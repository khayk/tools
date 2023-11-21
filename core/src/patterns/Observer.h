#pragma once

#include <algorithm>
#include <vector>

namespace dp {

template <class Observer>
class Observable
{
public:
    using ObserverType = Observer;

    /**
     * @brief Add an observer
     */
    void add(Observer& obs)
    {
        obs_.push_back(&obs);
    }

    /**
     * @brief Remove an observer
     */
    void remove(Observer& obs)
    {
        if (auto it = std::find(std::begin(obs_), std::end(obs_), &obs);
            std::end(obs_) != it)
        {
            *it = nullptr;
            dirty_ = true;
        }
    }

    /**
     * @brief Notify observers about change, handle functions with any number of arguments
     *
     * @tparam Fn   Type of the function.
     * @tparam Args Type of the arguments.
     * @param fn    Pointer to member function to be called, it should be the part of an
     *              interface for the Observer type.
     * @param args  Variable arguments providing [in,out] The arguments.
     */
    template <typename Fn, typename... Args>
    void notify(Fn fn, Args&&... args) noexcept
    {
        for (auto* obs : obs_)
        {
            if (obs)
            {
                (obs->*fn)(std::forward<Args>(args)...);
            }
        }

        // Some observers are removed while we are notifying
        if (dirty_)
        {
            cleanup();
        }
    }

private:
    std::vector<Observer*> obs_;
    bool dirty_ {false};

    void cleanup()
    {
        auto it =
            std::remove_if(obs_.begin(), obs_.end(), [](const Observer* obs) noexcept {
                return obs == nullptr;
            });
        obs_.erase(it, obs_.end());
        dirty_ = false;
    }
};


/**
 * @brief Helper class to perform automatic add/remove on behalf of
 *        the observer during the lifetime of this class instance.
 *
 * @note Observable should be an instance of Observable<Observer> class
 */
template <typename Observer>
class ScopedObserve
{
    using ObservableType = Observable<Observer>;

    ObservableType& observable_ {nullptr};
    Observer& observer_ {nullptr};

public:
    /**
     * @brief Adds observer to listen events from observable during the construction
     *        and removes observer on destruction
     */
    ScopedObserve(ObservableType& observable, Observer& observer)
        : observable_(observable)
        , observer_(observer)
    {
        observable_.add(observer_);
    }

    ~ScopedObserve()
    {
        observable_.remove(observer_);
    }
};

} // namespace dp