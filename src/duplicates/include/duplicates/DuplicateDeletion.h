#pragma once

#include <duplicates/DeletionStrategy.h>
#include <duplicates/DuplicateDetector.h>
#include <duplicates/Config.h>
#include <vector>
#include <unordered_set>
#include <filesystem>

namespace tools::dups {

using PathsVec = std::vector<fs::path>;
using PathsSet = std::unordered_set<fs::path>;

void deleteFiles(DeletionStrategy& strategy, const PathsVec& files);

bool deleteInteractively(DeletionStrategy& strategy,
                         PathsVec& files,
                         PathsSet& ignoredFiles);

void deleteDuplicates(const Config& cfg, const DuplicateDetector& detector);

} // namespace tools::dups