#include <duplicates/Utils.h>
#include <duplicates/Node.h>
#include <core/utils/File.h>
#include <ostream>

namespace tools::dups::util {

void outputTree(const Node* root, std::ostream& os)
{
    root->enumNodes([&os](const Node* node) {
        if (node->depth() == 0)
        {
            return;
        }

        os << std::string(1UL * (node->depth() - 1), ' ');
        os << core::file::path2s(node->name())
           << ((node->leaf() || (node->depth() == 1)) ? "" : "/") << '\n';
    });
}

} // namespace tools::dups::util
