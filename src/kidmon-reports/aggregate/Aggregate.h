#pragma once

#include <kidmon/data/Types.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <chrono>

class Data;
using DataCb = std::function<void(std::string_view field,
                                  std::string_view value,
                                  uint32_t depth,
                                  const Data& data)>;

class IAggregate
{
public:
    virtual ~IAggregate() = default;
    virtual void update(const Entry& entry) = 0;
    virtual void enumarate(size_t topN,
                           uint32_t depth,
                           const DataCb& dataCb) const = 0;

    virtual std::ostream& write(std::ostream& os, uint32_t prefix = 0) const = 0;
};

class Data : public IAggregate
{
    std::chrono::milliseconds duration_ {0};
    uint32_t frequency_ {};

public:
    void update(const Entry& entry) override
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

    void enumarate(size_t topN, uint32_t depth, const DataCb& dataCb) const override
    {
        std::ignore = topN;
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


/*
template <typename First, typename Second>
class Splitter : public IAggregate
{
    First first;
    Second second;

public:
    void update(const Entry& entry) override
    {
        first.update(entry);
        second.update(entry);
    }

    void enumarate(size_t topN, uint32_t depth, const DataCb& dataCb) const override
    {
        first.enumarate(topN, depth, dataCb);
        second.enumarate(topN, depth, dataCb);
    }

    std::ostream& write(std::ostream& os, uint32_t prefix) const
    {
        os << std::string(prefix, ' ') << "split ------- \n";
        prefix += 3;
        first.write(os, prefix);
        second.write(os, prefix);

        return os;
    }
};
*/


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
    void update(const Entry& entry) override
    {
        std::string key = fieldBuilder_.value(entry);
        Data::update(entry);

        data_[key].update(entry);
    }

    void enumarate(size_t topN, uint32_t depth, const DataCb& dataCb) const override
    {
        const auto reorder = orderedVec(topN);
        const auto field = fieldBuilder_.field();
        dataCb("total ", "", depth, *this);
        ++depth;

        for (const auto it : reorder)
        {
            const auto& key = it->first;
            const auto& type = it->second;
            dataCb(field, key, depth, type);
            type.enumarate(topN, depth, dataCb);
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
