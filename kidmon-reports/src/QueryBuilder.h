#pragma once

#include "Options.h"
#include "condition/ICondition.h"
#include "transform/ITransform.h"

#include <kidmon/repo/Filter.h>

namespace km::reports {

Filter buildFilter(const ReportsConfig& conf);
ConditionPtr buildCondition(const ReportsConfig& conf);
TransformPtr buildTransform(const ReportsConfig& conf);

} // namespace km::reports
