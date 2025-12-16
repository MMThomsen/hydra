#ifndef __UTIL_H__
#define __UTIL_H__

#include "util.h"
#include "pdt.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define CHECK(x) assert(x);

typedef int32_t timestamp;
typedef int32_t timestamp_delta;
const timestamp MAX_TIMESTAMP = 0x7FFFFFFF;

enum Boolean {
    FALSE, TRUE
};

Pdt::PdtT<Boolean> BooleanNot(const Pdt::PdtT<Boolean>& b, const std::vector<std::string>& vars);
Pdt::PdtT<Boolean> BooleanAnd(const Pdt::PdtT<Boolean>& b1, const Pdt::PdtT<Boolean>& b2, const std::vector<std::string>& vars);
Pdt::PdtT<Boolean> BooleanOr(const Pdt::PdtT<Boolean>& b1, const Pdt::PdtT<Boolean>& b2, const std::vector<std::string>& vars);
bool has_true_leaf(const Pdt::PdtT<Boolean>& pdt);

struct BoolVerdict {
    timestamp ts;
    bool b;

    BoolVerdict() {}
    BoolVerdict(timestamp ts, bool b) : ts(ts), b(b) {}
    bool operator==(const BoolVerdict &bv) const {
        return ts == bv.ts && b == bv.b;
    }
};

struct BooleanVerdict {
    timestamp ts;
    Pdt::PdtT<Boolean> b;
    std::vector<std::string> vars;

    BooleanVerdict(timestamp ts, Pdt::PdtT<Boolean> b, const std::vector<std::string>& vars) : ts(ts), b(b), vars(vars) {}
    bool operator==(const BooleanVerdict &bv) const {
        if (ts != bv.ts) return false;
        
        auto eq_fn = [](Boolean v1, Boolean v2) -> Boolean {
            return (v1 == v2) ? TRUE : FALSE;
        };
        Pdt::PdtT<Boolean> comparison = Pdt::apply2<Boolean, Boolean, Boolean>(vars, eq_fn, this->b, bv.b);
        
        std::function<bool(const Pdt::PdtT<Boolean>&)> has_true;
        has_true = [&](const Pdt::PdtT<Boolean>& pdt) -> bool {
            if (Pdt::isleaf(pdt)) {
                return Pdt::unleaf(pdt) == TRUE;
            } else if (Pdt::isnode(pdt)) {
                const auto& part = Pdt::part(pdt);
                for (const auto& [sub, sub_pdt] : part) {
                    if (Setc::isComplement(sub)) continue;
                    if (has_true(sub_pdt)) return true;
                }
                return false;
            }
            return false;
        };
        
        return has_true(comparison);
    }
    BooleanVerdict operator!() const {
        return BooleanVerdict(this->ts, BooleanNot(this->b, vars), vars);
    }
    BooleanVerdict operator&&(const BooleanVerdict &w) const {
        CHECK(this->ts == w.ts);
        return BooleanVerdict(this->ts, BooleanAnd(this->b, w.b, vars), vars);
    }
    BooleanVerdict operator||(const BooleanVerdict &w) const {
        CHECK(this->ts == w.ts);
        return BooleanVerdict(this->ts, BooleanOr(this->b, w.b, vars), vars);
    }
};

int parseNumber(const char *s, size_t *pos, timestamp *n);

FILE *open_file_type(const char *prefix, const char *ftype, const char *mode);

#endif /* __UTIL_H__ */
