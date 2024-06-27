#pragma once

#include "MsgHandler.h"
#include <kidmon/repo/Repository.h>

// * Entry          // full representation of the event
// * Statistics     // statistical data about multiple entries (part of report)
//
// * Report         // summary of the collection of an event based on a certain criteria
// * ReportBuilder  // interface for producing a report
//
// * IRepository
//      * DatabaseRepository
//      * FileSystemRepository
//      * InMemoryRepository


class DataHandler : public MsgHandler
{
    IRepository& repo_;
    std::string buffer_;

public:
    explicit DataHandler(IRepository& repo);

    bool handle(const nlohmann::json& payload,
                nlohmann::json& answer,
                std::string& error) override;
};
