#pragma once

#include "Options.h"
#include "condition/ICondition.h"

#include <kidmon/repo/Filter.h>
#include <kidmon/repo/Repository.h>

namespace km::reports {

/**
 * Throws std::invalid_argument if the user is not given or has no recorded data
 */
void validateUser(const IRepository& repo, const std::string& username);

Filter buildFilter(const ReportsConfig& conf);
ConditionPtr buildCondition(const ReportsConfig& conf);

} // namespace km::reports
