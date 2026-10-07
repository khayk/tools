#pragma once

#include "aggregate/Aggregate.h"

#include <core/utils/StopWatch.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace km::reports {

class QueryVisualizer
{
    AggregatePtr pathByNameAggr_;

public:
    struct Config
    {
        std::vector<std::string> fields_;
        uint32_t topN_ {10};
    };

    explicit QueryVisualizer(Config conf);

    // Called for every entry read from the repository
    void update(const Entry& entry);

    // Called for entries that passed the query condition
    void add(const Entry& entry);

    void display(std::ostream& os = std::cout) const;

    uint64_t numProcessed() const noexcept;
    uint64_t numFiltered() const noexcept;

private:
    Config conf_;
    StopWatch sw_ {true};
    uint64_t numEntries_ {};
    uint64_t numFiltered_ {};
    int64_t prevUpdate_ {100}; // don't show progress for short queries
};

} // namespace km::reports
