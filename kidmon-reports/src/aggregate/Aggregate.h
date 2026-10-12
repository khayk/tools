#pragma once

#include <kidmon/data/Types.h>

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <chrono>
#include <vector>

class Data;
using DataCb = std::function<void(std::string_view field,
                                  std::string_view value,
                                  uint32_t depth,
                                  const Data& data)>;

/**
 * Maximum number of children shown per tree level. The last value applies to all
 * deeper levels, empty means no limit.
 */
using TopN = std::vector<uint32_t>;

class IAggregate
{
public:
    virtual ~IAggregate() = default;
    virtual void update(const km::Entry& entry) = 0;
    virtual void enumerate(const TopN& top,
                           uint32_t depth,
                           const DataCb& dataCb) const = 0;

    virtual std::ostream& write(std::ostream& os, uint32_t prefix = 0) const = 0;
};

class Data : public IAggregate
{
    std::chrono::milliseconds duration_ {0};
    uint32_t frequency_ {};

public:
    void update(const km::Entry& entry) override
    {
        duration_ += entry.timestamp.duration;
        ++frequency_;
    }

    std::ostream& write(std::ostream& os, uint32_t prefix) const override
    {
        os << std::string(prefix, ' ') << "dur: " << duration_.count()
           << ", freq: " << frequency_;
        return os;
    }

    void enumerate(const TopN& top,
                   uint32_t depth,
                   const DataCb& dataCb) const override
    {
        std::ignore = top;
        dataCb("", "", depth, *this);
    }

    std::chrono::milliseconds duration() const noexcept
    {
        return duration_;
    }

    uint32_t frequency() const noexcept
    {
        return frequency_;
    }
};

using AggregatePtr = std::unique_ptr<IAggregate>;


class DescendingOrder
{
public:
    bool operator()(const Data& lhs, const Data& rhs) const noexcept
    {
        return lhs.duration() > rhs.duration();
    }
};


template <typename AggregateType, typename FieldBuilder>
class Aggregate : public Data
{
    using MapType = std::map<std::string, AggregateType>;
    using IterVec = std::vector<typename MapType::const_iterator>;

    FieldBuilder fieldBuilder_;
    MapType data_;

    IterVec orderedVec(size_t topN) const
    {
        IterVec reorder;
        reorder.reserve(data_.size());

        for (auto cit = data_.begin(); cit != data_.end(); ++cit)
        {
            reorder.push_back(cit);
        }

        DescendingOrder ordering;
        std::ranges::sort(reorder, [&ordering](const auto& lhs, const auto& rhs) {
            return ordering(lhs->second, rhs->second);
        });

        while (reorder.size() > topN)
        {
            reorder.pop_back();
        }

        return reorder;
    }

public:
    void update(const km::Entry& entry) override
    {
        std::string key = fieldBuilder_.value(entry);
        Data::update(entry);

        data_[key].update(entry);
    }

    void enumerate(const TopN& top,
                   uint32_t depth,
                   const DataCb& dataCb) const override
    {
        const size_t count = top.empty()
                                 ? std::numeric_limits<size_t>::max()
                                 : top[std::min<size_t>(depth, top.size() - 1)];
        const auto reorder = orderedVec(count);
        const auto field = fieldBuilder_.field();
        dataCb("total ", "", depth, *this);
        ++depth;

        for (const auto it : reorder)
        {
            const auto& key = it->first;
            const auto& type = it->second;
            // An empty value marks internal rows, so empty keys need a placeholder
            dataCb(field, key.empty() ? "<empty>" : key, depth, type);
            type.enumerate(top, depth, dataCb);
        }
    }

    std::ostream& write(std::ostream& os, uint32_t prefix) const override
    {
        Data::write(os, prefix);
        prefix += 3;
        os << '\n';

        const auto reorder = orderedVec(std::numeric_limits<size_t>::max());

        for (const auto it : reorder)
        {
            const auto& key = it->first;
            const auto& value = it->second;
            os << std::string(prefix, ' ') << "key: " << key << '\n';
            value.write(os, prefix);
            os << '\n';
        }

        return os;
    }
};
