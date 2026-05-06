#include "ITransform.h"
#include <string>
#include <vector>

class TitleToLowerTransform : public ITransform
{
    std::wstring wbuffer_;

public:
    void apply(Entry& entry) override;
};


class ProcessPathToLowerTransform : public ITransform
{
public:
    void apply(Entry& entry) override;
};


class SpreadTransform : public ITransform
{
    std::vector<TransformPtr> transformers_;

public:
    SpreadTransform(std::vector<TransformPtr> transformers);

    void apply(Entry& entry) override;
};
