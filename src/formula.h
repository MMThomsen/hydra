#ifndef __FORMULA_H__
#define __FORMULA_H__

#include "constants.h"
#include "common.h"
#include "DFA.h"
#include "util.h"
#include "pred.h"


#include <cassert>
#include <cstdlib>
#include <vector>
#include <functional>
#include <algorithm>

struct Regex;
struct LookaheadRegex;
struct SymbolRegex;
struct PlusRegex;
struct TimesRegex;
struct StarRegex;

struct Formula;
struct BoolFormula;
struct AtomFormula;
struct NegFormula;
struct AndFormula;
struct OrFormula;
struct PrevFormula;
struct NextFormula;
struct SinceFormula;
struct UntilFormula;
struct BwFormula;
struct FwFormula;
struct ExistsFormula;
struct EqConstFormula;

class RegexVisitor {
public:
    virtual void visit(LookaheadRegex *r) = 0;
    virtual void visit(SymbolRegex *r) = 0;
    virtual void visit(PlusRegex *r) = 0;
    virtual void visit(TimesRegex *r) = 0;
    virtual void visit(StarRegex *r) = 0;
};

class FormulaVisitor {
public:
    virtual void visit(BoolFormula *f) = 0;
    virtual void visit(AtomFormula *f) = 0;
    virtual void visit(NegFormula *f) = 0;
    virtual void visit(AndFormula *f) = 0;
    virtual void visit(OrFormula *f) = 0;
    virtual void visit(PrevFormula *f) = 0;
    virtual void visit(NextFormula *f) = 0;
    virtual void visit(SinceFormula *f) = 0;
    virtual void visit(UntilFormula *f) = 0;
    virtual void visit(BwFormula *f) = 0;
    virtual void visit(FwFormula *f) = 0;
    virtual void visit(ExistsFormula *f) = 0; 
    virtual void visit(EqConstFormula *f) = 0; 
};

struct Regex {
    virtual ~Regex() {}
    virtual void accept(RegexVisitor &v) = 0;

    virtual bool nullable() const = 0;
    virtual bool wf() const = 0;

    virtual bool equal(const Regex *r) const = 0;
    virtual bool equalLookahead(const LookaheadRegex *r) const {
        return false;
    }
    virtual bool equalSymbol(const SymbolRegex *r) const {
        return false;
    }
    virtual bool equalPlus(const PlusRegex *r) const {
        return false;
    }
    virtual bool equalTimes(const TimesRegex *r) const {
        return false;
    }
    virtual bool equalStar(const StarRegex *r) const {
        return false;
    }
};

struct LookaheadRegex : Regex {
    Formula *f;
    int f_owner;
    int fid;

    LookaheadRegex(Formula *f) : f(f), f_owner(1), fid(-1) {}
    ~LookaheadRegex();
    void accept(RegexVisitor &v) override {
        v.visit(this);
    }

    bool nullable() const override {
        return true;
    }
    bool wf() const override {
        return false;
    }

    bool equal(const Regex *r) const override {
        return r->equalLookahead(this);
    }
    bool equalLookahead(const LookaheadRegex *r) const override;
};

struct SymbolRegex : Regex {
    Formula *f;
    int f_owner;
    int fid;

    SymbolRegex(Formula *f) : f(f), f_owner(1), fid(-1) {}
    ~SymbolRegex();
    void accept(RegexVisitor &v) override {
        v.visit(this);
    }

    bool nullable() const override {
        return false;
    }
    bool wf() const override {
        return true;
    }

    bool equal(const Regex *r) const override {
        return r->equalSymbol(this);
    }
    bool equalSymbol(const SymbolRegex *r) const override;
};

struct PlusRegex : Regex {
    Regex *left, *right;

    PlusRegex(Regex *left, Regex *right) : left(left), right(right) {}
    ~PlusRegex() {
        if (left != NULL) delete left;
        if (right != NULL) delete right;
    }
    void accept(RegexVisitor &v) override {
        v.visit(this);
    }

    bool nullable() const override {
        return left->nullable() || right->nullable();
    }
    bool wf() const override {
        return left->wf() && right->wf();
    }

    bool equal(const Regex *r) const override {
        return r->equalPlus(this);
    }
    bool equalPlus(const PlusRegex *r) const override {
        return left->equal(r->left) && right->equal(r->right);
    }
};

struct TimesRegex : Regex {
    Regex *left, *right;

    TimesRegex(Regex *left, Regex *right) : left(left), right(right) {}
    ~TimesRegex() {
        if (left != NULL) delete left;
        if (right != NULL) delete right;
    }
    void accept(RegexVisitor &v) override {
        v.visit(this);
    }

    bool nullable() const override {
        return left->nullable() && right->nullable();
    }
    bool wf() const override {
        return right->wf() && (!right->nullable() || left->wf());
    }

    bool equal(const Regex *r) const override {
        return r->equalTimes(this);
    }
    bool equalTimes(const TimesRegex *r) const override {
        return left->equal(r->left) && right->equal(r->right);
    }
};

struct StarRegex : Regex {
    Regex *body;

    StarRegex(Regex *body) : body(body) {}
    ~StarRegex() {
        if (body != NULL) delete body;
    }
    void accept(RegexVisitor &v) override {
        v.visit(this);
    }

    bool nullable() const override {
        return true;
    }
    bool wf() const override {
        return body->wf();
    }

    bool equal(const Regex *r) const override {
        return r->equalStar(this);
    }
    bool equalStar(const StarRegex *r) const override {
        return body->equal(r->body);
    }
};

struct Formula {
    int is_temporal;

    virtual std::vector<std::string> free_variables() const {
        return std::vector<std::string>();
    }

    Formula(int is_temporal = 1) : is_temporal(is_temporal) {}
    virtual ~Formula() {}
    virtual Pdt::PdtT<bool> eval(const Event *e, const std::vector<std::string>& vars) const {
        assert(0);
    }
    virtual void accept(FormulaVisitor &v) = 0;

    virtual bool equal(const Formula *f) const = 0;

    virtual bool equalBool(const BoolFormula *f) const {
        return false;
    }
    virtual bool equalAtom(const AtomFormula *f) const {
        return false;
    }
    virtual bool equalNeg(const NegFormula *f) const {
        return false;
    }
    virtual bool equalAnd(const AndFormula *f) const {
        return false;
    }
    virtual bool equalOr(const OrFormula *f) const {
        return false;
    }
    virtual bool equalPrev(const PrevFormula *f) const {
        return false;
    }
    virtual bool equalNext(const NextFormula *f) const {
        return false;
    }
    virtual bool equalSince(const SinceFormula *f) const {
        return false;
    }
    virtual bool equalUntil(const UntilFormula *f) const {
        return false;
    }
    virtual bool equalBw(const BwFormula *f) const {
        return false;
    }
    virtual bool equalFw(const FwFormula *f) const {
        return false;
    }
    virtual bool equalExists(const ExistsFormula *f) const { 
        return false; 
    } 
    virtual bool equalEqConst(const EqConstFormula *f) const { 
        return false; 
    } 
};

struct BoolFormula : Formula {
    bool b;

    BoolFormula(bool b) : Formula(0), b(b) {}
    Pdt::PdtT<bool> eval(const Event *e, const std::vector<std::string>& vars) const override {
        return Pdt::Leaf(b);
    }
    void accept(FormulaVisitor &v) override {
        v.visit(this);
    }

    bool equal(const Formula *f) const override {
        return f->equalBool(this);
    }
    bool equalBool(const BoolFormula *f) const override {
        return b == f->b;
    }
};

struct AtomFormula : Formula {
    const char *pred_name;
    int pred;
    std::vector<Term> *args;
    int pred_owner;

    AtomFormula(const char *pred_name, int pred, std::vector<Term> *args, int pred_owner = 0) : Formula(0), pred_name(pred_name), pred(pred), args(args), pred_owner(pred_owner) {}
    ~AtomFormula() {
        if (pred_owner) delete [] pred_name;
        if (args != NULL) delete args;
    }
    Pdt::PdtT<bool> eval(const Event *e, const std::vector<std::string>& vars) const override {
        auto int_pdt = e->evalAtom(pred_name, pred, args, vars);
        auto to_bool = [](int val) -> bool { return val != 0; };
        return Pdt::apply1<int, bool>(vars, to_bool, int_pdt);
    }
    void accept(FormulaVisitor &v) override {
        v.visit(this);
    }

    bool equal(const Formula *f) const override {
        return f->equalAtom(this);
    }
    bool equalAtom(const AtomFormula *f) const override {
        return pred == f->pred;
    }

    std::vector<string> free_variables() const override {
        std::vector<std::string> vars;
            for (const auto& term : *args) {
                if (Term::isVar(term)) {
                    vars.push_back(Term::unvar(term));
                }
            }
        return vars;
    }
};


struct NegFormula : Formula {
    Formula *f;

    NegFormula(Formula *f) : Formula(f->is_temporal), f(f) {}
    ~NegFormula() override {
        if (f != NULL) delete f;
    }
    Pdt::PdtT<bool> eval(const Event *e, const std::vector<std::string>& vars) const override {
        auto pdt_before = f->eval(e, vars);
        auto negate = [](bool val) -> bool { return !val; };
        auto pdt_after = Pdt::apply1<bool, bool>(vars, negate, pdt_before);
        return pdt_after;
    }

    void accept(FormulaVisitor &v) override {
        v.visit(this);
    }

    bool equal(const Formula *f) const override {
        return f->equalNeg(this);
    }
    bool equalNeg(const NegFormula *sub) const override {
        return f->equal(sub->f);
    }

    std::vector<std::string> free_variables() const override {
        return f->free_variables();
    }

};

struct AndFormula : Formula {
    Formula *f, *g;

    AndFormula(Formula *f, Formula *g) : Formula(f->is_temporal || g->is_temporal), f(f), g(g) {}
    ~AndFormula() override {
        if (f != NULL) delete f;
        if (g != NULL) delete g;
    }
    Pdt::PdtT<bool> eval(const Event *e, const std::vector<std::string>& vars) const override {
        auto do_and = [](bool v1, bool v2) -> bool {
            return v1 && v2;
        };
        return Pdt::apply2<bool, bool, bool>(vars, do_and, f->eval(e, vars), g->eval(e, vars));
    }
    void accept(FormulaVisitor &v) override {
        v.visit(this);
    }
    bool equal(const Formula *f) const override {
        return f->equalAnd(this);
    }
    bool equalAnd(const AndFormula *sub) const override {
        return f->equal(sub->f) && g->equal(sub->g);
    }
    std::vector<std::string> free_variables() const override {
        std::vector<std::string> free_variables_f = f->free_variables();
        std::vector<std::string> free_variables_g = g->free_variables();

        std::vector<std::string> vars = free_variables_f;
        for (const auto& var : free_variables_g) { 
            if (std::find(vars.begin(), vars.end(), var) == vars.end()) {
                vars.push_back(var);
            }
        }

        return vars;
    }
};

struct OrFormula : Formula {
    Formula *f, *g;

    OrFormula(Formula *f, Formula *g) : Formula(f->is_temporal || g->is_temporal), f(f), g(g) {}
    ~OrFormula() override {
        if (f != NULL) delete f;
        if (g != NULL) delete g;
    }
    Pdt::PdtT<bool> eval(const Event *e, const std::vector<std::string>& vars) const override {
        auto do_or = [](bool v1, bool v2) -> bool {
            return v1 || v2;
        };

        return Pdt::apply2<bool, bool, bool>(vars,
            do_or,
            f->eval(e, vars),
            g->eval(e, vars)
        );
    }
    void accept(FormulaVisitor &v) override {
        v.visit(this);
    }

    bool equal(const Formula *f) const override {
        return f->equalOr(this);
    }
    bool equalOr(const OrFormula *sub) const override {
        return f->equal(sub->f) && g->equal(sub->g);
    }
    std::vector<std::string> free_variables() const override {
        std::vector<std::string> free_variables_f = f->free_variables();
        std::vector<std::string> free_variables_g = g->free_variables();

        std::vector<std::string> vars = free_variables_f;
        for (const auto& var : free_variables_g) { 
            if (std::find(vars.begin(), vars.end(), var) == vars.end()) {
                vars.push_back(var);
            }
        }

        return vars;
    }
};

struct PrevFormula : Formula {
    Formula *f;
    timestamp from, to;

    PrevFormula(Formula *f, interval in) : f(f), from(in.from), to(in.to) {}
    ~PrevFormula() override {
        if (f != NULL) delete f;
    }
    void accept(FormulaVisitor &v) override {
        v.visit(this);
    }

    bool equal(const Formula *f) const override {
        return f->equalPrev(this);
    }
    bool equalPrev(const PrevFormula *sub) const override {
        return f->equal(sub->f) && from == sub->from && to == sub->to;
    }
    std::vector<std::string> free_variables() const override {
        return f->free_variables();
    }
};

struct NextFormula : Formula {
    Formula *f;
    timestamp from, to;

    NextFormula(Formula *f, interval in) : f(f), from(in.from), to(in.to) {}
    ~NextFormula() override {
        if (f != NULL) delete f;
    }
    void accept(FormulaVisitor &v) override {
        v.visit(this);
    }

    bool equal(const Formula *f) const override {
        return f->equalNext(this);
    }
    bool equalNext(const NextFormula *sub) const override {
        return f->equal(sub->f) && from == sub->from && to == sub->to;
    }
    std::vector<std::string> free_variables() const override {
        return f->free_variables();
    }
};

struct SinceFormula : Formula {
    Formula *f, *g;
    timestamp from, to;

    SinceFormula(Formula *f, Formula *g, interval in) : f(f), g(g), from(in.from), to(in.to) {}
    ~SinceFormula() override {
        if (f != NULL) delete f;
        if (g != NULL) delete g;
    }
    void accept(FormulaVisitor &v) override {
        v.visit(this);
    }

    bool equal(const Formula *f) const override {
        return f->equalSince(this);
    }
    bool equalSince(const SinceFormula *sub) const override {
        return f->equal(sub->f) && g->equal(sub->g) && from == sub->from && to == sub->to;
    }

    std::vector<std::string> free_variables() const override {
        std::vector<std::string> free_variables_f = f->free_variables();
        std::vector<std::string> free_variables_g = g->free_variables();

        std::vector<std::string> vars = free_variables_f;
        for (const auto& var : free_variables_g) { 
            if (std::find(vars.begin(), vars.end(), var) == vars.end()) {
                vars.push_back(var);
            }
        }

        return vars;
    }
};

struct UntilFormula : Formula {
    Formula *f, *g;
    timestamp from, to;

    UntilFormula(Formula *f, Formula *g, interval in) : f(f), g(g), from(in.from), to(in.to) {}
    ~UntilFormula() override {
        if (f != NULL) delete f;
        if (g != NULL) delete g;
    }
    void accept(FormulaVisitor &v) override {
        v.visit(this);
    }

    bool equal(const Formula *f) const override {
        return f->equalUntil(this);
    }
    bool equalUntil(const UntilFormula *sub) const override {
        return f->equal(sub->f) && g->equal(sub->g) && from == sub->from && to == sub->to;
    }
    std::vector<std::string> free_variables() const override {
        std::vector<std::string> free_variables_f = f->free_variables();
        std::vector<std::string> free_variables_g = g->free_variables();

        std::vector<std::string> vars = free_variables_f;
        for (const auto& var : free_variables_g) { 
            if (std::find(vars.begin(), vars.end(), var) == vars.end()) {
                vars.push_back(var);
            }
        }

        return vars;
    }
};

struct BwFormula : Formula {
    Regex *r;
    timestamp from, to;

    BwFormula(Regex *r, interval in) : r(r), from(in.from), to(in.to) {}
    ~BwFormula() {
        if (r != NULL) delete r;
    }
    void accept(FormulaVisitor &v) override {
        v.visit(this);
    }

    bool equal(const Formula *f) const override {
        return f->equalBw(this);
    }
    bool equalBw(const BwFormula *f) const override {
        return r->equal(f->r) && from == f->from && to == f->to;
    }
    
    // Below is not needed for current setup, since we do not use the variable in temporal formulas.
    std::vector<std::string> free_variables() const override {
        std::vector<std::string> vars;
        std::function<void(Regex*)> collect = [&](Regex* regex) {
            if (auto* lr = dynamic_cast<LookaheadRegex*>(regex)) {
                auto fvars = lr->f->free_variables();
                for (const auto& v : fvars) {
                    if (std::find(vars.begin(), vars.end(), v) == vars.end()) vars.push_back(v);
                }
            } else if (auto* sr = dynamic_cast<SymbolRegex*>(regex)) {
                auto fvars = sr->f->free_variables();
                for (const auto& v : fvars) {
                    if (std::find(vars.begin(), vars.end(), v) == vars.end()) vars.push_back(v);
                }
            } else if (auto* pr = dynamic_cast<PlusRegex*>(regex)) {
                collect(pr->left);
                collect(pr->right);
            } else if (auto* tr = dynamic_cast<TimesRegex*>(regex)) {
                collect(tr->left);
                collect(tr->right);
            } else if (auto* sr = dynamic_cast<StarRegex*>(regex)) {
                collect(sr->body);
            }
        };
        collect(r);
        return vars;
    }
};

struct FwFormula : Formula {
    Regex *r;
    timestamp from, to;

    FwFormula(Regex *r, interval in) : r(r), from(in.from), to(in.to) {}
    ~FwFormula() {
        if (r != NULL) delete r;
    }
    void accept(FormulaVisitor &v) override {
        v.visit(this);
    }

    bool equal(const Formula *f) const override {
        return f->equalFw(this);
    }
    bool equalFw(const FwFormula *f) const override {
        return r->equal(f->r) && from == f->from && to == f->to;
    }
    
    // Below is not needed for current setup, since we do not use the variable in temporal formulas.
    std::vector<std::string> free_variables() const override {
        std::vector<std::string> vars;
        std::function<void(Regex*)> collect = [&](Regex* regex) {
            if (auto* lr = dynamic_cast<LookaheadRegex*>(regex)) {
                auto fvars = lr->f->free_variables();
                for (const auto& v : fvars) {
                    if (std::find(vars.begin(), vars.end(), v) == vars.end()) vars.push_back(v);
                }
            } else if (auto* sr = dynamic_cast<SymbolRegex*>(regex)) {
                auto fvars = sr->f->free_variables();
                for (const auto& v : fvars) {
                    if (std::find(vars.begin(), vars.end(), v) == vars.end()) vars.push_back(v);
                }
            } else if (auto* pr = dynamic_cast<PlusRegex*>(regex)) {
                collect(pr->left);
                collect(pr->right);
            } else if (auto* tr = dynamic_cast<TimesRegex*>(regex)) {
                collect(tr->left);
                collect(tr->right);
            } else if (auto* sr = dynamic_cast<StarRegex*>(regex)) {
                collect(sr->body);
            }
        };
        collect(r);
        return vars;
    }
};

struct ExistsFormula : Formula {
    const char *pred_name;
    Formula *f;

    ExistsFormula(const char *pred_name, Formula *f)
        : Formula(f->is_temporal), pred_name(pred_name), f(f) {}
    
    ~ExistsFormula() {
        if (f != NULL) delete f;
    }
    
    void accept(FormulaVisitor &v) override { 
        v.visit(this); 
    }
    
    bool equal(const Formula *f) const override { 
        return f->equalExists(this); 
    }
    
    bool equalExists(const ExistsFormula *sub) const override {
        return f->equal(sub->f) && strcmp(pred_name, sub->pred_name) == 0;
    }
    
    std::vector<std::string> free_variables() const override {
        auto child_vars = f->free_variables();
        std::vector<std::string> vars;
        for (const auto& var : child_vars) {
            if (var != std::string(pred_name)) {
                vars.push_back(var);
            }
        }
        return vars;
    }
    
    Pdt::PdtT<bool> eval(const Event *e, const std::vector<std::string>& vars) const override {
        std::vector<std::string> local_vars;
        local_vars.push_back(std::string(pred_name));
        local_vars.insert(local_vars.end(), vars.begin(), vars.end());
        Pdt::PdtT<bool> local_pdt = f->eval(e, local_vars);

        std::function<Pdt::PdtT<bool>(const Pdt::PdtT<bool>&)> process;
        process = [&](const Pdt::PdtT<bool>& pdt) -> Pdt::PdtT<bool> {
            if (Pdt::isleaf(pdt)) {
                return pdt;
            }
            
            if (Pdt::isnode(pdt)) {
                const auto& var_name = Pdt::var(pdt);
                const auto& part = Pdt::part(pdt);
                
                if (var_name == std::string(pred_name)) {
                    std::vector<Pdt::PdtT<bool>> branches_to_or;
                    for (const auto& [subset, sub_pdt] : part) {
                        //if (Setc::isComplement(subset)) continue;
                        branches_to_or.push_back(process(sub_pdt));
                    }
                    
                    if (branches_to_or.empty()) {
                        return Pdt::Leaf<bool>(false);
                    }
                    
                    if (branches_to_or.size() == 1) {
                        return branches_to_or[0];
                    }
                    
                    auto or_all = [](const std::vector<bool>& values) -> bool {
                        for (bool v : values) {
                            if (v) return true;
                        }
                        return false;
                    };
                    
                    return Pdt::applyN<bool, bool>(vars, or_all, branches_to_or);
                } else {
                    Part::PartT<Pdt::PdtT<bool>> new_part;
                    for (const auto& [subset, sub_pdt] : part) {
                        new_part.push_back({subset, process(sub_pdt)});
                    }
                    return Pdt::Node<bool>(var_name, new_part);
                }
            }
            
            return Pdt::Leaf<bool>(false);
        };
        
        auto result = process(local_pdt);
        
        return result;
    }
};

struct EqConstFormula : Formula {
    const char *var_name;
    Term *constant;

    EqConstFormula(const char *var_name, Term *constant): Formula(0), var_name(var_name), constant(constant) {}
    
    ~EqConstFormula() override {
        if (constant != NULL) delete constant;
    }
    
    void accept(FormulaVisitor &v) override {
        v.visit(this);
    }
    
    bool equal(const Formula *f) const override {
        return f->equalEqConst(this);
    }
    
    bool equalEqConst(const EqConstFormula *sub) const override {
        return Term::equal(*constant, *(sub->constant)) && strcmp(var_name, sub->var_name) == 0;
    }
    
    std::vector<string> free_variables() const override {
        std::vector<std::string> vars;
        vars.push_back(std::string(var_name));
        return vars;
    }
    Pdt::PdtT<bool> eval(const Event *e, const std::vector<std::string>& vars) const override {
        std::vector<std::string> pdt_vars = { std::string(var_name) };
        std::vector<std::unordered_map<std::string, Dom>> maps;

        if (!Term::isVar(*constant)) {
            std::unordered_map<std::string, Dom> m;
            m[std::string(var_name)] = Term::unconst(*constant); 
            maps.push_back(std::move(m));
        } else {
            std::unordered_map<std::string, Dom> m;
            m[std::string(var_name)] = Dom::Str(Term::unvar(*constant)); 
            maps.push_back(std::move(m));
        }

        auto pdt_int = Pdt::pdt_of(pdt_vars, maps);

        auto to_bool = [](int v) -> bool { return v != 0; };

        return Pdt::apply1<int, bool>(vars, to_bool, pdt_int);
    }
};

#endif /* __FORMULA_H__ */
