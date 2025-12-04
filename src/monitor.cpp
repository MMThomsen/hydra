#include "monitor.h"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <stdexcept>

#include "common.h"
#include "formula.h"
#include "trie.h"
#include "util.h"

Monitor *NonTempMonitor::clone() {
    NonTempMonitor *mon = new NonTempMonitor(fmla, input_reader, handle->clone());
    mon->eof = eof;
    return mon;
}
BooleanVerdict NonTempMonitor::step_impl(const std::vector<std::string>& vars) {
    input_reader->read_handle(handle);
    if (handle->eof) throw EOL();
    Pdt::PdtT<bool> b = fmla->eval(handle, vars);
            auto do_boolean = [](bool v) -> Boolean { 
            return v ? TRUE : FALSE;
        };
        Pdt::PdtT<Boolean> pb = Pdt::apply1<bool, Boolean>(vars, do_boolean, b);

        auto leaf_identity = [](Boolean b) -> Boolean { return b; };
        auto check_any_true = [](const Part::PartT<Boolean>& part) -> Boolean {
            // Return TRUE if any element in the partition is TRUE
            for (const auto& [sub, val] : part) {
                if (val == TRUE) return TRUE;
            }
            return FALSE;
        };
        
        // Helper function to print a Boolean PDT
        std::function<void(const Pdt::PdtT<Boolean>&, const std::string&, int)> print_pdt;
        print_pdt = [&](const Pdt::PdtT<Boolean>& p, const std::string& indent, int depth) {
            if (Pdt::isleaf(p)) {
                std::cout << indent << "Leaf(" << (Pdt::unleaf(p) == TRUE ? "TRUE" : "FALSE") << ")\n";
            } else if (Pdt::isnode(p)) {
                std::cout << indent << "Node(\"" << Pdt::var(p) << "\", [\n";
                const auto& part = Pdt::part(p);
                for (size_t i = 0; i < part.size(); ++i) {
                    const auto& [sub, sub_pdt] = part[i];
                    std::cout << indent << "  (" << Setc::to_string(sub) << ",\n";
                    print_pdt(sub_pdt, indent + "    ", depth + 1);
                    std::cout << indent << "  )";
                    if (i < part.size() - 1) std::cout << ",";
                    std::cout << "\n";
                }
                std::cout << indent << "])\n";
            }
        };
        
        // Simple recursive function to check if any leaf in PDT is TRUE
        // Only check explicit (non-complement) branches
        std::function<bool(const Pdt::PdtT<Boolean>&)> has_true_leaf;
        has_true_leaf = [&](const Pdt::PdtT<Boolean>& pdt) -> bool {
            if (Pdt::isleaf(pdt)) {
                return Pdt::unleaf(pdt) == TRUE;
            } else if (Pdt::isnode(pdt)) {
                const auto& part = Pdt::part(pdt);
                for (const auto& [sub, sub_pdt] : part) {
                    // Skip complement branches - only check explicit domain values
                    if (Setc::isComplement(sub)) continue;
                    
                    if (has_true_leaf(sub_pdt)) {
                        return true;
                    }
                }
                return false;
            }
            return false;
        };
        
        Boolean result = has_true_leaf(pb) ? TRUE : FALSE;
        
        return BooleanVerdict(handle->ts, result);
        //return BooleanVerdict(handle->ts, b ? TRUE : FALSE); // change true to / last part, to return a Pdt::PdtT<Boolean>
}

Monitor *PrevMonitor::clone() {
    PrevMonitor *mon = new PrevMonitor(input_reader, handle->clone(), subf->clone(), from, to);
    mon->eof = eof;
    mon->v = v;
    return mon;
}
BooleanVerdict PrevMonitor::step_impl(const std::vector<std::string>& vars) {
    input_reader->read_handle(handle);
    if (handle->eof) throw EOL();
    Boolean b = FALSE;
    if (v) {
        if (mem(v->ts, handle->ts, from, to)) b = v->b;
    }
    try {
        v = subf->step(vars);
    } catch (const EOL &e) {
        eof = 1;
    }
    return BooleanVerdict(handle->ts, b);
}

Monitor *NextMonitor::clone() {
    NextMonitor *mon = new NextMonitor(input_reader, handle->clone(), subf->clone(), from, to);
    mon->eof = eof;
    mon->t = t;
    return mon;
}
BooleanVerdict NextMonitor::step_impl(const std::vector<std::string>& vars) {
    input_reader->read_handle(handle);
    if (handle->eof) throw EOL();
    if (t) {
        timestamp t0 = *t;
        t = handle->ts;
        try {
            BooleanVerdict v = subf->step(vars);
            Boolean b = FALSE;
            if (mem(t0, handle->ts, from, to)) b = v.b;
            return BooleanVerdict(t0, b);
        } catch (const EOL &e) {
            eof = 1;
            if (mem(t0, handle->ts, from, to)) {
                throw EOL();
            } else {
                return BooleanVerdict(t0, FALSE);
            }
        }
    } else {
        subf->step(vars);
        t = handle->ts;
        return step_impl(vars);
    }
}

Monitor *SinceMonitor::clone() {
    SinceMonitor *mon = new SinceMonitor(input_reader, handle->clone(), subf->clone(), subg->clone(), from, to);
    mon->eof = eof;
    mon->cphi = cphi;
    mon->cpsi = cpsi;
    mon->ocpsi = ocpsi;
    mon->otpsi = otpsi;
    return mon;
}
BooleanVerdict SinceMonitor::step_impl(const std::vector<std::string>& vars) {
    BooleanVerdict vf = subf->step(vars);
    if (vf.b == TRUE) cphi++;
    else cphi = 0;
    cpsi++;
    if (ocpsi) (*ocpsi)++;
    while (cpsi > 0 && memL(handle->ts, vf.ts, from, to)) {
        input_reader->read_handle(handle);
        BooleanVerdict vg = subg->step(vars);
        if (vg.b == TRUE) {
            ocpsi = cpsi;
            otpsi = vg.ts;
        }
        cpsi--;
    }
    Boolean b = FALSE;
    if (ocpsi && (*ocpsi) - 1 <= cphi && memR((*otpsi), vf.ts, from, to)) b = TRUE;
    return BooleanVerdict(vf.ts, b);
}

Monitor *UntilMonitor::clone() {
    UntilMonitor *mon = new UntilMonitor(input_reader, front->clone(), back->clone(), subf->clone(), subg->clone(), from, to);
    mon->eof = eof;
    mon->c = c;
    mon->z = z;
    return mon;
}
BooleanVerdict UntilMonitor::step_impl(const std::vector<std::string>& vars) {
    input_reader->read_handle(back);
    if (back->eof) throw EOL();
    while (loopCondUntil()) {
        BooleanVerdict vf = subf->step(vars);
        BooleanVerdict vg = subg->step(vars);
        input_reader->read_handle(front);
        c++;
        z = make_pair(vf.ts, make_pair(vf.b, vg.b));
    }
    if (c == 0) throw EOL();
    else {
        c--;
        if (z->second.second && memL(back->ts, z->first, from, to)) return BooleanVerdict(back->ts, TRUE);
        else if (!z->second.first) return BooleanVerdict(back->ts, FALSE);
        else if (front->eof) throw EOL();
        else return BooleanVerdict(back->ts, FALSE);
    }
}

Monitor *BwMonitor::clone() {
    BwMonitor *mon = new BwMonitor(from, to, dfa, s->clone(), 0);
    mon->eof = eof;
    return mon;
}
BooleanVerdict BwMonitor::step_impl(const std::vector<std::string>& vars) {
    return s->check_bw(from, to);
}

Monitor *FwMonitor::clone() {
    FwMonitor *mon = new FwMonitor(from, to, dfa, s->clone(), 0);
    mon->eof = eof;
    return mon;
}
BooleanVerdict FwMonitor::step_impl(const std::vector<std::string>& vars) {
    return s->check_fw(from, to);
}
