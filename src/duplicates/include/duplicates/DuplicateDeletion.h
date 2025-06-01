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

void deleteFiles(IDeletionStrategy& strategy, PathsVec& files);

bool deleteInteractively(IDeletionStrategy& strategy,
                         PathsVec& files,
                         PathsSet& ignoredFiles,
                         std::ostream& out,
                         std::istream& in);

void deleteDuplicates(const Config& cfg, const DuplicateDetector& detector,
                      std::ostream& out, std::istream& in);

} // namespace tools::dups