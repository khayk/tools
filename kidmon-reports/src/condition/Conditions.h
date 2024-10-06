#pragma once 

#include "ICondition.h"
#include <kidmon/data/Types.h>

class BinaryCondition : public ICondition
{
    ConditionPtr lhs_;
    ConditionPtr rhs_;

public:
    BinaryCondition(ConditionPtr lhs, ConditionPtr rhs);

    const ConditionPtr& lhs() const;
    const ConditionPtr& rhs() const;

    ConditionPtr& lhs();
    ConditionPtr& rhs();

    void write(std::ostream& os) const override;

private:
    virtual std::string_view name() const noexcept = 0;
};


class LogicalAND : public BinaryCondition
{
public:
    LogicalAND(ConditionPtr lhs, ConditionPtr rhs);

    std::string_view name() const noexcept override;

    bool met(const Entry& entry) const override;
};


class LogicalOR : public BinaryCondition
{
public:
    LogicalOR(ConditionPtr lhs, ConditionPtr rhs);

    std::string_view name() const noexcept override;

    bool met(const Entry& entry) const override;
};


class StringCondition : public ICondition
{
private:
    std::string needle_;
    std::string attributeName_;

    // We need this buffer to slightly improve performance
    mutable std::string buffer_;
    virtual void fetchValue(const Entry& entry, std::string& value) const = 0;

protected:
    const std::string& value(const Entry& entry) const;

public:
    StringCondition(std::string needle, std::string attributeName);

    const std::string& needle() const noexcept;
    const std::string& attributeName() const noexcept;
};


class IsStringCondition : public StringCondition
{
public:
    using StringCondition::StringCondition;

    void write(std::ostream& os) const override;
    bool met(const Entry& entry) const override;
};


class HasStringCondition : public StringCondition
{
public:
    using StringCondition::StringCondition;

    void write(std::ostream& os) const override;
    bool met(const Entry& entry) const override;
};


class HasProcessCondition : public HasStringCondition
{
    void fetchValue(const Entry& entry, std::string& value) const override;
public:
    HasProcessCondition(std::string processName);
};


class HasTitleCondition : public HasStringCondition
{
    void fetchValue(const Entry& entry, std::string& value) const override;
public:
    HasTitleCondition(std::string title);    
};
