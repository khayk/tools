#pragma once

#include <functional>
#include <memory>
#include <vector>

namespace dp {

template<class ...Args>
class Callback
{
public:
    using Function    = std::function<void(Args...)>;
    using FunctionPtr = std::shared_ptr<Function>;
    using Functions   = std::vector<FunctionPtr>;

    FunctionPtr add(Function fn)
    {
        FunctionPtr fp = std::make_shared<Function>(std::move(fn));
        pending_.push_back(fp);
        changed_ = true;

        return fp;
    }

    void remove(FunctionPtr fp)
    {
        auto it = std::find(std::begin(funcs_), std::end(funcs_), fp);

        if (std::end(funcs_) != it)
        {
            (*it).reset();
            changed_ = true;
        }

        removeIf(pending_, fp);
    }

    void operator()(Args... args)
    {
        if (changed_)
        {
            applyChanges();
        }

        for (auto& fp : funcs_)
        {
            if (fp)
            {
                (*fp)(args ...);
            }
        }
    }

private:
    Functions pending_;
    Functions funcs_;
    bool changed_{ false };

    void applyChanges()
    {
        funcs_.insert(funcs_.end(), pending_.begin(), pending_.end());
        pending_.clear();

        FunctionPtr fp;
        removeIf(funcs_, fp);
        changed_ = false;
    }

    void removeIf(Functions& funcs, const FunctionPtr& fn)
    {
        auto it = std::remove_if(funcs.begin(), funcs.end(),
            [&fn](const FunctionPtr& fp) noexcept {
                return fn == fp;
            });

        funcs.erase(it, funcs.end());
    }
};

} // namespace dp