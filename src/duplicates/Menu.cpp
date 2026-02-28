#include <core/utils/Number.h>
#include <duplicates/Menu.h>
#include <format>
#include <ostream>
#include <istream>

namespace tools::dups {

MenuEntry::MenuEntry(std::string_view title, Matcher matcher, Action action)
    : title_(title)
    , matcher_(std::move(matcher))
    , action_(std::move(action))
{
}

const std::string& MenuEntry::getTitle() const noexcept
{
    return title_;
}

const Matcher& MenuEntry::matcher() const noexcept
{
    return matcher_;
}

Action& MenuEntry::action() noexcept
{
    return action_;
}

Menu::Menu(std::string_view title)
    : title_(title)
{
}

const std::string& Menu::getTitle() const noexcept
{
    return title_;
}

Menu::Children& Menu::children() noexcept
{
    return children_;
}

const Menu::Children& Menu::children() const noexcept
{
    return children_;
}

void Menu::add(std::string_view title, Matcher matcher, Action action)
{
    children_.push_back(
        std::make_unique<MenuEntry>(title, std::move(matcher), std::move(action)));
}


Navigation UserIO::run(Menu& m, bool isChild)
{
    while (true)
    {
        printOptions(m, isChild);
        prompt_ = prompt();

        if (prompt_.empty())
        {
            return Navigation::Quit;
        }

        if (isChild && (prompt_ == "b" || prompt_ == "B"))
        {
            return Navigation::Back;
        }

        if (prompt_ == "q" || prompt_ == "Q")
        {
            return Navigation::Quit;
        }

        bool handled = false;
        for (auto& child : m.children())
        {
            Navigation result = Navigation::Continue;

            if (child->matcher()(prompt_))
            {
                result = child->action()(*this);
                handled = true;
            }

            if (result == Navigation::Quit || result == Navigation::Done)
            {
                return result;
            }
        }

        if (!handled)
        {
            invalidInput();
        }
    }

    return Navigation::Continue;
}

void UserIO::printText(std::string_view text)
{
    std::ignore = text;
}

const std::string& UserIO::currentPrompt() const noexcept
{
    return prompt_;
}


StreamIO::StreamIO(std::ostream& out, std::istream& in)
    : out_(out)
    , in_(in)
{
}

void StreamIO::printText(std::string_view text)
{
    out_ << text;
}

void StreamIO::printOptions(const Menu& m, bool isChild)
{
    out_ << std::format("{:-^60s}\n", std::format("> {} <", m.getTitle()));

    for (const auto& child : m.children())
    {
        out_ << std::format("  {}\n", child->getTitle());
    }

    if (isChild)
    {
        out_ << "  [b] Back\n";
    }

    out_ << "  [q] Quit\n";
}

void StreamIO::invalidInput()
{
    out_ << "Invalid input.\n";
}

std::string StreamIO::prompt()
{
    std::string input;

    while (in_ && input.empty())
    {
        out_ << "> ";
        std::getline(in_, input);

        if (in_ && input.empty())
        {
            input = prevInput_;
        }
    }

    if (!input.empty())
    {
        prevInput_ = input;
    }

    return input;
}

Matcher Matchers::Range(int min, int max)
{
    return [min, max](const std::string& s) {
        int def {max + 1};
        auto val = num::s2num(s, def);
        return val >= min && val <= max;
    };
}

Matcher Matchers::Key(char c)
{
    return [c](const std::string& s) {
        return s.length() == 1 && std::tolower(s[0]) == std::tolower(c);
    };
}

} // namespace tools::dups
