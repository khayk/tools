#include "QueryVisualizer.h"
#include "aggregate/Predicate.h"

#include <core/utils/Str.h>

namespace km::reports {

QueryVisualizer::QueryVisualizer(Config conf)
    : conf_(std::move(conf))
{
    // using SplitterAggr = Splitter<ProcPathAggr, TitleAggr>;
    using TitleAggr = Aggregate<Data, TitleBuilder>;
    using ProcPathAggr = Aggregate<TitleAggr, ProcPathBuilder>;
    using ProcNameAggr = Aggregate<ProcPathAggr, ProcNameBuilder>;

    pathByNameAggr_ = std::make_unique<ProcNameAggr>();
}

void QueryVisualizer::update(const Entry& entry)
{
    std::ignore = entry;
    ++numEntries_;

    if (sw_.elapsedMs() - prevUpdate_ > 100)
    {
        prevUpdate_ = sw_.elapsedMs();
        std::cout << "Processed: " << numEntries_ << "\r" << std::flush;
    }
}

void QueryVisualizer::add(const Entry& entry)
{
    ++numFiltered_;
    pathByNameAggr_->update(entry);
}

void QueryVisualizer::display(std::ostream& os) const
{
    os << "Filtered: " << numFiltered_ << " out of " << numEntries_ << '\n';

    // pathByNameAggr_->write(std::cout, conf_.topN_, 0);
    // pathByNameAggr_->write(std::cout, 0);

    pathByNameAggr_->enumerate(
        conf_.topN_,
        0,
        [&os](std::string_view field,
              std::string_view value,
              uint32_t depth,
              const Data& data) {
            if (value.empty() && depth != 0)
            {
                return;
            }

            os << std::string(4UL * depth, ' ') << field;

            if (!value.empty())
            {
                os << ": " << value << ", ";
            }

            os << "duration: " << core::str::humanizeDuration(data.duration()) << '\n';
        });
}

uint64_t QueryVisualizer::numProcessed() const noexcept
{
    return numEntries_;
}

uint64_t QueryVisualizer::numFiltered() const noexcept
{
    return numFiltered_;
}

} // namespace km::reports
