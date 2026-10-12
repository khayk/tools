#include "QueryVisualizer.h"
#include "aggregate/Predicate.h"

#include <core/utils/Str.h>

#include <cstdio>

#ifdef _WIN32
    #include <io.h>
#else
    #include <unistd.h>
#endif

namespace km::reports {

namespace {

bool isStderrTerminal() noexcept
{
#ifdef _WIN32
    return _isatty(_fileno(stderr)) != 0;
#else
    return isatty(fileno(stderr)) != 0;
#endif
}

} // namespace

QueryVisualizer::QueryVisualizer(Config conf)
    : conf_(std::move(conf))
    , showProgress_(isStderrTerminal())
{
    using TitleAggr = Aggregate<Data, TitleBuilder>;
    using ProcPathAggr = Aggregate<TitleAggr, ProcPathBuilder>;
    using ProcNameAggr = Aggregate<ProcPathAggr, ProcNameBuilder>;

    pathByNameAggr_ = std::make_unique<ProcNameAggr>();
}

void QueryVisualizer::update(const Entry& entry)
{
    std::ignore = entry;
    ++numEntries_;

    // Progress goes to stderr, and only to a terminal, to keep the report clean
    if (showProgress_ && sw_.elapsedMs() - prevUpdate_ > 100)
    {
        prevUpdate_ = sw_.elapsedMs();
        progressShown_ = true;
        std::cerr << "Processed: " << numEntries_ << "\r" << std::flush;
    }
}

void QueryVisualizer::add(const Entry& entry)
{
    ++numFiltered_;
    pathByNameAggr_->update(entry);
}

void QueryVisualizer::display(std::ostream& os) const
{
    if (progressShown_)
    {
        std::cerr << std::string(32, ' ') << '\r' << std::flush;
    }

    os << "Filtered: " << numFiltered_ << " out of " << numEntries_ << '\n';

    pathByNameAggr_->enumerate(
        conf_.top_,
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
