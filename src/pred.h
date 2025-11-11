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


struct Term {
    // OCaml payloads
    using VarT   = std::string;
    using ConstT = Dom;


    // OCaml: type t = Var of string | Const of Dom.t
    using t = std::variant<VarT, ConstT>;

    // ---- construction (mirrors OCaml constructors) ----
    Term();
    static Term Var(std::string v); // Why not reference VarT
    static Term Const(ConstT v);

    // predicates / projections
    static bool         isVar(const Term& t)    noexcept;
    static bool         isConst(const Term& t)  noexcept;

    // helpers
    static VarT         unvar(const Term& t);
    static ConstT       unconst(const Term& t);

    // ---- OCaml helpers (static, Dom-style) ----
    static std::vector<std::string> fv_list(const std::vector<Term>& xs);

    static bool         equal(const Term& a, const Term& b);

    static std::string  to_string(const Term& t);
    static std::string  value_to_string(const Term& t);
    static std::string  list_to_string(const std::vector<Term>& trms);
    static std::string  list_to_json_string(const std::vector<Term>& trms);

    //#############################CHECK#################################//

    // equality / ordering (inline, header-only)
    friend bool operator==(const Term& a, const Term& b) noexcept {return Term::equal(a, b); }
    friend bool operator!=(const Term& a, const Term& b) noexcept {return !Term::equal(a, b);}
    friend bool operator<(const Term& a, const Term& b) noexcept {
        const bool a_is_var = Term::isVar(a);
        const bool b_is_var = Term::isVar(b);

        // Mixed-kind: Var < Const
        if (a_is_var != b_is_var) return a_is_var;

        // Same-kind (both Var):
        if (a_is_var) {
            // both Var: lexicographic on variable name
            return std::get<VarT>(a.v_) < std::get<VarT>(b.v_);
        } else {
            // both Const: delegate to Dom’s order
            return Dom::compare_t(std::get<ConstT>(a.v_), std::get<ConstT>(b.v_)) < 0;
        }
    }

    // comparator functor for ordered containers
    struct Less {
        bool operator()(const Term& a, const Term& b) const noexcept { return a < b; }
    };


    // stream output (handy for debugging)
    friend std::ostream& operator<<(std::ostream& os, const Term& t) {
        return os << Term::to_string(t);
    }
    //#############################CHECK#################################//
private:
    explicit Term(t v) : v_(std::move(v)) {}   // used by the factory functions
    t v_;    

};

struct Sig {
    struct Props {
        int arity{};
        std::vector<std::pair<std::string, Dom::tt>> ntconsts;
      };

    static std::unordered_map<std::string, Props> table;

    // ---- operations ----
    static void add(const std::string& name,
        const std::vector<std::pair<std::string, Dom::tt>>& ntconsts);

    static std::vector<std::string> vars(const std::string& name);

    static void print_table();

    
};

// OCaml: check_terms p_name trms
// Throws std::invalid_argument on arity/type errors; returns `trms` unchanged on success.
std::vector<Term> check_terms(const std::string& p_name, 
    const std::vector<Term>& trms);

#endif // __PRED_H__

