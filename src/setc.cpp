#include "setc.h"
#include <sstream>
#include <stdexcept>
#include <set>
#include <algorithm>
#include <iterator>

static Setc::SetT set_inter(const Setc::SetT& a, const Setc::SetT& b) {
    Setc::SetT out(a.key_comp());
    std::set_intersection(
        a.begin(), a.end(),
        b.begin(), b.end(),
        std::inserter(out, out.end()),
        a.key_comp()
    );
    return out;
}

static Setc::SetT set_union(const Setc::SetT& a, const Setc::SetT& b) {
    Setc::SetT out(a.key_comp());
    std::set_union(
        a.begin(), a.end(),
        b.begin(), b.end(),
        std::inserter(out, out.end()),
        a.key_comp()
    );
    return out;
}

static Setc::SetT set_diff(const Setc::SetT& a, const Setc::SetT& b) {
    Setc::SetT out(a.key_comp());
    std::set_difference(
        a.begin(), a.end(),
        b.begin(), b.end(),
        std::inserter(out, out.end()),
        a.key_comp()
    );
    return out;
}


Setc::Setc() : type_(sType::Finite), s_() {}

Setc Setc::Finite(SetT s) { return Setc{ sType::Finite, std::move(s) }; }

Setc Setc::Complement(SetT s) { return Setc{ sType::Complement, std::move(s) }; }

Setc Setc::univ() { return Complement(SetT{}); }


bool Setc::isFinite(const Setc& x) noexcept { return x.type_ == sType::Finite; }

bool Setc::isComplement(const Setc& x) noexcept { return x.type_ == sType::Complement; }

bool Setc::isEmpty(const Setc& x) noexcept { return x.type_ == sType::Finite && x.s_.empty(); }

int Setc::length(const Setc& x) noexcept { return static_cast<int>(x.s_.size()); }

bool Setc::equal(const Setc& a, const Setc& b) noexcept {
    return a.type_ == b.type_ && a.s_ == b.s_;
}

Setc Setc::add(const Setc& x, const Dom& v) {
    SetT s = x.s_;
    if (x.type_ == sType::Finite) {
        s.insert(v);
        return Finite(std::move(s));
    } else {
        
        s.erase(v);
        return Complement(std::move(s));
    }
}


Setc Setc::inter(const Setc& a, const Setc& b) {
    using T = sType;
    if (a.type_ == T::Finite && b.type_ == T::Finite) {
        return Finite(set_inter(a.s_, b.s_));
    } else if (a.type_ == T::Finite && b.type_ == T::Complement) {
        return Finite(set_diff(a.s_, b.s_));      
    } else if (a.type_ == T::Complement && b.type_ == T::Finite) {
        return Finite(set_diff(b.s_, a.s_));      
    } else {
        return Complement(set_union(a.s_, b.s_)); 
    }
}

Setc Setc::union_(const Setc& a, const Setc& b) {
    using T = sType;
    if (a.type_ == T::Finite && b.type_ == T::Finite) {
        return Finite(set_union(a.s_, b.s_));
    } else if (a.type_ == T::Finite && b.type_ == T::Complement) {
        return Complement(set_diff(b.s_, a.s_));  
    } else if (a.type_ == T::Complement && b.type_ == T::Finite) {
        return Complement(set_diff(a.s_, b.s_));  
    } else {
        return Complement(set_inter(a.s_, b.s_)); 
    }
}

Setc Setc::diff(const Setc& a, const Setc& b) {
    using T = sType;
    if (a.type_ == T::Finite && b.type_ == T::Finite) {
        return Finite(set_diff(a.s_, b.s_));
    } else if (a.type_ == T::Finite && b.type_ == T::Complement) {
        return Finite(set_inter(a.s_, b.s_));     
    } else if (a.type_ == T::Complement && b.type_ == T::Finite) {
        return Complement(set_union(a.s_, b.s_)); 
    } else {
        return Finite(set_diff(b.s_, a.s_));     
    }
}


std::optional<Dom> Setc::min_elt(const Setc& x) {
    if (x.type_ != sType::Finite || x.s_.empty()) return std::nullopt;
    return *x.s_.begin();
}

static Dom pick_int(const Setc::SetT& excl) {
    if (!excl.count(Dom::Int(0))) return Dom::Int(0);
    for (int k=1;;++k){ if(!excl.count(Dom::Int(k)))return Dom::Int(k);
                        if(!excl.count(Dom::Int(-k)))return Dom::Int(-k); }
}
static Dom pick_float(const Setc::SetT& excl) {
    if (!excl.count(Dom::Float(0.0))) return Dom::Float(0.0);
    for (int k=1;;++k){ if(!excl.count(Dom::Float(double(k)))) return Dom::Float(double(k));
                        if(!excl.count(Dom::Float(double(-k))))return Dom::Float(double(-k)); }
}
static Dom pick_str(const Setc::SetT& excl) {
    if (!excl.count(Dom::Str(std::string{}))) return Dom::Str(std::string{});
    for (char c='a'; c<='z'; ++c) {
        std::string s(1,c);
        if (!excl.count(Dom::Str(s))) return Dom::Str(s);
    }
    for (int n=0;;++n) {
        auto s = std::to_string(n);
        if (!excl.count(Dom::Str(s))) return Dom::Str(s);
    }
}

Dom Setc::some_elt(Dom::tt ty, const Setc& x) {
    if (x.type_ == sType::Finite) {
        if (x.s_.empty()) throw std::invalid_argument("Setc::some_elt: empty finite set");
        return *x.s_.begin();
    }
    const auto& excl = x.s_; 
    switch (ty) {
        case Dom::tt::TInt:   return pick_int(excl);
        case Dom::tt::TFloat: return pick_float(excl);
        case Dom::tt::TStr:   return pick_str(excl);
    }
    return Dom::Int(0); 
}

// ===== conversions / pretty =====
std::vector<Dom> Setc::to_list(const Setc& x) {
    if (x.type_ != sType::Finite)
        throw std::invalid_argument("Setc::to_list: undefined for complemented (infinite) sets");
    return std::vector<Dom>(x.s_.begin(), x.s_.end());
}

static std::string json_escape(const std::string& s) {
    std::string out; out.reserve(s.size());
    for (char c: s) {
        switch(c){
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

static std::string join_dom_set(const Setc::SetT& s) {
    std::ostringstream oss;
    bool first=true;
    for (const auto& d : s) {
        if (!first) oss << ", ";
        first=false;
        oss << Dom::to_string(d);
    }
    return oss.str();
}

std::string Setc::to_json(const Setc& x) {
    if (x.type_ == sType::Finite) {
        std::ostringstream oss;
        oss << "[";
        bool first=true;
        for (const auto& d : x.s_) {
            if(!first) oss << ", ";
            first=false;
            oss << "\"" << json_escape(Dom::to_string(d)) << "\"";
        }
        oss << "]";
        return oss.str();
    } else {
        std::ostringstream oss;
        oss << "{\"complement_of\": [";
        bool first=true;
        for (const auto& d : x.s_) {
            if(!first) oss << ", ";
            first=false;
            oss << "\"" << json_escape(Dom::to_string(d)) << "\"";
        }
        oss << "]}";
        return oss.str();
    }
}

std::string Setc::to_string(const Setc& x) {
    if (x.type_ == sType::Finite) {
        return "{" + join_dom_set(x.s_) + "}";
    } else {
        return "Complement of {" + join_dom_set(x.s_) + "}";
    }
}

std::string Setc::to_latex(const Setc& x) {
    if (x.type_ == sType::Finite) {
        return "\\{" + join_dom_set(x.s_) + "\\}";
    } else {
        return "\\{" + join_dom_set(x.s_) + "\\}^\\cp";
    }
}