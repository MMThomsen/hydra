#include "trie.h"
#include "pred.h"

#include <cstdlib>

Trie trie;

int Trie::getOrAdd(const char *s)
{
    TrieNode<int> *cur = &root;

    while (*s != 0) {
        if (cur->next[*s] == NULL) {
            cur->next[*s] = new TrieNode<int>(-1);
        }
        cur = cur->next[*s++];
    }

    if (cur->value == -1) {
        cur->value = cnt++;
    }

    return cur->value;
}

std::vector<Term>* Trie::addVars(std::vector<Term> *args)
{
    if (args != nullptr) {
        for (const auto& term : *args) {
            if (Term::isVar(term)) {
                std::string var_name = Term::unvar(term);
                // Add only if not already present
                if (std::find(vars.begin(), vars.end(), var_name) == vars.end()) {
                    vars.push_back(var_name);
                }
            }
        }
    }
    return args;
}
