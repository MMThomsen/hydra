#include "pred.h"
#include <sstream>
#include <iostream>


// ===== Term =====


Term::Term() = default;

Term Term::Var(std::string v) {
    return Term(std::move(v));
}

Term Term::Const(ConstT v) {
    return Term(std::move(v));
}

bool Term::isVar(const Term& t)   noexcept { return std::holds_alternative<VarT>(t.v_); }
bool Term::isConst(const Term& t) noexcept { return std::holds_alternative<ConstT>(t.v_); }

Term::VarT Term::unvar(const Term& t) {
    if (!isVar(t)) throw std::invalid_argument("unvar is undefined for Consts");
    return std::get<VarT>(t.v_);
    
}

Term::ConstT Term::unconst(const Term& t) {
    if (!isConst(t)) throw std::invalid_argument("unconst is undefined for Vars");
    return std::get<ConstT>(t.v_);
}

std::vector<std::string> Term::fv_list(const std::vector<Term>& xs) {
    std::vector<std::string> out;
    out.reserve(xs.size());
    for (const auto& t : xs) {
        if (isVar(t)) out.push_back(std::get<VarT>(t.v_));
    }
    return out;
}

bool Term::equal(const Term& a, const Term& b)  {
    if (isVar(a) && isVar(b)) {
        return std::get<VarT>(a.v_) == std::get<VarT>(b.v_);
    }

    if (isConst(a) && isConst(b)) {
        return Dom::equal(std::get<ConstT>(a.v_), std::get<ConstT>(b.v_));
    }
    return false;
}

std::string Term::to_string(const Term& t) {
    if (isVar(t)) {
        return std::string("Var ") + std::get<VarT>(t.v_);
    } else {
        return std::string("Const ") + Dom::to_string(std::get<ConstT>(t.v_));
    }
}

std::string Term::value_to_string(const Term& t) {
    if (isVar(t)) {
        return std::get<VarT>(t.v_);
    } else {
        return Dom::to_string(std::get<ConstT>(t.v_));
    }
}

std::string Term::list_to_string(const std::vector<Term>& trms) {
    if (trms.empty()) return "[]";
    std::ostringstream oss;
    for (std::size_t i = 0; i < trms.size(); ++i) {
        if (i) oss << ", ";
        oss << Term::value_to_string(trms[i]);
    }
    return oss.str();
}

std::string Term::list_to_json_string(const std::vector<Term>& trms) {
    if (trms.empty()) return "";
    std::ostringstream oss;
    for (std::size_t i = 0; i < trms.size(); ++i) {
        if (i) oss << ", ";

        if (Term::isConst(trms[i])) {
            oss << "<" << Term::value_to_string(trms[i]) << ">";
        } else {
            oss << Term::value_to_string(trms[i]);
        }
    }
    return oss.str();
}

// ===== Sig =====

std::unordered_map<std::string, Sig::Props> Sig::table{};

void Sig::add(const std::string& p_name, const std::vector<std::pair<std::string, Dom::tt>>& ntconsts){
    Props ps;
    ps.arity    = static_cast<int>(ntconsts.size());
    ps.ntconsts = ntconsts;

    auto [it, inserted] = table.emplace(p_name, std::move(ps));
    if (!inserted) {
        throw std::invalid_argument("predicate already exists: " + p_name);
    }
}

std::vector<std::string> Sig::vars(const std::string& name) {
    auto it = table.find(name);
    if (it == table.end()) {
        throw std::invalid_argument("predicate not found: " + name);
    }

    std::vector<std::string> out;
    out.reserve(it->second.ntconsts.size());
    for (const auto& p : it->second.ntconsts) {
        out.push_back(p.first);
    }
    return out;
}

void Sig::print_table() {
    for (const auto& [n, ps] : table) {
        std::cout << n << "(";
        for (std::size_t i = 0; i < ps.ntconsts.size(); ++i) {
            const auto& [var, tt] = ps.ntconsts[i];
            if (i) std::cout << ",";
            std::cout << var << ":" << Dom::tt_to_string(tt);
        }
        std::cout << ")\n";
    }
}

// ===== Pred =====

std::vector<Term>   check_terms(const std::string& p_name, 
                                const std::vector<Term>& trms) {
    // look up p_name in Sig::table and and retrieve element
    auto it = Sig::table.find(p_name);

    // p_name not found
    if (it == Sig::table.end()) {
        throw std::invalid_argument("predicate not found: " + p_name);
    }
    
    // binding props for p_name
    const auto& props = it->second;

    // check arity
    if (static_cast<int>(trms.size()) != props.arity) {
        throw std::invalid_argument(
            "arity of " + p_name + " is " + std::to_string(props.arity));
    }

    // Vars are always okay, but Const must match declared Dom::tt
    for (std::size_t i = 0; i < trms.size(); ++i) {
        const Term& t = trms[i];
        const auto& expected_tt = props.ntconsts[i].second;
        if (Term::isConst(t)) {
            auto d = Term::unconst(t);
            if (!Dom::tt_equal(Dom::tt_of_domain(d), expected_tt)) {
                throw std::invalid_argument(
                    "type of terms of " + p_name + " do not match the signature");
            }
        }
    }

    return trms;

}