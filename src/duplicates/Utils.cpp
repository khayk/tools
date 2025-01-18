#include <duplicates/Utils.h>
#include <duplicates/Node.h>
#include <core/utils/Str.h>

#include <ostream>

namespace tools::dups::util {

uint16_t digits(size_t n)
{
    uint16_t d = 0;

    while (n != 0)
    {
        d++;
        n /= 10;
    }

    return d;
}

void treeDump(const Node* root, std::ostream& os)
{
    root->enumNodes([&os](const Node* node) {
        std::wstring ws(node->name());
        os << std::string(2 * node->depth(), ' ');

        if (!node->leaf())
        {
            os << "- ";
        }
        else
        {
            os << "* ";
        }

        os << str::ws2s(ws) << " (" << node->size() << ")\n";
    });
}

} // namespace tools::dups::util


