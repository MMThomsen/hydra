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
        
        return BooleanVerdict(handle->ts, pb, vars);
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
    Pdt::PdtT<Boolean> b = Pdt::Leaf(FALSE);
    if (v) {
        if (mem(v->ts, handle->ts, from, to)) b = v->b;
    }
    try {
        v = subf->step(vars);
    } catch (const EOL &e) {
        eof = 1;
    }
    return BooleanVerdict(handle->ts, b, vars);
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
            Pdt::PdtT<Boolean> b = Pdt::Leaf(FALSE);
            if (mem(t0, handle->ts, from, to)) b = v.b;
            return BooleanVerdict(t0, b, vars);
        } catch (const EOL &e) {
            eof = 1;
            if (mem(t0, handle->ts, from, to)) {
                throw EOL();
            } else {
                return BooleanVerdict(t0, Pdt::Leaf(FALSE), vars);
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
    if (has_true_leaf(vf.b)) cphi++;
    else cphi = 0;
    cpsi++;
    if (ocpsi) (*ocpsi)++;
    while (cpsi > 0 && memL(handle->ts, vf.ts, from, to)) {
        input_reader->read_handle(handle);
        BooleanVerdict vg = subg->step(vars);
        if (has_true_leaf(vg.b)) {
            ocpsi = cpsi;
            otpsi = vg.ts;
        }
        cpsi--;
    }
    Pdt::PdtT<Boolean> b = Pdt::Leaf(FALSE);
    if (ocpsi && (*ocpsi) - 1 <= cphi && memR((*otpsi), vf.ts, from, to)) b = Pdt::Leaf(TRUE);
    return BooleanVerdict(vf.ts, b, vars);
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
        if ((has_true_leaf(z->second.second) ? TRUE : FALSE) && memL(back->ts, z->first, from, to)) return BooleanVerdict(back->ts, Pdt::Leaf(TRUE), vars);
        else if ((!has_true_leaf(z->second.first)) ? TRUE : FALSE) return BooleanVerdict(back->ts, Pdt::Leaf(FALSE), vars);
        else if (front->eof) throw EOL();
        else return BooleanVerdict(back->ts, Pdt::Leaf(FALSE), vars);
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
