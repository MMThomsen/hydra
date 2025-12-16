#include "dom.h"
#include <variant> 
#include <charconv>
#include <sstream>


Dom::Dom() = default;

Dom Dom::Int(int v)         { return Dom(v); }
Dom Dom::Str(std::string v) { return Dom(std::move(v)); }
Dom Dom::Float(double v)    { return Dom(v); }

bool Dom::equal(const Dom& a, const Dom& b) noexcept {
    return a.v_ == b.v_;
}

int Dom::compare_t(const Dom& a, const Dom& b) noexcept {
    if (a.v_ == b.v_) return 0;

    if (std::holds_alternative<int>(a.v_) && std::holds_alternative<int>(b.v_))
        return (std::get<int>(a.v_) > std::get<int>(b.v_)) ? 1 : -1;

    if (std::holds_alternative<std::string>(a.v_) && std::holds_alternative<std::string>(b.v_))
        return (std::get<std::string>(a.v_) > std::get<std::string>(b.v_)) ? 1 : -1;

    if (std::holds_alternative<double>(a.v_) && std::holds_alternative<double>(b.v_))
        return (std::get<double>(a.v_) > std::get<double>(b.v_)) ? 1 : -1;

    auto rank = [](const Dom& d) noexcept {
        if (std::holds_alternative<double>(d.v_))      return 0;
        if (std::holds_alternative<std::string>(d.v_)) return 1;
        return 2;
    };

    int ra = rank(a), rb = rank(b);
    return (ra < rb) ? -1 : 1;
}

Dom::tt Dom::tt_of_string(const std::string& s) {
    if (s == "int")    return tt::TInt;
    if (s == "string") return tt::TStr;   
    if (s == "float")  return tt::TFloat;
    throw std::invalid_argument("type " + s + " is not supported");
}


Dom::tt Dom::tt_of_domain(const Dom& d) noexcept {
    if (std::holds_alternative<int>(d.v_))          return tt::TInt;
    if (std::holds_alternative<std::string>(d.v_))  return tt::TStr;
    return tt::TFloat;
}


std::string Dom::tt_to_string(Dom::tt t) noexcept {
    switch (t) {
        case tt::TInt:   return "int";
        case tt::TStr:   return "string";
        case tt::TFloat: return "float";
    }
    return "int";
}

Dom Dom::tt_default(Dom::tt t) {
    switch (t) {
        case tt::TInt:   return Dom::Int(0);
        case tt::TStr:   return Dom::Str("");
        case tt::TFloat: return Dom::Float(0.0);
    }
    return Dom::Int(0);
}

Dom Dom::string_to_t(const std::string& s, Dom::tt t) {
    switch (t) {
        case tt::TInt: {
            int v{};
            auto res = std::from_chars(s.data(), s.data() + s.size(), v);
            if (res.ec != std::errc() || res.ptr != s.data() + s.size()) 
                throw std::invalid_argument(s + " is not an int");
            return Dom::Int(v);
        }
        case tt::TStr:
            return Dom::Str(s);

        case tt::TFloat: {
            double v{};
            auto res = std::from_chars(s.data(), s.data() + s.size(), v);
            if (res.ec != std::errc() || res.ptr != s.data() + s.size()) 
                throw std::invalid_argument(s + " is not a float");
            return Dom::Float(v);
        }
    }
    return Dom::Int(0);
}

std::string Dom::to_string(const Dom& d) {
    if (std::holds_alternative<int>(d.v_))         return std::to_string(std::get<int>(d.v_));
    if (std::holds_alternative<std::string>(d.v_)) return std::get<std::string>(d.v_);
    std::ostringstream oss;
    oss << std::get<double>(d.v_);
    return oss.str();
}

std::string Dom::list_to_string(const std::vector<Dom>& ds) {
    std::string out;
    for (const auto& d : ds) {
        if (!out.empty()) out += ", ";
        out += Dom::to_string(d);
    }
    return out;
}