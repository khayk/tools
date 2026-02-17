#include <cstdint>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <iosfwd>

namespace tools::dups {

enum class Navigation : uint8_t
{
    Continue,
    Back,
    Quit
};

class Renderer;

using Matcher = std::function<bool(const std::string&)>;
using Action = std::function<Navigation(Renderer&)>;

class MenuEntry
{
    std::string title_;
    Matcher matcher_;
    Action action_;

public:
    MenuEntry(std::string_view title, Matcher matcher, Action action);

    const std::string& getTitle() const noexcept;
    const Matcher& matcher() const noexcept;
    Action& action() noexcept;
};

using MenuPtr = std::unique_ptr<MenuEntry>;


class Menu
{
    using Children = std::vector<MenuPtr>;
    Children children_;
    std::string title_;

public:
    Menu(std::string_view title);

    const std::string& getTitle() const noexcept;
    Children& children() noexcept;
    const Children& children() const noexcept;

    void add(std::string_view title, Matcher matcher, Action action);
};


class Renderer
{
    std::string prompt_;

public:
    Renderer() = default;
    virtual ~Renderer() = default;

    virtual Navigation run(Menu& m, bool isChild = true);

    const std::string& currentPrompt() const noexcept;

private:
    virtual void renderEntries(const Menu& m, bool isChild) = 0;
    virtual std::string prompt() = 0;
    virtual void invalidInput() = 0;
};


class StreamRenderer : public Renderer
{
    std::string prevInput_;
    std::ostream& out_;
    std::istream& in_;

public:
    StreamRenderer(std::ostream& out, std::istream& in);

protected:
    void renderEntries(const Menu& m, bool isChild) override;
    void invalidInput() override;
    std::string prompt() override;
};


struct Matchers
{
    static Matcher Range(int min, int max);
    static Matcher Key(char c);
};

} // namespace tools::dups
