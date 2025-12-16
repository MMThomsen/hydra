#ifndef __SETC_H__
#define __SETC_H__

#include "dom.h"
#include <set>
#include <vector>
#include <optional>
#include <string>
#include <stdexcept>

struct Setc {
    using SetT = std::set<Dom, Dom::Less>;

    enum class sType { Finite, Complement };

    Setc();                              
    static Setc Finite(SetT s);     
    static Setc Complement(SetT s);        
    static Setc univ();             

    static bool isFinite     (const Setc& x) noexcept;
    static bool isComplement (const Setc& x) noexcept;
    static bool isEmpty      (const Setc& x) noexcept;
    static int  length       (const Setc& x) noexcept;
    static bool equal        (const Setc& a, const Setc& b) noexcept;

    static Setc add   (const Setc& x, const Dom& v);
    static Setc inter (const Setc& a, const Setc& b);
    static Setc union_(const Setc& a, const Setc& b);
    static Setc diff  (const Setc& a, const Setc& b);

    static std::optional<Dom> min_elt(const Setc& x);        
    static Dom                some_elt(Dom::tt ty, const Setc& x); 

    static std::vector<Dom> to_list(const Setc& x); 
    static std::string      to_json  (const Setc& x);
    static std::string      to_string(const Setc& x);
    static std::string      to_latex (const Setc& x);

    bool operator<(const Setc& other) const {
        if (type_ != other.type_) return type_ < other.type_;
        return s_ < other.s_;
    }
    
    bool operator==(const Setc& other) const {
        return type_ == other.type_ && s_ == other.s_;
    }

private:

    sType type_;
    SetT  s_;


    Setc(sType t, SetT s) : type_(t), s_(std::move(s)) {}
};

#endif // __SETC_H__