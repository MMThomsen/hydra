#ifndef __PRED_H__
#define __PRED_H__

#include "dom.h"

#include <string>
#include <variant>
#include <vector>
#include <unordered_map>
#include <utility>
#include <ostream>
#include <stdexcept>
#include <optional>


struct Term {
    using VarT   = std::string;
    using ConstT = Dom;

    using t = std::variant<VarT, ConstT>;

    Term();
    static Term Var(std::string v);
    static Term Const(ConstT v);

    static bool         isVar(const Term& t)    noexcept;
    static bool         isConst(const Term& t)  noexcept;

    static VarT         unvar(const Term& t);
    static ConstT       unconst(const Term& t);

    static std::vector<std::string> fv_list(const std::vector<Term>& xs);

    static bool         equal(const Term& a, const Term& b);

    static std::string  to_string(const Term& t);
    static std::string  value_to_string(const Term& t);
    static std::string  list_to_string(const std::vector<Term>& trms);
    static std::string  list_to_json_string(const std::vector<Term>& trms);


    static std::optional<std::unordered_map<std::string, Dom>> match_terms(const std::vector<Term>& trms, 
                                                                           const std::vector<Dom>& ds,
                                                                           const std::unordered_map<std::string, Dom>& map);


    friend bool operator==(const Term& a, const Term& b) noexcept {return Term::equal(a, b); }
    friend bool operator!=(const Term& a, const Term& b) noexcept {return !Term::equal(a, b);}
    friend bool operator<(const Term& a, const Term& b) noexcept {
        const bool a_is_var = Term::isVar(a);
        const bool b_is_var = Term::isVar(b);

        if (a_is_var != b_is_var) return a_is_var;

        if (a_is_var) {
            return std::get<VarT>(a.v_) < std::get<VarT>(b.v_);
        } else {
            return Dom::compare_t(std::get<ConstT>(a.v_), std::get<ConstT>(b.v_)) < 0;
        }
    }

    struct Less {
        bool operator()(const Term& a, const Term& b) const noexcept { return a < b; }
    };


    friend std::ostream& operator<<(std::ostream& os, const Term& t) {
        return os << Term::to_string(t);
    }

private:
    explicit Term(t v) : v_(std::move(v)) {} 
    t v_;    

};

struct Sig {
    struct Props {
        int arity{};
        std::vector<std::pair<std::string, Dom::tt>> ntconsts;
      };

    static std::unordered_map<std::string, Props> table;

    static void add(const std::string& name,
        const std::vector<std::pair<std::string, Dom::tt>>& ntconsts);

    static std::vector<std::string> vars(const std::string& name);

    static void print_table();

    
};

std::vector<Term> check_terms(const std::string& p_name, 
    const std::vector<Term>& trms);

#endif // __PRED_H__