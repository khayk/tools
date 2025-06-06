#include <duplicates/Utils.h>
#include <duplicates/Node.h>
#include <core/utils/File.h>

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

void outputTree(const Node* root, std::ostream& os)
{
    root->enumNodes([&os](const Node* node) {
        if (node->depth() == 0)
        {
            return;
        }

        os << std::string(1UL * (node->depth() - 1), ' ');
        os << file::path2s(node->name())
           << (node->leaf() || node->depth() == 1 ? "" : "/") << '\n';
    });
}

} // namespace tools::dups::util
