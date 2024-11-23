#include "Conditions.h"
#include <core/utils/Str.h>
#include <core/utils/File.h>

#include <fmt/format.h>

void TrueCondition::write(std::ostream& os) const
{
    os << "true";
}

bool TrueCondition::met(const Entry&) const
{
    return true;
}

void FalseCondition ::write(std::ostream& os) const
{
    os << "false";
}

bool FalseCondition::met(const Entry&) const
{
    return false;
}


UnaryCondition::UnaryCondition(ConditionPtr cond)
    : cond_ {std::move(cond)}
{
}

const ConditionPtr& UnaryCondition::condition() const
{
    return cond_;
}

ConditionPtr& UnaryCondition::condition()
{
    return cond_;
}

void UnaryCondition::write(std::ostream& os) const
{
    os << name() << '(';
    condition()->write(os);
    os << ")";
}


BinaryCondition::BinaryCondition(ConditionPtr lhs, ConditionPtr rhs)
    : lhs_(std::move(lhs))
    , rhs_(std::move(rhs))
{
    if (!lhs_ || !rhs_)
    {
        throw std::runtime_error("Condition can't be null");
    }
}

const ConditionPtr& BinaryCondition::lhs() const
{
    return lhs_;
}

const ConditionPtr& BinaryCondition::rhs() const
{
    return rhs_;
}

ConditionPtr& BinaryCondition::lhs()
{
    return lhs_;
}

ConditionPtr& BinaryCondition::rhs()
{
    return rhs_;
}

void BinaryCondition::write(std::ostream& os) const
{
    os << '(';
    lhs()->write(os);
    os << ") " << name() << " (";
    rhs()->write(os);
    os << ')';
}


LogicalAND::LogicalAND(ConditionPtr lhs, ConditionPtr rhs)
    : BinaryCondition(std::move(lhs), std::move(rhs))
{
}

std::string_view LogicalAND::name() const noexcept
{
    return "&&";
}

bool LogicalAND::met(const Entry& entry) const
{
    return lhs()->met(entry) && rhs()->met(entry);
}


LogicalOR::LogicalOR(ConditionPtr lhs, ConditionPtr rhs)
    : BinaryCondition(std::move(lhs), std::move(rhs))
{
}

std::string_view LogicalOR::name() const noexcept
{
    return "||";
}

bool LogicalOR::met(const Entry& entry) const
{
    return lhs()->met(entry) || rhs()->met(entry);
}


Negate::Negate(ConditionPtr cond)
    : UnaryCondition(std::move(cond))
{
}

std::string_view Negate::name() const noexcept
{
    return "!";
}

bool Negate::met(const Entry& entry) const
{
    return !condition()->met(entry);
}


const std::string& StringCondition::value(const Entry& entry) const
{
    fetchValue(entry, buffer_);
    return buffer_;
}

StringCondition::StringCondition(std::string needle, std::string attributeName)
    : needle_(std::move(needle))
    , attributeName_(std::move(attributeName))
{
}

const std::string& StringCondition::needle() const noexcept
{
    return needle_;
}

const std::string& StringCondition::attributeName() const noexcept
{
    return attributeName_;
}


void IsStringCondition::write(std::ostream& os) const
{
    os << fmt::format("{} is '{}'", attributeName(), needle());
}

bool IsStringCondition::met(const Entry& entry) const
{
    return value(entry) == needle();
}

void HasStringCondition::write(std::ostream& os) const
{
    os << fmt::format("{} has '{}'", attributeName(), needle());
}

bool HasStringCondition::met(const Entry& entry) const
{
    return value(entry).find(needle()) != std::string::npos;
}

void HasProcessCondition::fetchValue(const Entry& entry, std::string& value) const
{
    value = file::path2s(entry.processInfo.processPath);
}

HasProcessCondition::HasProcessCondition(std::string processName)
    : HasStringCondition(std::move(processName), "process")
{
}

void HasTitleCondition::fetchValue(const Entry& entry, std::string& value) const
{
    value = entry.windowInfo.title;
}

HasTitleCondition::HasTitleCondition(std::string title)
    : HasStringCondition(std::move(title), "title")
{
}
