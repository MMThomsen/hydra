#include "util.h"

#include <cstdlib>

Pdt::PdtT<Boolean> BooleanNot(const Pdt::PdtT<Boolean>& b, const std::vector<std::string>& vars) {
    auto negate = [](Boolean val) -> Boolean { 
        return (val == TRUE) ? FALSE : TRUE; 
    };
    return Pdt::apply1<Boolean, Boolean>(vars, negate, b);
}
Pdt::PdtT<Boolean> BooleanAnd(const Pdt::PdtT<Boolean>& b1, const Pdt::PdtT<Boolean>& b2, const std::vector<std::string>& vars) {
    auto do_and = [](Boolean v1, Boolean v2) -> Boolean {
        if (v1 == FALSE || v2 == FALSE) return FALSE;
            else return TRUE;
        };
    return Pdt::apply2<Boolean, Boolean, Boolean>(vars, do_and, b1, b2);
    }
    
Pdt::PdtT<Boolean> BooleanOr(const Pdt::PdtT<Boolean>& b1, const Pdt::PdtT<Boolean>& b2, const std::vector<std::string>& vars) {
    auto do_or = [](Boolean v1, Boolean v2) -> Boolean {
        if (v1 == TRUE || v2 == TRUE) return TRUE;
        else return FALSE;
    };
    return Pdt::apply2<Boolean, Boolean, Boolean>(vars, do_or, b1, b2);
}

bool has_true_leaf(const Pdt::PdtT<Boolean>& pdt) {
    std::function<bool(const Pdt::PdtT<Boolean>&)> check;
    check = [&](const Pdt::PdtT<Boolean>& p) -> bool {
        if (Pdt::isleaf(p)) {
            return Pdt::unleaf(p) == TRUE;
        } else if (Pdt::isnode(p)) {
            const auto& part = Pdt::part(p);
            for (const auto& [sub, sub_pdt] : part) {
                if (check(sub_pdt)) return true;
            }
            return false;
        }
        return false;
    };
    return check(pdt);
}

int parseNumber(const char *s, size_t *pos, timestamp *n)
{
    int i = (pos == NULL ? 0 : *pos);
    timestamp ans = 0;
    if (!('0' <= s[i] && s[i] <= '9')) return 1;
    while ('0' <= s[i] && s[i] <= '9') {
        int d = s[i++] - '0';
        if (ans > MAX_TIMESTAMP / 10) return 1;
        ans *= 10;
        if (ans >= MAX_TIMESTAMP - d) return 1;
        ans += d;
    }
    if (pos != NULL) *pos = i;
    *n = ans;
    return 0;
}

FILE *open_file_type(const char *prefix, const char *ftype, const char *mode) {
    char *file_name = new char[strlen(prefix) + strlen(ftype) + 1];
    strcpy(file_name, prefix);
    strcat(file_name, ftype);

    FILE *f = fopen(file_name, mode);
    delete [] file_name;

    return f;
}
