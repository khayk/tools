#pragma once

#include "ICondition.h"
#include <kidmon/data/Types.h>

class TrueCondition : public ICondition
{
public:
    void write(std::ostream& os) const override;
    bool met(const km::Entry& entry) const override;
};

class FalseCondition : public ICondition
{
public:
    void write(std::ostream& os) const override;
    bool met(const km::Entry& entry) const override;
};

class UnaryCondition : public ICondition
{
public:
    UnaryCondition(ConditionPtr cond);

    const ConditionPtr& condition() const;
    ConditionPtr& condition();

    void write(std::ostream& os) const override;

private:
    virtual std::string_view name() const noexcept = 0;

    ConditionPtr cond_;
};


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

    bool met(const km::Entry& entry) const override;
};


class LogicalOR : public BinaryCondition
{
public:
    LogicalOR(ConditionPtr lhs, ConditionPtr rhs);

    std::string_view name() const noexcept override;

    bool met(const km::Entry& entry) const override;
};


class Negate : public UnaryCondition
{
public:
    Negate(ConditionPtr cond);

    std::string_view name() const noexcept override;

    bool met(const km::Entry& entry) const override;
};


class StringCondition : public ICondition
{
private:
    std::string needle_;
    std::string attributeName_;
    bool caseSensitive_;

    // We need these buffers to slightly improve performance
    mutable std::string buffer_;
    mutable std::wstring wbuffer_;
    virtual void fetchValue(const km::Entry& entry, std::string& value) const = 0;

protected:
    // Lowercased when matching is case-insensitive; the entry itself is untouched
    const std::string& value(const km::Entry& entry) const;

public:
    /**
     * When matching is case-insensitive, the needle is expected to be lowercase
     */
    StringCondition(std::string needle, std::string attributeName, bool caseSensitive);

    const std::string& needle() const noexcept;
    const std::string& attributeName() const noexcept;
};


class IsStringCondition : public StringCondition
{
public:
    using StringCondition::StringCondition;

    void write(std::ostream& os) const override;
    bool met(const km::Entry& entry) const override;
};


class HasStringCondition : public StringCondition
{
public:
    using StringCondition::StringCondition;

    void write(std::ostream& os) const override;
    bool met(const km::Entry& entry) const override;
};


/**
 * Matches the executable name, or the full path if the needle contains a path
 * separator
 */
class HasProcessCondition : public HasStringCondition
{
    bool matchPath_;

    void fetchValue(const km::Entry& entry, std::string& value) const override;

public:
    HasProcessCondition(const std::string& process, bool caseSensitive = true);
};


class HasTitleCondition : public HasStringCondition
{
    void fetchValue(const km::Entry& entry, std::string& value) const override;

public:
    HasTitleCondition(std::string title, bool caseSensitive = true);
};
