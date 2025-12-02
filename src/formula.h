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
struct ExistsFormula;  // Added

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
    virtual void visit(ExistsFormula *f) = 0;  // Added
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
    virtual bool eval(const Event *e, const std::vector<std::string>& vars) const {
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
    virtual bool equalExists(const ExistsFormula *f) const {  // Added
        return false;  // Added
    }  // Added
};

struct BoolFormula : Formula {
    bool b;

    BoolFormula(bool b) : Formula(0), b(b) {}
    bool eval(const Event *e, const std::vector<std::string>& vars) const override {
        return b;
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
    std::vector<Term> *args;                // Added
    int pred_owner;

    AtomFormula(const char *pred_name, int pred, std::vector<Term> *args, int pred_owner = 0) : Formula(0), pred_name(pred_name), pred(pred), args(args), pred_owner(pred_owner) {}
    ~AtomFormula() {
        if (pred_owner) delete [] pred_name;
        if (args != NULL) delete args;      // Added
    }
    bool eval(const Event *e, const std::vector<std::string>& vars) const override {
        std::cout << "AtomFormula::eval():  vars = [";
        for (size_t i = 0; i < vars.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << vars[i];
        }
        std::cout << "]\n";
        return e->evalAtom(pred_name, pred, args, vars); 
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
    bool eval(const Event *e, const std::vector<std::string>& vars) const override {
        return !f->eval(e, vars);
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
    bool eval(const Event *e, const std::vector<std::string>& vars) const override {
        // Create a function that calls evalAtomPDT and then use do_and, vars and apply2 
        // to compare 


        std::cout << "AndFormula::eval():  vars = [";
        for (size_t i = 0; i < vars.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << vars[i];
        }
        std::cout << "]\n";
        return f->eval(e, vars) && g->eval(e, vars);
    }
    //bool eval(const Event *e) const override { // remember to add vars/free variables here
        // FIRST: Evaluate children to populate pdt_list
        //bool f_result = f->eval(e);
        //bool g_result = g->eval(e);
//
        //
        //// THEN: Print PDTs from pdt_list for testing
        //std::cout << "\n=== AndFormula::eval() - Checking PDTs ===\n";
        //
        //// Helper function to print a PDT
        //std::function<void(const Pdt::PdtT<int>&, const std::string&, int)> print_pdt;
        //print_pdt = [&](const Pdt::PdtT<int>& p, const std::string& indent, int depth) {
        //    if (Pdt::isleaf(p)) {
        //        std::cout << indent << "Leaf(" << Pdt::unleaf(p) << ")\n";
        //    } else if (Pdt::isnode(p)) {
        //        std::cout << indent << "Node(\"" << Pdt::var(p) << "\", [\n";
        //        const auto& part = Pdt::part(p);
        //        for (size_t i = 0; i < part.size(); ++i) {
        //            const auto& [sub, sub_pdt] = part[i];
        //            std::cout << indent << "  (" << Setc::to_string(sub) << ",\n";
        //            print_pdt(sub_pdt, indent + "    ", depth + 1);
        //            std::cout << indent << "  )";
        //            if (i < part.size() - 1) std::cout << ",";
        //            std::cout << "\n";
        //        }
        //        std::cout << indent << "])\n";
        //    }
        //};
        //
        //// Check and print PDT for predicate 0
        //if (e->pdt_list.size() > 0 && e->pdt_list[0].has_value()) {
        //    std::cout << "\n=== PDT for predicate 0 ===\n";
        //    std::cout << "vars: [";
        //    for (size_t i = 0; i < e->vars.size(); ++i) {
        //        if (i > 0) std::cout << ", ";
        //        std::cout << e->vars[i];
        //    }
        //    std::cout << "]\n";
        //    std::cout << "PDT structure:\n";
        //    print_pdt(e->pdt_list[0].value(), "  ", 0);
        //    std::cout << "=== End PDT 0 ===\n";
        //} else {
        //    std::cout << "predicate 0 is empty\n";
        //}
        //
        //// Check and print PDT for predicate 1
        //if (e->pdt_list.size() > 1 && e->pdt_list[1].has_value()) {
        //    std::cout << "\n=== PDT for predicate 1 ===\n";
        //    std::cout << "vars: [";
        //    for (size_t i = 0; i < e->vars.size(); ++i) {
        //        if (i > 0) std::cout << ", ";
        //        std::cout << e->vars[i];
        //    }
        //    std::cout << "]\n";
        //    std::cout << "PDT structure:\n";
        //    print_pdt(e->pdt_list[1].value(), "  ", 0);
        //    std::cout << "=== End PDT 1 ===\n";
        //} else {
        //    std::cout << "predicate 1 is empty\n";
        //}
        //
        //std::cout << "=== End AndFormula check ===\n\n";
        //
        //// Combine PDTs if both predicates have data
        //if (e->pdt_list.size() > 1 && e->pdt_list[0].has_value() && e->pdt_list[1].has_value()) {
        //    std::cout << "\n=== Combining PDTs with apply2 ===\n";
        //    
        //    // Logical AND function for combining PDT values
        //    auto do_and = [](int val1, int val2) -> int {
        //        return val1 && val2;
        //    };
//
        //    // Combine PDTs using apply2
        //    // Template parameters: <InputType1, InputType2, OutputType>
        //    auto combined_pdt = Pdt::apply2<int, int, int>(e->vars,
        //        do_and,
        //        e->pdt_list[0].value(),
        //        e->pdt_list[1].value()
        //    );
        //    
        //    // Print the combined PDT
        //    std::cout << "\n=== Combined PDT (p AND q) ===\n";
        //    std::cout << "vars: [";
        //    for (size_t i = 0; i < e->vars.size(); ++i) {
        //        if (i > 0) std::cout << ", ";
        //        std::cout << e->vars[i];
        //    }
        //    std::cout << "]\n";
        //    std::cout << "Combined PDT structure:\n";
        //    print_pdt(combined_pdt, "  ", 0);
        //    std::cout << "=== End Combined PDT ===\n\n";
        //} else {
        //    std::cout << "Cannot combine PDTs: one or both predicates empty\n\n";
        //}

        //return f_result && g_result;
    //}
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
    bool eval(const Event *e, const std::vector<std::string>& vars) const override {
        std::cout << "OrFormula::eval():  vars = [";
        for (size_t i = 0; i < vars.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << vars[i];
        }
        std::cout << "]\n";
        return f->eval(e, vars) || g->eval(e, vars);
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

struct ExistsFormula : Formula {  // Added
    const char *pred_name;  // Added
    Formula *f;  // Added
    int var_owner;  // Added

    ExistsFormula(const char *pred_name, Formula *f, int var_owner = 0)  // Added
        : Formula(f->is_temporal), pred_name(pred_name), f(f), var_owner(var_owner) {}  // Added
    
    void accept(FormulaVisitor &v) override { v.visit(this); }  // Added
    bool equal(const Formula *f) const override { return false; }  // Added
};  // Added

#endif /* __FORMULA_H__ */
