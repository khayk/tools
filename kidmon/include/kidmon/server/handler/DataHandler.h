#pragma once

#include "MsgHandler.h"
#include <kidmon/data/Types.h>

#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

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

class Filter {
public:
    explicit Filter(std::string username,
                    TimePoint dateFrom = TimePoint::min(),
                    TimePoint dateTo = TimePoint::max());

private:
    std::string username_;
    TimePoint dateTimeFrom_;
    TimePoint datetimeTo_;
};


class IRepository
{
public:
    using UserCb = std::function<void(const std::string&)>;
    using EntryCb = std::function<void(const Entry&)>;

    virtual ~IRepository() = default;

    virtual void add(const Entry& entry) = 0;

    virtual void queryUsers(UserCb cb) const = 0;

    virtual void queryEntries(const Filter& filter, EntryCb cb) const = 0;
};


class FileSystemRepository : public IRepository
{
public:
    explicit FileSystemRepository(fs::path reportsDir);
    ~FileSystemRepository() override;

    void add(const Entry& entry) override;

    void queryUsers(UserCb cb) const override;

    void queryEntries(const Filter& filter, EntryCb cb) const override;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};


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
