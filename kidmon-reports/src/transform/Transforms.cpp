#include "Transforms.h"
#include <core/utils/Str.h>
#include <kidmon/data/Types.h>


void TitleToLowerTransform::apply(Entry& entry)
{
    str::s2ws(entry.windowInfo.title, wbuffer_);
    str::lowerInplace(wbuffer_);
    str::ws2s(wbuffer_, entry.windowInfo.title);
}


void ProcessPathToLowerTransform::apply(Entry& entry)
{
    auto wstr = entry.processInfo.processPath.wstring();
    str::lowerInplace(wstr);
    entry.processInfo.processPath.assign(wstr);
}


SpreadTransform::SpreadTransform(std::vector<TransformPtr> transformers)
    : transformers_(std::move(transformers))
{
}


void SpreadTransform::apply(Entry& entry)
{
    for (auto& transformer : transformers_)
    {
        transformer->apply(entry);
    }
}
