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
    auto entry = std::make_unique<MenuEntry>(title, std::move(matcher), std::move(action));
    children_.push_back(std::move(entry));
}


Navigation Renderer::run(Menu& m, bool isChild)
{
    while (true)
    {
        renderEntries(m, isChild);

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

            if (result == Navigation::Quit)
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

const std::string& Renderer::currentPrompt() const noexcept
{
    return prompt_;
}


StreamRenderer::StreamRenderer(std::ostream& out, std::istream& in)
    : out_(out)
    , in_(in)
{
}

void StreamRenderer::renderEntries(const Menu& m, bool isChild)
{
    out_ << std::format("========= {} =========\n", m.getTitle());
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

void StreamRenderer::invalidInput()
{
    out_ << "Invalid input.\n";
}

std::string StreamRenderer::prompt()
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
