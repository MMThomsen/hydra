#ifndef __DOM_H__
#define __DOM_H__

//#include "common.h"
#include <string>
#include <variant>
#include <vector>
#include <ostream>
#include <stdexcept>


struct Dom {
    // Whymon OCaml: type tt = TInt | TStr | TFloat
    enum class tt { TInt, TStr, TFloat };

    // Whymon OCaml: type t  = Int of int | Str of string | Float of float
    using t = std::variant<int, std::string, double>;


    Dom();
    static Dom Int(int v);
    static Dom Str(std::string v);
    static Dom Float(double v);


    // Comparison functions
    static bool         equal(const Dom& d, const Dom& dprime) noexcept;
    static int          compare_t(const Dom& d, const Dom& dprime) noexcept;
    static bool         tt_equal(tt a, tt b) noexcept { return a == b; }

    // Conversion functions
    static tt           tt_of_string(const std::string& s);
    static tt           tt_of_domain(const Dom& d) noexcept;

    static std::string  tt_to_string(tt t) noexcept;
    static Dom          tt_default(tt t);
    static Dom          string_to_t(const std::string& s, tt t);
    static std::string  to_string(const Dom& d);
    static std::string  list_to_string(const std::vector<Dom>& ds);

    friend bool operator==(const Dom& d, const Dom& dprime) noexcept { return equal(d, dprime); }
    friend bool operator!=(const Dom& d, const Dom& dprime) noexcept { return !equal(d, dprime); }
    friend bool operator<(const Dom& d, const Dom& dprime) noexcept { return compare_t(d, dprime) < 0; }

    struct Less { bool operator()(const Dom& a, const Dom& b) const noexcept { return a < b; } };


    friend std::ostream& operator<<(std::ostream& os, const Dom& d) { return os << Dom::to_string(d); }

    private:
        explicit Dom(t v) : v_(std::move(v)) {}   
        t v_;                                     
};

#endif // __DOM_H__
