
#ifndef __PDT_H__
#define __PDT_H__

#include "dom.h"
#include "setc.h"
#include "pred.h"

#include <vector>
#include <utility>
#include <stdexcept>
#include <optional>
#include <algorithm>
#include <sstream>
#include <functional>
#include <iostream>



struct Part {
    using Sub = Setc;

    template <typename A>
    using PartT = std::vector<std::pair<Sub, A>>;

    //####################### singleton_set ###############################
    static Sub::SetT singleton_set(const Dom& d) {
    Sub::SetT s{ Dom::Less{} };
    s.insert(d);
    return s;
    }

    //######################## trivial #################################
    template <typename A>
    static PartT<A> trivial(const A& a) {
        PartT<A> out;
        out.emplace_back(Sub::univ(), a);
        return out;
    }
    //############################# hd #################################
    template <typename A>
    static const A& hd(const PartT<A>& part) {
        if (part.empty()) throw std::invalid_argument("Part::hd: empty partition");
        return part.front().second;
    }

    //############################# length #################################
    template <typename A>
    static int length(const PartT<A>& part) {
        return static_cast<int>(part.size());
    }

    //############################# map #################################
    template <typename A, typename B, typename F>
    static PartT<B> map(const PartT<A>& part, F f) {
        PartT<B> out; 
        out.reserve(part.size());
        for (const auto& [s, a] : part) {
            out.emplace_back(s, f(a));
        }
        return out;
    }

    //############################# map2 #################################
    template <typename A, typename F>
    static PartT<A> map2(const PartT<A>& part, F f) {
        PartT<A> out;
        out.reserve(part.size());
        for (const auto& p : part) {
            out.emplace_back(f(p));
        }
        return out;
    }

    //############################# fold_left #################################
    template <typename A, typename B, typename F>
    static B fold_left(const PartT<A>& part, B init, F f) {
        for (const auto& [_, a] : part) {
            init = f(init, a);
        }
        return init;
    }

    //############################# filter #################################
    template <typename A, typename F>
    static PartT<A> filter(const PartT<A>& part, F f) {
        PartT<A> out;
        out.reserve(part.size());
        for (const auto& [s, a] : part) {
            if (f(a)) {
                out.emplace_back(s, a);
            }
        }
        return out;
    }

    //############################# exists #################################
    template <typename A, typename F>
    static bool exists(const PartT<A>& part, F f) {
        for (const auto& [_, a] : part) {
            if (f(a)) {
                return true;
            }
        }
        return false;
    }

    //############################# find #################################
    template <typename A, typename F>
    static std::optional<std::pair<Sub, A>> find(const PartT<A>& part, F f) {
        for (const auto& [sub, val] : part) {
            if (f(val)) {
                std::pair<Sub, A> result = std::make_pair(sub, val);
                return result;
            }
        }
        return std::nullopt;
    }

    //############################# for_all #################################
    template <typename A, typename F>
    static bool for_all(const PartT<A>& part, F f) {
        for (const auto& [_, a] : part) {
            if (!f(a)) {
                return false;
            }
        }
        return true;
    }

    //############################# values #################################
    template <typename A>
    static std::vector<A> values(const PartT<A>& part) {
        std::vector<A> out;
        out.reserve(part.size());
        for (const auto& [_, a] : part) {
            out.push_back(a);
        }
        return out;
    }

    //############################# tabulate #################################
    template <typename A, typename F>
    static PartT<A> tabulate(const Sub::SetT& ds, F f, const A& z) {
        PartT<A> out;
        out.emplace_back(Sub::Complement(ds), z); 
        for (const auto& d : ds) {
            out.emplace_back(Sub::Finite(singleton_set(d)), f(d));
        }
        return out;
    }

    //############################# keys #################################
    template <typename A> 
    static std::vector<Sub> keys(const PartT<A>& part) {
        std::vector<Sub> out;
        out.reserve(part.size());
        for (const auto& [s, _] : part) {
            out.push_back(s);
        }
        return out;
    }

    //############################# equal #################################
    template <typename A, typename EqA>
    static bool equal(const PartT<A>& a, const PartT<A>& b, EqA eq) {
        if (a.size() != b.size()) {
            return false;
        }
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (!Sub::equal(a[i].first, b[i].first)) {
                return false;
            }
        }

        for (std::size_t i = 0; i < a.size(); ++i) {
            if (!eq(a[i].second, b[i].second)) {
                return false;
            }
        }
        return true;
    }

    
    //############################ merge2 #################################

    // merge2 f part1 part2, for partitions of the same type.
    template <typename A, typename B, typename F>
    static PartT<B> merge2(F f, const PartT<A>& part1, const PartT<A>& part2) {
        if (part1.empty()) return {};
        PartT<B> result;

        const auto& [sub1, v1] = part1.front();

        for (const auto& [sub2, v2] : part2) {
            Sub inter = Sub::inter(sub1, sub2);
            if (!Sub::isEmpty(inter)) {
                result.emplace_back(std::move(inter), f(v1, v2));
            }
        }


        PartT<A> part2not1;
        part2not1.reserve(part2.size());
        for (const auto& [sub2, v2] : part2) {
            Sub diff = Sub::diff(sub2, sub1);
            if (!Sub::isEmpty(diff)) {
                part2not1.emplace_back(std::move(diff), v2);
            }
        }


        PartT<B> tail = merge2<A,B,F>(f,
            PartT<A>(part1.begin() + 1, part1.end()),
            part2not1);

        result.insert(result.end(),
                      std::make_move_iterator(tail.begin()),
                      std::make_move_iterator(tail.end()));
        return result;
    }

    // merge2 for mixed-type partitions overload for merge3 usage.
    template <typename A1, typename A2, typename B, typename F>
    static PartT<B> merge2(F f, const PartT<A1>& part1, const PartT<A2>& part2) {
       
        if (part1.empty()) return {};
        PartT<B> result;

        const auto& [sub1, v1] = part1.front();

        for (const auto& [sub2, v2] : part2) {
            Sub inter = Sub::inter(sub1, sub2);
            if (!Sub::isEmpty(inter)) {
                result.emplace_back(std::move(inter), f(v1, v2));
            }
        }

        PartT<A2> part2not1;
        part2not1.reserve(part2.size());
        for (const auto& [sub2, v2] : part2) {
            Sub diff = Sub::diff(sub2, sub1);
            if (!Sub::isEmpty(diff)) {
                part2not1.emplace_back(std::move(diff), v2);
            }
        }

        PartT<B> tail = merge2<A1, A2, B, F>(f,
            PartT<A1>(part1.begin() + 1, part1.end()),
            part2not1);

        result.insert(result.end(),
                    std::make_move_iterator(tail.begin()),
                    std::make_move_iterator(tail.end()));
        return result;
    }

    //############################ merge3 #################################
    // merge3 f p1 p2 p3 = merge2 (fun pt3 f' -> f' pt3) p3 (merge2 f p1 p2)
    template <typename A, typename C, typename F>
    static PartT<C> merge3(F f,
                        const PartT<A>& p1,
                        const PartT<A>& p2,
                        const PartT<A>& p3)
    {
        if (p1.empty() || p2.empty() || p3.empty()) {
            throw std::invalid_argument("Part::merge3: one of the partitions is empty");
        }

        
        // Function waiting for v3: g(v3) = f(v1, v2, v3).
        auto p12 = merge2<A, std::function<C(const A&)>>(
            [f](const A& v1, const A& v2) {
                return [f, v1, v2](const A& v3) -> C {
                    return f(v1, v2, v3);
                };
            },
            p1, p2);

        return merge2<std::function<C(const A&)>, A, C>(
            [](const std::function<C(const A&)>& g, const A& v3) {
                return g(v3);
            },
            p12, p3);  
    }
    //########################### split_prod ##############################
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // DMITRIY: Correct assumption that part can be of type std::vector<std::pair<Sub, std::pair<A,B>>>;
    // From OCAml i know that the A in this: std::vector<std::pair<Sub, A>>, must be a pair.
    // From the .mli i can see that for PDT: val split_prod: ('a * 'b) t -> 'a t * 'b t, thus the
    // possibility of the elements in the pair having different types.
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    template <typename A, typename B>
    static std::pair<PartT<A>, PartT<B>> split_prod(const PartT<std::pair<A,B>>& part) {
        auto p1 = Part::map<std::pair<A,B>, A>(part, [](const auto& ab){ return ab.first; });
        auto p2 = Part::map<std::pair<A,B>, B>(part, [](const auto& ab){ return ab.second; });
        return { std::move(p1), std::move(p2) };
    }


    //########################### split_list ##############################
    template <typename A>
    static std::vector<PartT<A>> split_list(const PartT<std::vector<A>>& part) {
        if (part.empty()) {
            return {};
        }
        
        std::vector<Sub> subs; 
        std::vector<std::vector<A>> vs;
        std::size_t width = part.front().second.size();
        subs.reserve(part.size());
        vs.reserve(part.size());
        // let subs = List.map part ~f:fst in
        // let vs = List.map part ~f:snd in
        for (const auto& [sub, v] : part) {
            if (v.size() != width) {
                throw std::invalid_argument("Part::split_list: inconsistent payload widths");
            }
            subs.push_back(sub);
            vs.push_back(v);
        }

        // (List.transpose vs)
        std::vector<std::vector<A>> cols(width, std::vector<A>());
        for (std::size_t j = 0; j < width; ++j) {
            cols[j].reserve(vs.size());
        }
        for (const auto& r : vs) {
            for (std::size_t j = 0; j < width; ++j) {
                cols[j].push_back(r[j]);
            }
        }

        // List.zip_exn subs
        std::vector<PartT<A>> out; out.reserve(width);
        for (std::size_t j = 0; j < width; ++j) {
            PartT<A> pj; pj.reserve(subs.size());
            for (std::size_t i = 0; i < subs.size(); ++i) {
                pj.emplace_back(subs[i], cols[j][i]);
            }
            out.push_back(std::move(pj));
        }
        return out;

    }


    //########################### dedup ##################################
    template <typename A, typename P_EQ>
    static PartT<A> dedup(P_EQ p_eq, const PartT<A>& part) {
        PartT<A> acc;
        acc.reserve(part.size());

        auto aux = [&](PartT<A>& acc, const std::pair<Sub, A>& sv) -> void {
            const auto& [s, v] = sv;

            for (auto it = acc.begin(); it != acc.end(); ++it) {
                if (p_eq(it->second, v)) {
                    it->first = Sub::union_(s, it->first); // same as OCaml: union s t
                    return;
                }
            }
            acc.emplace_back(s, v);
        };

        for (const auto& sv : part) {
            aux(acc, sv);
        }
        return acc;
    }

    //####################### map_dedub ###############################
    template <typename D, typename A, typename P_EQ, typename F>
    static PartT<A> map_dedup(P_EQ p_eq, const PartT<D>& part, F f) {
        auto mapped = Part::map<D, A>(part, f);
        PartT<A> result = Part::dedup(p_eq, mapped);
        return result;
    }


    //####################### map2_dedub ###############################
    template <typename A, typename P_EQ, typename F>
    static PartT<A> map2_dedup(P_EQ p_eq, const PartT<A>& part, F f) {
        auto mapped = Part::map2<A>(part, f);
        PartT<A> result = Part::dedup(p_eq, mapped);
        return result;
    }


    //####################### tabulate_dedup ###########################

    template <typename A, typename P_EQ, typename F>
    static PartT<A> tabulate_dedup(P_EQ p_eq, const Sub::SetT& ds, F f, const A& z) {
        auto tabulate_res = Part::tabulate<A>(ds, f, z);
        PartT<A> result = Part::dedup(p_eq, tabulate_res);
        return result;
    }


    //####################### merge2_dedup ###############################
    /*
    ############## Merge2 Code ###################
    template <typename A, typename B, typename F>
    static PartT<B> merge2(F f, const PartT<A>& part1, const PartT<A>& part2) {


    ########################### dedup ##################################
     template <typename A, typename P_EQ>
    static PartT<A> dedup(P_EQ p_eq, const PartT<A>& part) {
    */


    template <typename A, typename B, typename P_EQ, typename F>
    static PartT<B> merge2_dedup(P_EQ p_eq, F f, const PartT<A>& part1, const PartT<A>& part2) {
        PartT<B> merge2_res = Part::merge2<A, A, B>(f, part1, part2);
        PartT<B> result = Part::dedup(p_eq, merge2_res);
        return result;
    }

    template <typename A1, typename A2, typename B, typename P_EQ, typename F>
    static PartT<B> merge2_dedup(P_EQ p_eq, F f,
                                const PartT<A1>& part1,
                                const PartT<A2>& part2) {
        PartT<B> merged = Part::merge2<A1, A2, B>(f, part1, part2);
        return Part::dedup(p_eq, merged);
    }


    //####################### merge3_dedup ###############################

    template <typename A, typename B, typename P_EQ, typename F>
    static PartT<B> merge3_dedup(P_EQ p_eq, F f, 
                                     const PartT<A>& part1, 
                                     const PartT<A>& part2,
                                     const PartT<A>& part3) {
        PartT<B> merge3_res = Part::merge3<A, B>(f, part1, part2, part3);
        PartT<B> result = Part::dedup(p_eq, merge3_res);
        return result;
    }


    // ####################### split_prod_dedup ###############################
    /*
    template <typename A, typename B>
    static std::pair<PartT<A>, PartT<B>> split_prod(const PartT<std::pair<A,B>>& part) {
        auto p1 = Part::map<std::pair<A,B>, A>(part, [](const auto& ab){ return ab.first; });
        auto p2 = Part::map<std::pair<A,B>, B>(part, [](const auto& ab){ return ab.second; });
        return { std::move(p1), std::move(p2) };
    }
        */


    template <typename A, typename B, typename P_EQ>
    static std::pair<PartT<A>, PartT<B>> split_prod_dedup(P_EQ p_eq, const PartT<std::pair<A,B>>& part) {
        auto [part1, part2] = Part::split_prod<A, B>(part);
        PartT<A> part1_dedup = Part::dedup(p_eq, part1);
        PartT<B> part2_dedup = Part::dedup(p_eq, part2);
        return { std::move(part1_dedup), std::move(part2_dedup) };
    }

    // ####################### split_list_dedup ###############################
    /*
    template <typename A>
    static std::vector<PartT<A>> split_list(const PartT<std::vector<A>>& part) {
        */
    template <typename A, typename P_EQ>
    static std::vector<PartT<A>> split_list_dedup(P_EQ p_eq, const PartT<std::vector<A>>& part) {
        auto parts = Part::split_list<A>(part);
        std::vector<PartT<A>> out;
        out.reserve(parts.size());

        for (const auto& p: parts) {
            out.emplace_back(Part::dedup(p_eq, p));
        }
        return out;
    }


    //####################### sort ###############################

    template <typename A>
    static PartT<A> sort(const PartT<A>& part) {
        if (part.empty()) {
            return PartT<A> {};
        }
        PartT<A> out;
        out.reserve(part.size());
        out.push_back(part.front());


        
        PartT<A> remainder(part.begin() + 1, part.end());
        std::sort(remainder.begin(), remainder.end(), 
            [](const std::pair<Sub, A>&a, const std::pair<Sub, A>& b) {
                auto min_a = Sub::min_elt(a.first);
                auto min_b = Sub::min_elt(b.first);

                return Dom::Less{}(min_a.value(), min_b.value());
            }    
        );

        // reversing the sorted remainder (Ocaml List.rev)
        std::reverse(remainder.begin(), remainder.end());
        
        out.insert(out.end(), remainder.begin(), remainder.end());

        return out;
    }
    /*####################### join_parts ###############################
    ______________________________________________________________________
    OCAML PART:
          let rec join_parts ps = match ps with 
            | [] -> trivial []
            | [p] -> List.map ~f:(fun (sub, x) -> (sub, [x])) p
            | p :: ps -> merge2 (fun x y -> x :: y) p (join_parts ps)
    Notes:
        p = PartT<A> 
        join_parts ps = PartT<std::vector<A>>
    ______________________________________________________________________ 
    merge2 
    multitype c++:
        template <typename A1, typename A2, typename B, typename F>
        static PartT<B> merge2(F f, const PartT<A1>& part1, const PartT<A2>& part2) {
        }
    sametype:
        template <typename A, typename B, typename F>
        static PartT<B> merge2(F f, const PartT<A>& part1, const PartT<A>& part2) {
    ______________________________________________________________________ 

    
    */
    template <typename A>
    static PartT<std::vector<A>> join_parts(const std::vector<PartT<A>>& ps) {
        if (ps.empty()) {
            return Part::trivial<std::vector<A>>(std::vector<A>{});
        }
        if (ps.size() == 1) {
            return Part::map<A, std::vector<A>>(ps.front(), 
                [](const A& x) -> std::vector<A> {
                    return std::vector<A>{x}; 
                });
        } else {
            const auto& p       = ps.front();
            std::vector<PartT<A>> ps_tail(ps.begin() + 1, ps.end());

            auto recursive_result = join_parts<A>(ps_tail);

            return Part::merge2<A, std::vector<A>, std::vector<A>>(
                [](const A& x, const std::vector<A>& y) -> std::vector<A> {
                    std::vector<A> result;
                    result.reserve(y.size() + 1);
                    result.push_back(x);                              // x :: 
                    result.insert(result.end(), y.begin(), y.end()); // y
                    return result;
                },
                p, recursive_result);
        }
    }

    /*####################### merge_parts ###############################
    ______________________________________________________________________
    OCAML PART:
        let merge_parts f ps = 
        let joint_parts = List.map ~f:(fun (sub, xs) -> (sub, f xs)) (join_parts ps) in
        joint_parts

        val merge_parts : ('a list -> 'b) -> 'a t list -> 'b t
    */

    template <typename A, typename B, typename F>
    static PartT<B> merge_parts(F f, const std::vector<PartT<A>>& ps) {
        const auto& ps_join_parts = Part::join_parts<A>(ps);
        return Part::map<std::vector<A>, B>(ps_join_parts, f);
    }





    //####################### el_to_string ###############################
    // let el_to_string indent var f (sub, v) =
    // Printf.sprintf "%s%s ∈ %s\n\n%s" indent (Term.value_to_string var) (Setc.to_string sub) (f v)
    template <typename A, typename F>
    static std::string el_to_string(const std::string& indent, const Term& var, F f, const std::pair<Sub, A>& sub_v) {
        const auto& [sub, v] = sub_v;
        std::ostringstream oss; 
        oss << indent << Term::value_to_string(var);
        oss << " ∈ ";
        oss << Setc::to_string(sub) << "\n\n" << f(v);
        return oss.str();
    }


    //####################### to_string ###############################
    template <typename A, typename F>
    static std::string to_string(const std::string& indent, const Term& var, F f, const PartT<A>& part) {
        std::ostringstream oss;
        if (part.empty()) {
            oss << indent << "❮ · ❯";
        } else if (part.size() == 1) {
            oss << indent << "❮\n\n" << el_to_string(indent, var, f(indent),  part[0]) << "\n" << indent << "❯\n";
        } else {
            oss << indent << "❮\n\n";
            for (const auto& el : part) {
                oss << el_to_string(indent, var, f(indent), el) << "\n\n";
            }
            oss << indent << "❯\n";
        }

        return oss.str();
    }

};

/*
#####################################################################
#####################################################################
                                PDT
#####################################################################
##################################################################### 
- Constructors          : Done                      (TESTED)
- apply1                : Done                      (TESTED)
- apply2                : Done                      (TESTED)
- apply3                : Done                      (TESTED)
- applyN                : Done                      (TESTED)
    - Part::join_parts  : DONE                      (TESTED)
    - Part::merge_parts : DONE                      (TESTED)
- split_prod            : DONE                      (TESTED)
- split_list            : DONE                      (TESTED)
- to_string             : Done                      (TESTED)
- unleaf                : Done                      (TESTED)
- var                   : Done                      (TESTED)
- part                  : Done                      (TESTED)
- papply_list           : Done                      (TESTED)
- is_leaf               : DONE                      (TESTED)
- hide                  : DONE                      (TESTED)
- equal                 : DONE                      (TESTED)
- reduce                : DONE                      (TESTED)                       
- apply1_reduce         : DONE                      (TESTED)
- apply2_reduce         : DONE                      (TESTED)
- split_prod_reduce     : DONE                      (TESTED)
- split_list_reduce     : Needed
- hide_reduce           : Needed
- fst_leaf              : No Needed (used for agg)
- to_latex              : Not needed
- to_light_string       : Not needed
- hide2                 : Not needed
- aux                   : Not Needed
- reorder               : Not Needed
- fold                  : Not Needed
*/

struct Pdt {
    template <typename A> struct Leaf;
    template <typename A> struct Node;

    template <typename A>
    using PdtT = std::variant<Leaf<A>, Node<A>>;


    template <typename A>
    struct Leaf {
        A value;
        explicit Leaf(A v) : value(std::move(v)) {}
    };

    template <typename A>
    struct Node {
        std::string x;
        Part::PartT<PdtT<A>> part; 

        Node(std::string v, Part::PartT<PdtT<A>> p) : x(std::move(v)), part(std::move(p)) {}
    };


    /*
    #####################################################################
                                 Helper Functions
    #####################################################################   
    */

    
    // ############################# isleaf #################################
    // not needed, just for easy reading.
    template <typename A>
    static bool isleaf(const PdtT<A>& pdt) {
        return std::holds_alternative<Leaf<A>>(pdt);
    }

    // ############################# isnode #################################
    // not needed, just for easy reading.
    template <typename A>
    static bool isnode(const PdtT<A>& pdt) {
        return std::holds_alternative<Node<A>>(pdt);
    }


    // ############################# unleaf #################################
    template <typename A>
    static A unleaf(const PdtT<A>& pdt) {
        if (isleaf(pdt)) {
            const auto& leaf = std::get<Leaf<A>>(pdt);
            return leaf.value;
        } else {
            throw std::invalid_argument("Pdt::unleaf: function not defined for nodes");
        }
    }

    // ############################# var #################################
    template <typename A>
    static std::string var(const PdtT<A>& pdt) {
        if (isnode(pdt)) {
            const auto& node = std::get<Node<A>>(pdt);
            const auto& x = node.x;
            return x;
        } else {
            throw std::invalid_argument("Pdt::var: var is underfined for leafs");
        }
    }


    // ############################# part #################################
    template <typename A>
    static Part::PartT<PdtT<A>> part(const PdtT<A>& pdt) {
        if (isnode(pdt)) {
            const auto& node = std::get<Node<A>>(pdt);
            const auto& part = node.part;
            return part;
        } else {
            throw std::invalid_argument("Pdt::part: part is undefined for leafs");
        }
    }


    /*
    #####################################################################
                                    Functions
    #####################################################################   
    */


    // ############################# apply1 #################################
    
    template <typename A, typename B, typename F>
    static PdtT<B> apply1(const std::vector<std::string>& vars, F f, const PdtT<A>& pdt) {
        // Ocaml: | _ , Leaf l -> Leaf (f l)
        if (isleaf(pdt)) {
            const auto& leaf = std::get<Leaf<A>>(pdt);
            return Leaf<B>(f(leaf.value));
        }
        // OCaml: | z :: vars, Node (x, part)
        if (isnode(pdt) && !vars.empty()) {
            // vars elements
            const std::string& z = vars.front();
            std::vector<std::string> remainder(vars.begin() + 1, vars.end());

            // node elements
            const auto& x       = Pdt::var(pdt);
            const auto& part    = Pdt::part(pdt);

            //if String.equal x z then Node (x, Part.map part (apply1 vars f))
            if (x == z) {
            // lambda function for (apply1 vars f) which should be applied
            // to orignal part .
                auto new_part = Part::map<PdtT<A>, PdtT<B>>(part, 
                [&](const PdtT<A>& sub_pdt) -> PdtT<B> {
                    return apply1<A, B, F>(remainder, f, sub_pdt);
                });
                return Node<B>(x, std::move(new_part));
            } else {
                return apply1<A, B, F>(remainder, f, pdt);
            }

        } else {
            throw std::invalid_argument("Pdt::apply1: variable list is empty");
        }
    }


    /* ############################# apply2 #################################
    !!!! NOTES/TO-DO: FIX std::get<Leaf>

    Ocaml:
      let rec apply2 vars f pdt1 pdt2 = match vars, pdt1, pdt2 with
        | _ , Leaf l1, Leaf l2 -> Leaf (f l1 l2)
        | _ , Leaf l1, Node (x, part2) -> Node (x, Part.map part2 (apply1 vars (f l1)))
        | _ , Node (x, part1), Leaf l2 -> Node (x, Part.map part1 (apply1 vars (fun l1 -> f l1 l2)))
        | z :: vars, Node (x, part1), Node (y, part2) ->
        if String.equal x z && String.equal y z then
            Node (z, Part.merge2 (apply2 vars f) part1 part2)
        else (if String.equal x z then
                Node (x, Part.map part1 (fun pdt1 -> apply2 vars f pdt1 (Node (y, part2))))
                else (if String.equal y z then
                        Node (y, Part.map part2 (apply2 vars f (Node (x, part1))))
                    else apply2 vars f (Node (x, part1)) (Node (y, part2))))
        | _ -> raise (Invalid_argument "variable list is empty")

    */
    template <typename A, typename B, typename C, typename F>
    static PdtT<C> apply2(const std::vector<std::string>& vars, F f, const PdtT<A>& pdt1, const PdtT<B>& pdt2) {
        // Ocaml: | _ , Leaf l -> Leaf (f l)
        if (isleaf(pdt1) && isleaf(pdt2)) {
            const auto& leaf1 = std::get<Leaf<A>>(pdt1);
            const auto& leaf2 = std::get<Leaf<B>>(pdt2);
            return Leaf<C>(f(leaf1.value, leaf2.value));
        }

        //     | _ , Leaf l1, Node (x, part2) -> Node (x, Part.map part2 (apply1 vars (f l1)))
        if(isleaf(pdt1) && isnode(pdt2)) {
            const auto& l1    = Pdt::unleaf(pdt1);
            const auto& part2 = Pdt::part(pdt2);
            const auto& x     = Pdt::var(pdt2);
            
            auto new_part = Part::map<PdtT<B>, PdtT<C>>(part2,
                [&](const PdtT<B>& sub_pdt) -> PdtT<C> {
                    auto partial_f = [&](const B& l2) -> C {
                        return f(l1, l2);
                    };
                    return apply1<B, C>(vars, partial_f, sub_pdt);
                });
            
            return Pdt::Node<C>(x, new_part);
        }

        //  | _ , Node (x, part1), Leaf l2 -> Node (x, Part.map part1 (apply1 vars (fun l1 -> f l1 l2)))
        if(isnode(pdt1) && isleaf(pdt2)) {
            const auto& x     = Pdt::var(pdt1);
            const auto& part1 = Pdt::part(pdt1);
            const auto& l2    = Pdt::unleaf(pdt2);
            
            auto new_part = Part::map<PdtT<A>, PdtT<C>>(part1,
                [&](const PdtT<A>& sub_pdt) -> PdtT<C> {
                    // (fun l1 -> f l1 l2)
                    auto partial_f = [&](const A& l1) -> C {
                        return f(l1, l2);
                    };
                    return apply1<A, C>(vars, partial_f, sub_pdt);
                });
            
            return Pdt::Node<C>(x, new_part);
        }

        // OCaml: | z :: vars, Node (x, part)
        if (isnode(pdt1) && isnode(pdt2) && !vars.empty()) {
            // vars elements
            const std::string& z = vars.front();
            std::vector<std::string> vars_tail(vars.begin() + 1, vars.end());

            // node elements
            const auto& x = Pdt::var(pdt1);
            const auto& part1 = Pdt::part(pdt1);
            const auto& y = Pdt::var(pdt2);
            const auto& part2 = Pdt::part(pdt2);

            //OCAML: if String.equal x z && String.equal y z then
            //          Node (z, Part.merge2 (apply2 vars f) part1 part2)
            if (x == z && y == z) {
                auto new_part = Part::merge2<PdtT<A>, PdtT<B>, PdtT<C>>(
                    [&](const PdtT<A>& sub_pdt1, const PdtT<B>& sub_pdt2) -> PdtT<C> {
                        return apply2<A, B, C, F>(vars_tail, f, sub_pdt1, sub_pdt2);
                    },
                    part1, part2);
                
                return Pdt::Node<C>(z, new_part);
            // else (if String.equal x z then
            //   Node (x, Part.map part1 (fun pdt1 -> apply2 vars f pdt1 (Node (y, part2))))
            } else {
                if (x == z) {
                    auto new_part = Part::map<PdtT<A>, PdtT<C>>(part1,
                                [&](const PdtT<A>& sub_pdt1) -> PdtT<C> {
                                    return apply2<A, B, C, F>(vars_tail, f, 
                                                              sub_pdt1, 
                                                              Pdt::Node<B>(y, part2));
                                });
                            
                            return Pdt::Node<C>(x, new_part);
                // else (if String.equal y z then
                //     Node (y, Part.map part2 (apply2 vars f (Node (x, part1))))
                } else {
                    if (y == z) {
                        auto new_part = Part::map<PdtT<B>, PdtT<C>>(part2,
                            [&](const PdtT<B>& sub_pdt2) -> PdtT<C> {
                                return apply2<A, B, C, F>(vars_tail, f, 
                                                          Pdt::Node<A>(x, part1), 
                                                          sub_pdt2);
                            });
                        
                        return Pdt::Node<C>(y, new_part); 
                    } else {
                            return apply2<A, B, C, F>(vars_tail, f, 
                                                      Pdt::Node<A>(x, part1), 
                                                      Pdt::Node<B>(y, part2));
                    }
                }
                
            }

        } else {
            throw std::invalid_argument("Pdt::apply2: variable list is empty");
        }
    }


    // ############################# apply3 #################################
   template <typename A, typename B, typename C, typename D, typename F>
   static PdtT<D> apply3(const std::vector<std::string>& vars, F f, 
                         const PdtT<A>& pdt1, 
                         const PdtT<B>& pdt2, 
                         const PdtT<C>& pdt3) {
        
        auto pdt12 = apply2<A, B, std::function<D(const C&)>>(vars,
        [f](const A& v1, const B& v2) -> std::function<D(const C&)> {
            return [f, v1, v2](const C& v3) -> D {
                return f(v1, v2, v3);
            };
        },
        pdt1, pdt2);


        return apply2<std::function<D(const C&)>, C, D>(
            vars,
            [](const std::function<D(const C&)>& g, const C& v3) -> D {
                return g(v3);
            },
            pdt12, pdt3);
   }

    // ############################# papply_list #################################

    template <typename A, typename B, typename F>
    static B papply_list(F f, 
                        const std::vector<std::optional<A>>& xs,
                        const std::vector<A>& ys) {
        if (xs.empty()) {
            return f(ys);
        }

        if (ys.empty()) {
            throw std::invalid_argument("undefined if the ys list is empty");
        }

        const auto& xs_front = xs.front();
        std::vector<std::optional<A>> xs_tail(xs.begin() + 1, xs.end());

        const auto& ys_front = ys.front();
        std::vector<A> ys_tail(ys.begin() + 1, ys.end());

        if (!xs_front.has_value()) {
            std::function<B(const std::vector<A>&)> new_f = 
                [f, ys_front](const std::vector<A>& zs) -> B {
                    std::vector<A> new_list;
                    new_list.reserve(zs.size() + 1);
                    new_list.push_back(ys_front);
                    new_list.insert(new_list.end(), zs.begin(), zs.end());
                    return f(new_list);
                };
            return papply_list<A, B>(new_f, xs_tail, ys_tail);
        } else {
            const A& x = xs_front.value();
            std::function<B(const std::vector<A>&)> new_f = 
                [f, x](const std::vector<A>& zs) -> B {
                    std::vector<A> new_list;
                    new_list.reserve(zs.size() + 1);
                    new_list.push_back(x);
                    new_list.insert(new_list.end(), zs.begin(), zs.end());
                    return f(new_list);
                };
            return papply_list<A, B>(new_f, xs_tail, ys);
        }
    }

    // f : vector<PdtT<A>> -> PdtT<B>
    template <typename A, typename B>
    static PdtT<B> papply_list_pdt(
        std::function<PdtT<B>(const std::vector<PdtT<A>>&)> f,
        const std::vector<std::optional<PdtT<A>>>& xs,
        const std::vector<PdtT<A>>& ys)
    {
        if (xs.empty()) {
            return f(ys);
        }
        if (ys.empty()) throw std::invalid_argument("undefined if the ys list is empty");

        const auto& xs_front = xs.front();
        std::vector<std::optional<PdtT<A>>> xs_tail(xs.begin() + 1, xs.end());

        const auto& ys_front = ys.front();
        std::vector<PdtT<A>> ys_tail(ys.begin() + 1, ys.end());

        if (!xs_front.has_value()) {
            std::function<PdtT<B>(const std::vector<PdtT<A>>&)> new_f =
                [f, ys_front](const std::vector<PdtT<A>>& zs) -> PdtT<B> {
                    std::vector<PdtT<A>> new_list; new_list.reserve(zs.size() + 1);
                    new_list.push_back(ys_front);
                    new_list.insert(new_list.end(), zs.begin(), zs.end());
                    return f(new_list);
                };
            return papply_list_pdt<A,B>(new_f, xs_tail, ys_tail);
        } else {
            const PdtT<A>& x = *xs_front;
            std::function<PdtT<B>(const std::vector<PdtT<A>>&)> new_f =
                [f, x](const std::vector<PdtT<A>>& zs) -> PdtT<B> {
                    std::vector<PdtT<A>> new_list; new_list.reserve(zs.size() + 1);
                    new_list.push_back(x);
                    new_list.insert(new_list.end(), zs.begin(), zs.end());
                    return f(new_list);
                };
            return papply_list_pdt<A,B>(new_f, xs_tail, ys);
        }
    }

    /* ############################# applyN #################################
    */
    template <typename A, typename B, typename F>
    static PdtT<B> applyN(const std::vector<std::string>& vars, 
                          F f, 
                          std::vector<PdtT<A>>& pdts) {
        if (vars.empty()) {
            std::vector<A> unleaf_list;
            unleaf_list.reserve(pdts.size());

            for (const auto& pdt : pdts) {
                unleaf_list.push_back(Pdt::unleaf(pdt));
            }
            return Leaf<B>(f(unleaf_list));
        } else {

        // OCaml: | z :: vars -> 
        const auto& z = vars.front();
        std::vector<std::string> vars_tail(vars.begin() + 1, vars.end()); 


        //## Multiple of the below list.map can maybe be done in one for loop -> future improvement #############
        
        // OCaml: let f' = papply_list f (List.map ~f:(fun pdt -> if is_leaf pdt then Some (unleaf pdt) else None) pdts) in
        std::vector<std::optional<A>> leaf_optionals;

        for (const auto& pdt : pdts) {
            if (Pdt::isleaf(pdt)) {
                leaf_optionals.push_back(std::make_optional(Pdt::unleaf(pdt)));
            } else {
                leaf_optionals.push_back(std::nullopt);
            }
        }


        std::function<B(const std::vector<A>&)> f_prime = [f, leaf_optionals](const std::vector<A>& ys_values) -> B {
            return Pdt::papply_list<A, B>(f, leaf_optionals, ys_values);
        };

        // OCaml: let nodes = List.filter ~f:(fun pdt -> not (is_leaf pdt)) pdts in
        std::vector<PdtT<A>> nodes;
        nodes.reserve(pdts.size());
        for (const auto& pdt : pdts) {
            if (!Pdt::isleaf(pdt)) {
                nodes.push_back(pdt);
            }
        }

        // OCaml: let other_nodes = List.map ~f:(fun pdt -> if String.equal (var pdt) z then None else Some pdt) nodes in
        std::vector<std::optional<PdtT<A>>> other_nodes;
        other_nodes.reserve(nodes.size());
        for (const auto& pdt : nodes) {
            if (Pdt::var(pdt) == z) {
                other_nodes.push_back(std::nullopt);
            } else {
                other_nodes.push_back(std::make_optional(pdt));
            }
        }

        // OCaml:  let z_parts = List.map (List.filter ~f:(fun pdt -> String.equal (var pdt) z) nodes) ~f:(part)
        std::vector<Part::PartT<PdtT<A>>> z_parts;
        for (const auto& pdt : nodes) {
            if (Pdt::var(pdt) == z) {
                z_parts.push_back(Pdt::part(pdt));
            }
        }


        if (z_parts.empty()) {
            return applyN<A, B>(vars_tail, f_prime, nodes);
        } else {
            // OCaml: fun pdts -> papply_list (applyN vars f') other_nodes pdts
            auto merge_func = [&](const std::vector<PdtT<A>>& pdts) -> PdtT<B> {
                auto f_applyN = [&](const std::vector<PdtT<A>>& zs) -> PdtT<B> {
                    std::vector<PdtT<A>> zs_copy = zs;
                    auto result = applyN<A, B>(vars_tail, f_prime, zs_copy);
                    return result;
                };
                auto result = Pdt::papply_list_pdt<A,B>(f_applyN, other_nodes, pdts);
                return result;
            };

            auto merged_part = Part::merge_parts<PdtT<A>, PdtT<B>>(merge_func, z_parts);
            return Pdt::Node<B>(z, merged_part);
        }

        }

    }

    /*################################ split_prod ################################
    OCaml: val split_prod: ('a * 'b) t -> 'a t * 'b t
    
        
    let rec split_prod = function
        | Leaf (l1, l2) -> (Leaf l1, Leaf l2)
        | Node (x, part) -> let (part1, part2) = Part.split_prod (Part.map part split_prod) in
                            (Node (x, part1), Node (x, part2))


    c++
        auto [part1, part2] = Part::split_prod<A, B>(part);


        template <typename A, typename B>
        static std::pair<PartT<A>, PartT<B>> split_prod(const PartT<std::pair<A,B>>& part) {
            auto p1 = Part::map<std::pair<A,B>, A>(part, [](const auto& ab){ return ab.first; });
            auto p2 = Part::map<std::pair<A,B>, B>(part, [](const auto& ab){ return ab.second; });
            return { std::move(p1), std::move(p2) };
    }
    */
    template <typename A, typename B>
    static std::pair<PdtT<A>, PdtT<B>> split_prod(const PdtT<std::pair<A,B>>& pdt) {
        if (isleaf(pdt)) {
            auto [l1, l2] = Pdt::unleaf(pdt);
            return {Pdt::Leaf<A>(l1), Pdt::Leaf<B>(l2)};
        }

        if (isnode(pdt)) {
            const auto& x = Pdt::var(pdt);
            const auto& part = Pdt::part(pdt);

            auto mapped_part = Part::map<PdtT<std::pair<A,B>>, std::pair<PdtT<A>, PdtT<B>>>(part, 
                [](const PdtT<std::pair<A,B>>& sub_pdt) -> std::pair<PdtT<A>, PdtT<B>> {
                    return split_prod<A, B>(sub_pdt);
                }
            );

            auto [part1, part2] = Part::split_prod<PdtT<A>, PdtT<B>>(mapped_part);

            return {Pdt::Node<A>(x, std::move(part1)), Pdt::Node<B>(x, std::move(part2))};
        }
        throw std::invalid_argument("Pdt::split_prod: invalid PDT variant");
    }


    /*################################ split_list ################################
    val split_list: 'a list t -> 'a t list
      let rec split_list = function
    | Leaf l -> List.map l ~f:(fun el -> Leaf el)
    | Node (x, part) -> List.map (Part.split_list (Part.map part split_list)) ~f:(fun el -> Node (x, el))
    */
    template <typename A>
    static std::vector<PdtT<A>> split_list(const PdtT<std::vector<A>>& list_pdt) {
        if (isleaf(list_pdt)) {
            const auto& leaf_value = unleaf(list_pdt);
            std::vector<PdtT<A>> out;
            for (const auto& l : leaf_value) {
                out.push_back(Leaf<A>(l));
            }
            return out;

        } if (isnode(list_pdt)) {
            const auto& x = Pdt::var(list_pdt);
            const auto& part = Pdt::part(list_pdt);
            

            auto mapped_part = Part::map<PdtT<std::vector<A>>, std::vector<PdtT<A>>>(
                part,
                [](const PdtT<std::vector<A>>& sub_pdt) -> std::vector<PdtT<A>> {
                    return split_list<A>(sub_pdt);
                }
            );


            auto split_parts = Part::split_list(mapped_part);

            std::vector<PdtT<A>> result;
            for (const auto& split_part : split_parts) {
                result.push_back(Node<A>(x, split_part));
            }
            return result;
        }

        throw std::invalid_argument("Pdt::split_list: invalid PDT variant");
    }

    /*################################ hide ################################
    let rec hide vars f_leaf f_node pdt = match vars, pdt with
        |  _ , Leaf l -> Leaf (f_leaf l)
        | [_], Node (_, part) -> Leaf (f_node (Part.map part unleaf))
        | x :: vars, Node (y, part) -> if String.equal x y then
                                        Node (y, Part.map part (hide vars f_leaf f_node))
                                    else hide vars f_leaf f_node (Node (y, part))
        | _ -> raise (Invalid_argument "function not defined for other cases")

        val hide: string list -> ('a -> 'b) -> ('a Part.t -> 'b) -> 'a t -> 'b t
    */
    template <typename A, typename B, typename F_LEAF, typename F_NODE>
    static PdtT<B> hide(const std::vector<std::string>& vars, 
                        F_LEAF f_leaf, 
                        F_NODE f_node, 
                        const PdtT<A>& pdt) {
        if (Pdt::isleaf(pdt)) {
        const auto& l = Pdt::unleaf(pdt);
        return Leaf<B>(f_leaf(l));
        }

        if (Pdt::isnode(pdt) && vars.size() == 1) {    
            const auto& part = Pdt::part(pdt);
            auto mapped_part = Part::map<PdtT<A>, A>(part, 
                [](const PdtT<A>& sub_pdt) -> A {
                    return Pdt::unleaf(sub_pdt);
                });
            return Leaf<B>(f_node(mapped_part));
        }

        if (isnode(pdt) && !vars.empty()) {
            const auto& part    = Pdt::part(pdt);
            const auto& y       = Pdt::var(pdt);
            const auto& x       = vars.front();
            std::vector<std::string> vars_tail(vars.begin() + 1, vars.end());

            if (x == y) {
                auto new_part = Part::map<PdtT<A>, PdtT<B>>(part,
                    [&](const PdtT<A>& sub_pdt) -> PdtT<B> {
                        return Pdt::hide<A, B, F_LEAF, F_NODE>(vars_tail, f_leaf, f_node, sub_pdt);
                    });
                return Node<B>(y, new_part);
            } else {
                return Pdt::hide<A, B, F_LEAF, F_NODE>(vars_tail, f_leaf, f_node, Node<A>(y, part));
            }

        }
        throw std::invalid_argument("Pdt::hide: function not defined for other cases");
    }


    /*################################ equal ################################
    val equal: ('a -> 'a -> bool) -> 'a t -> 'a t -> bool
    let rec equal p_eq pdt1 pdt2 =
        match pdt1, pdt2 with
        | Leaf l1, Leaf l2 -> p_eq l1 l2
        | Node (x, part), Node (x', part') -> String.equal x x' && Int.equal (Part.length part) (Part.length part') &&
                                                List.for_all2_exn part part' ~f:(fun (s, v) (s', v') ->
                                                    Setc.equal s s' && equal p_eq v v')
        | _ -> raise (Invalid_argument "function not defined for other cases")

    */
    template <typename A, typename P_EQ>
    static bool equal(P_EQ p_eq, const PdtT<A>& pdt1, const PdtT<A>& pdt2) {
        if (Pdt::isleaf(pdt1) && Pdt::isleaf(pdt2)) {
            return p_eq(Pdt::unleaf(pdt1), Pdt::unleaf(pdt2));
        }
        if (Pdt::isnode(pdt1) && Pdt::isnode(pdt2)) {
            const auto& x = Pdt::var(pdt1);
            const auto& part = Pdt::part(pdt1);
            const auto& x_prime = Pdt::var(pdt2);
            const auto& part_prime = Pdt::part(pdt2);


            if (x != x_prime || Part::length(part) != Part::length(part_prime)) {
                return false;
            }

            for (size_t i = 0; i < part.size(); ++i) {
                const auto& [s, v] = part[i];
                const auto& [s_prime, v_prime] = part_prime[i];

                if (!Setc::equal(s, s_prime) || !equal(p_eq, v, v_prime)) {
                    return false;
                }
            }
            return true;
        }
        
        throw std::invalid_argument("Pdt::equal: function not defined for other cases");
    }


    /*################################ reduce ################################
    val reduce: ('a -> 'a -> bool) -> 'a t -> 'a t

    let rec reduce p_eq = function
        | Leaf l -> Leaf l
        | Node (x, part) -> Node (x, Part.dedup (equal p_eq) (Part.map part (reduce p_eq)))
    */
    template <typename A, typename P_EQ>
    static PdtT<A> reduce(P_EQ p_eq, const PdtT<A>& pdt) {
        if (Pdt::isleaf(pdt)) {
            return pdt;
        }
        if (Pdt::isnode(pdt)) {
            const auto& x       = Pdt::var(pdt);
            const auto& part    = Pdt::part(pdt);

            // (Part.map part (reduce p_eq))
            auto mapped_part = Part::map<PdtT<A>, PdtT<A>>(part,
                [&p_eq](const PdtT<A>& sub_pdt) -> PdtT<A> {
                    return reduce(p_eq, sub_pdt); 
                });

            //Part.dedup (equal p_eq) (Part.map part (reduce p_eq))
            auto deduped_part = Part::dedup(
                [&p_eq](const PdtT<A>& a, const PdtT<A>& b) -> bool {
                    return Pdt::equal(p_eq, a, b);
                },
                mapped_part);

            return Node<A>(x, deduped_part);
        }
        throw std::invalid_argument("Pdt::reduce: invalid PDT variant");

    }


    /*################################ apply1_reduce ################################
    val apply1_reduce: ('a -> 'a -> bool) -> string list -> ('b -> 'a) -> 'b t -> 'a t

    let rec apply1_reduce p_eq vars f pdt = match vars, pdt with
        | _ , Leaf l -> Leaf (f l)
        | z :: vars, Node (x, part) ->
            if String.equal x z then
                Node (x, Part.map_dedup (equal p_eq) part (apply1_reduce p_eq vars f))
            else apply1_reduce p_eq vars f (Node (x, part))
        | _ -> raise (Invalid_argument "variable list is empty")
    */

    template <typename B, typename A, typename P_EQ, typename F>
    static PdtT<A> apply1_reduce(P_EQ p_eq, 
                              const std::vector<std::string>& vars, 
                              F f,
                              const PdtT<B>& pdt) {
        if (Pdt::isleaf(pdt)) {
            const auto l = Pdt::unleaf(pdt);
            return Leaf<A>(f(l));
        }

        if (Pdt::isnode(pdt) && !vars.empty()) {
            const auto& z = vars.front(); 
            std::vector<std::string> vars_tail(vars.begin() + 1, vars.end());
            const auto x    = Pdt::var(pdt);
            const auto part = Pdt::part(pdt);

            if (x == z) {
                auto mapped_deduped = Part::map_dedup<PdtT<B>, PdtT<A>>(
                    [&p_eq](const PdtT<A>& a, const PdtT<A>& b) -> bool {
                        return Pdt::equal(p_eq, a, b);
                    },
                    part,
                    [&](const PdtT<B>& sub_pdt) -> PdtT<A> {
                        return apply1_reduce<B, A, P_EQ, F>(p_eq, vars_tail, f, sub_pdt);
                    });
                return Node<A>(x, mapped_deduped);
            } else {
                return apply1_reduce<B, A, P_EQ, F>(p_eq, vars_tail, f, Node<B>(x, part));
            }
        }
        throw std::invalid_argument("Pdt::apply1_reduce: variable list is empty");
    }


    /*################################ apply2_reduce ################################
    val apply2_reduce: ('a -> 'a -> bool) -> string list -> ('b -> 'c -> 'a) -> 'b t -> 'c t -> 'a t

    let rec apply2_reduce p_eq vars f pdt1 pdt2 = match vars, pdt1, pdt2 with
        | _ , Leaf l1, Leaf l2 -> Leaf (f l1 l2)
        | _ , Leaf l1, Node (x, part2) -> Node (x, Part.map_dedup (equal p_eq) part2 (apply1_reduce p_eq vars (f l1)))
        | _ , Node (x, part1), Leaf l2 -> Node (x, Part.map_dedup (equal p_eq) part1 (apply1_reduce p_eq vars (fun l1 -> f l1 l2)))
        | z :: vars, Node (x, part1), Node (y, part2) ->
        if String.equal x z && String.equal y z then
            Node (z, Part.merge2_dedup (equal p_eq) (apply2_reduce p_eq vars f) part1 part2)
        else (if String.equal x z then
                Node (x, Part.map_dedup (equal p_eq) part1 (fun pdt1 -> apply2_reduce p_eq vars f pdt1 (Node (y, part2))))
                else (if String.equal y z then
                        Node (y, Part.map_dedup (equal p_eq) part2 (apply2_reduce p_eq vars f (Node (x, part1))))
                    else apply2_reduce p_eq vars f (Node (x, part1)) (Node (y, part2))))
        | _ -> raise (Invalid_argument "variable list is empty")

    */

    template <typename B, typename C, typename A, typename P_EQ, typename F>
    static PdtT<A> apply2_reduce(P_EQ p_eq, 
                              const std::vector<std::string>& vars, 
                              F f,
                              const PdtT<B>& pdt1,
                              const PdtT<C>& pdt2) {
        if (Pdt::isleaf(pdt1) && Pdt::isleaf(pdt2)) {
            const auto l1 = Pdt::unleaf(pdt1);
            const auto l2 = Pdt::unleaf(pdt2);
            return Leaf<A>(f(l1, l2));
        } 
        /*| _ , Leaf l1, Node (x, part2) -> Node (x, Part.map_dedup (equal p_eq) part2 (apply1_reduce p_eq vars (f l1)))
                                  apply1:   Node (x, Part.map_dedup (equal p_eq) part (apply1_reduce p_eq vars f))
        auto new_part = Part::map<PdtT<B>, PdtT<C>>(part2,
                [&](const PdtT<B>& sub_pdt) -> PdtT<C> {
                    auto partial_f = [&](const B& l2) -> C {
                        return f(l1, l2);
                    };
                    return apply1<B, C>(vars, partial_f, sub_pdt);
                });
            
            return Pdt::Node<C>(x, new_part);*/
        if (Pdt::isleaf(pdt1) && Pdt::isnode(pdt2)) {
            const auto& l1 = Pdt::unleaf(pdt1);
            const auto& x = Pdt::var(pdt2);
            const auto& part2 = Pdt::part(pdt2);
            
            auto mapped_deduped = Part::map_dedup<PdtT<C>, PdtT<A>>(
                [&p_eq](const PdtT<A>& a, const PdtT<A>& b) -> bool {
                    return Pdt::equal(p_eq, a, b);
                },
                part2,
                [&](const PdtT<C>& sub_pdt) -> PdtT<A> {
                    // Create partial function that takes a C and returns an A
                    auto partial_f = [&](const C& l2) -> A {
                        return f(l1, l2);
                    };
                    // KEEP VERSION IF ERROR:
                    return apply1_reduce<C, A, P_EQ>(p_eq, vars, partial_f, sub_pdt);
                }
            );
            return Node<A>(x, mapped_deduped);
        }

        /*| _ , Node (x, part1), Leaf l2 -> Node (x, Part.map_dedup (equal p_eq) part1 (apply1_reduce p_eq vars (fun l1 -> f l1 l2)))
            Same as the previous. Few differen, partial_f now auto partial_f = [&](const B& l1) -> A 
        */
        if (Pdt::isnode(pdt1) && Pdt::isleaf(pdt2)) {
            const auto& x = Pdt::var(pdt1);
            const auto& part1 = Pdt::part(pdt1);
            const auto& l2 = Pdt::unleaf(pdt2);
            
            auto mapped_deduped = Part::map_dedup<PdtT<B>, PdtT<A>>(
                [&p_eq](const PdtT<A>& a, const PdtT<A>& b) -> bool {
                    return Pdt::equal(p_eq, a, b);
                },
                part1,
                [&](const PdtT<B>& sub_pdt) -> PdtT<A> {
                    auto partial_f = [&](const B& l1) -> A {
                        return f(l1, l2);
                    };
                    return apply1_reduce<B, A, P_EQ>(p_eq, vars, partial_f, sub_pdt);
                }
            );
            return Node<A>(x, mapped_deduped);
        }

        /*| z :: vars, Node (x, part1), Node (y, part2) ->
            if String.equal x z && String.equal y z then
                Node (z, Part.merge2_dedup (equal p_eq) (apply2_reduce p_eq vars f) part1 part2)
            else (if String.equal x z then
                    Node (x, Part.map_dedup (equal p_eq) part1 (fun pdt1 -> apply2_reduce p_eq vars f pdt1 (Node (y, part2))))
                    else (if String.equal y z then
                            Node (y, Part.map_dedup (equal p_eq) part2 (apply2_reduce p_eq vars f (Node (x, part1))))
                        else apply2_reduce p_eq vars f (Node (x, part1)) (Node (y, part2))))
            | _ -> raise (Invalid_argument "variable list is empty")
        */
        if (isnode(pdt1) && isnode(pdt2) && !vars.empty()) {
            // vars elements
            const std::string& z = vars.front();
            std::vector<std::string> vars_tail(vars.begin() + 1, vars.end());

            // node elements
            const auto x        = Pdt::var(pdt1);
            const auto part1    = Pdt::part(pdt1);
            const auto y        = Pdt::var(pdt2);
            const auto part2    = Pdt::part(pdt2);

            /*apply2_reduce:
                if String.equal x z && String.equal y z then
                    Node (z, Part.merge2_dedup (equal p_eq) (apply2_reduce p_eq vars f) part1 part2)
             apply2:
                if String.equal x z && String.equal y z then
                    Node (z, Part.merge2 (apply2 vars f) part1 part2)       
                    
                    */
            if (x == z && y == z) {
                // Node (z, Part.merge2_dedup (equal p_eq) (apply2_reduce p_eq vars f) part1 part2)
                auto merged_deduped = Part::merge2_dedup<PdtT<B>, PdtT<C>, PdtT<A>>(
                    [p_eq](const PdtT<A>& a, const PdtT<A>& b) -> bool {
                        return Pdt::equal(p_eq, a, b);
                    },
                    [p_eq, f, vars_tail](const PdtT<B>& sub1, const PdtT<C>& sub2) -> PdtT<A> {
                        return apply2_reduce<B, C, A>(p_eq, vars_tail, f, sub1, sub2);
                    },
                    part1, part2);

                return Node<A>(z, merged_deduped);

            } else {
                if (x == z) {
                    auto mapped_deduped = Part::map_dedup<PdtT<B>, PdtT<A>>(
                        [&p_eq](const PdtT<A>& a, const PdtT<A>& b) -> bool {
                            return Pdt::equal(p_eq, a, b);
                        },
                        part1,
                        [&](const PdtT<B>& sub_pdt1) -> PdtT<A> {
                            return apply2_reduce<B, C, A, P_EQ, F>(
                                p_eq, vars_tail, f, 
                                sub_pdt1, 
                                Node<C>(y, part2));
                        });
                    
                    return Node<A>(x, mapped_deduped);
                } else {
                    if (y == z) {
                        auto mapped_deduped = Part::map_dedup<PdtT<C>, PdtT<A>>(
                            [&p_eq](const PdtT<A>& a, const PdtT<A>& b) -> bool {
                                return Pdt::equal(p_eq, a, b);
                            },
                            part2,
                            [&](const PdtT<C>& sub_pdt2) -> PdtT<A> {
                                return apply2_reduce<B, C, A, P_EQ, F>(
                                    p_eq, vars_tail, f,
                                    Node<B>(x, part1),
                                    sub_pdt2);
                            });
                        
                        return Node<A>(y, mapped_deduped);
                    } else {
                        return apply2_reduce<B, C, A, P_EQ, F>(
                            p_eq, vars_tail, f,
                            Node<B>(x, part1),
                            Node<C>(y, part2));
                    }
                }
            }
        } else {
            throw std::invalid_argument("Pdt::apply2_reduce: variable list is empty");
        }

    }


    /*################################ split_prod_reduce ################################
    val split_prod_reduce: ('a -> 'a -> bool) -> ('a * 'a) t -> 'a t * 'a t
    let rec split_prod_reduce p_eq = function
        | Leaf (l1, l2) -> (Leaf l1, Leaf l2)
        | Node (x, part) -> let (part1, part2) = Part.split_prod_dedup (equal p_eq) (Part.map part (split_prod_reduce p_eq)) in
                            (Node (x, part1), Node (x, part2))
    */

    template <typename A, typename P_EQ>
    static std::pair<Pdt::PdtT<A>, Pdt::PdtT<A>>
    split_prod_reduce(P_EQ p_eq, const Pdt::PdtT<std::pair<A,A>>& pdt) {
        // Leaf (l1, l2) -> (Leaf l1, Leaf l2)
        if (Pdt::isleaf(pdt)) {
            auto [l1, l2] = Pdt::unleaf(pdt);
            return { Pdt::Leaf<A>(l1), Pdt::Leaf<A>(l2) };
        }

        /*  Node (x, part) ->
        //   let (part1, part2) =
        //     Part.split_prod_dedup (equal p_eq)
        //       (Part.map part (split_prod_reduce p_eq))
        //   in (Node (x, part1), Node (x, part2))
        */
        if (Pdt::isnode(pdt)) {
            const auto& x    = Pdt::var(pdt);
            const auto& part = Pdt::part(pdt);

            auto mapped = Part::map<Pdt::PdtT<std::pair<A,A>>, std::pair<Pdt::PdtT<A>, Pdt::PdtT<A>>>(
                part,
                [&p_eq](const Pdt::PdtT<std::pair<A,A>>& sub) -> std::pair<Pdt::PdtT<A>, Pdt::PdtT<A>> {
                    return split_prod_reduce<A, P_EQ>(p_eq, sub);
                }
            );

            auto [part1, part2] =
                Part::split_prod_dedup<Pdt::PdtT<A>, Pdt::PdtT<A>>(
                    [&p_eq](const Pdt::PdtT<A>& a, const Pdt::PdtT<A>& b) -> bool {
                        return Pdt::equal(p_eq, a, b);
                    },
                    mapped
                );

            return { Pdt::Node<A>(x, part1),
                    Pdt::Node<A>(x, part2) };
        }

        throw std::invalid_argument("Pdt::split_prod_reduce: invalid PDT variant");
    }

    /*################################ split_list_reduce ################################
    val split_list_reduce: ('a -> 'a -> bool) -> 'a list t -> 'a t list
    let rec split_list_reduce p_eq = function
        | Leaf l -> List.map l ~f:(fun el -> Leaf el)
        | Node (x, part) -> List.map (Part.split_list_dfedup (equal p_eq) (Part.map part (split_list_reduce p_eq)))
                            ~f:(fun el -> Node (x, el))
    */

    template <typename A, typename P_EQ>
    static std::vector<PdtT<A>> split_list_reduce(P_EQ p_eq, 
                                                  const PdtT<std::vector<A>>& list_pdt) {
        if (isleaf(list_pdt)) {
           const auto& leaf_value = unleaf(list_pdt);
           std::vector<Pdt::PdtT<A>> out;
           out.reserve(leaf_value.size());
           for (const auto& l : leaf_value) {
               out.push_back(Leaf<A>(l));
           }
           return out;
        }

        if (isnode(list_pdt)) {
            const auto x    = Pdt::var(list_pdt);
            const auto part = Pdt::part(list_pdt);

            // Part.map part (split_list_reduce p_eq)
            auto mapped = Part::map<PdtT<std::vector<A>>, std::vector<PdtT<A>>>(
                part,
                [&p_eq](const PdtT<std::vector<A>>& sub) -> std::vector<PdtT<A>> {
                    return split_list_reduce<A, P_EQ>(p_eq, sub);
                }
            );

            // Part.split_list_dedup (equal p_eq) mapped
            auto parts = Part::split_list_dedup<PdtT<A>>(
                [&p_eq](const PdtT<A>& u, const PdtT<A>& v) -> bool {
                    return Pdt::equal(p_eq, u, v);
                },
                mapped
            );

            // List.map parts ~f:(fun el -> Node (x, el))
            std::vector<PdtT<A>> result;
            result.reserve(parts.size());
            for (const auto& p : parts) {
                result.push_back(Node<A>(x, p));
            }
            return result;
        }
        throw std::invalid_argument("Pdt::split_list_reduce: invalid PDT variant");
    }


    /*################################ hide_reduce ################################
    val hide_reduce: ('a -> 'a -> bool) -> string list -> ('b -> 'a) -> ('b Part.t -> 'a) -> 'b t -> 'a t
    let rec hide_reduce p_eq vars f_leaf f_node pdt = match vars, pdt with
        |  _ , Leaf l -> Leaf (f_leaf l)
        | [_], Node (_, part) -> Leaf (f_node (Part.map part unleaf))
        | x :: vars, Node (y, part) -> if String.equal x y then
                                        Node (y, Part.map_dedup (equal p_eq) part (hide_reduce p_eq vars f_leaf f_node))
                                    else hide_reduce p_eq vars f_leaf f_node (Node (y, part))
        | _ -> raise (Invalid_argument "function not defined for other cases")
    */

    template <typename B, typename A, typename P_EQ, typename F_LEAF, typename F_NODE>
    static PdtT<A> hide_reduce(P_EQ p_eq, 
                            const std::vector<std::string>& vars,
                            F_LEAF f_leaf,
                            F_NODE f_node,
                            const PdtT<B>& pdt) {
        // | _, Leaf l -> Leaf (f_leaf l)
        if (Pdt::isleaf(pdt)) {
            const auto& l = Pdt::unleaf(pdt);
            return Pdt::Leaf<A>(f_leaf(l));
        }


        // | [_], Node (_, part) -> Leaf (f_node (Part.map part unleaf))
        if (Pdt::isnode(pdt) && vars.size() == 1) {
            const auto& part = Pdt::part(pdt);
            auto mapped_part = Part::map<PdtT<B>, B>(part,
                [](const PdtT<B>& sub_pdt) -> B {
                    return Pdt::unleaf(sub_pdt);
                });
            return Pdt::Leaf<A>(f_node(mapped_part));
        }

        /*  | x :: vars, Node (y, part) -> if x = y then
                Node (y, Part.map_dedup (equal p_eq) part (hide_reduce p_eq vars f_leaf f_node))
             else hide_reduce p_eq vars f_leaf f_node (Node (y, part))
        */
        if (Pdt::isnode(pdt) && !vars.empty()) {
            const auto& y    = Pdt::var(pdt);
            const auto& part = Pdt::part(pdt);
            const auto& x    = vars.front();
            std::vector<std::string> vars_tail(vars.begin() + 1, vars.end());

            if (x == y) {
                auto mapped_deduped = Part::map_dedup<PdtT<B>, PdtT<A>>(

                    [&p_eq](const PdtT<A>& u, const PdtT<A>& v) -> bool {
                        return Pdt::equal(p_eq, u, v);
                    },
                    part,

                    [&p_eq, &vars_tail, f_leaf, f_node](const PdtT<B>& sub_pdt) -> PdtT<A> {
                        return hide_reduce<B, A, P_EQ, F_LEAF, F_NODE>(p_eq, vars_tail, f_leaf, f_node, sub_pdt);
                    }
                );
                return Pdt::Node<A>(y, mapped_deduped);
            } else {
                return hide_reduce<B, A, P_EQ, F_LEAF, F_NODE>(p_eq, vars_tail, f_leaf, f_node,
                                                            Pdt::Node<B>(y, part));
            }
        }

        // | _ -> raise ...
        throw std::invalid_argument("Pdt::hide_reduce: function not defined for other cases");
    }






    /*
    #####################################################################
                                Construction Functions
    #####################################################################    
    */

    /*################################ pdt_of ################################
    
    OCaml reference:
      let rec pdt_of tp r trms (vars: string list) maps : Expl.t = match vars with
        | [] -> if List.is_empty maps then Leaf (V (VPred (tp, r, trms)))
                else Leaf (S (SPred (tp, r, trms)))
        | x :: vars ->
           let ds = List.fold maps ~init:[]
                      ~f:(fun acc map -> match Map.find map x with
                                         | None -> acc
                                         | Some(d) -> d :: acc) in
           let find_maps d = List.fold maps ~init:[]
                               ~f:(fun acc map -> match Map.find map x with
                                                  | None -> acc
                                                  | Some(d') -> if Dom.equal d d' then
                                                                  map :: acc
                                                                else acc) in
           let part = Part.tabulate_dedup (Pdt.equal Proof.equal) (Set.of_list (module Dom) ds)
                        (fun d -> pdt_of tp r trms vars (find_maps d)) 
                        (pdt_of tp r trms vars []) in
           Node (x, part)
    */
    static PdtT<int> pdt_of(const std::vector<std::string>& vars,
                            const std::vector<std::unordered_map<std::string, Dom>>& maps) {
        // Base case: no variables left
        // OCaml: | [] -> if List.is_empty maps then Leaf (V ...) else Leaf (S ...)
        if (vars.empty()) {
            if (maps.empty()) {
                return Leaf<int>(0);  // No matches found
            } else {
                return Leaf<int>(1);  // Matches found
            }
        }

        // Recursive case: split first variable
        // OCaml: | x :: vars ->
        const std::string& x = vars.front();
        std::vector<std::string> vars_tail(vars.begin() + 1, vars.end());

        // Collect all domain values for variable x across all maps
        // OCaml: let ds = List.fold maps ~init:[] ~f:(fun acc map -> match Map.find map x with ...)
        // Then: Set.of_list (module Dom) ds
        Setc::SetT ds;  // Using Setc::SetT (std::set<Dom>) for automatic deduplication
        for (const auto& map : maps) {
            auto it = map.find(x);
            if (it != map.end()) {
                ds.insert(it->second);
            }
        }

        // Define find_maps function: filter maps where map[x] == d
        // OCaml: let find_maps d = List.fold maps ~init:[] ~f:(fun acc map -> ...)
        auto find_maps = [&](const Dom& d) -> std::vector<std::unordered_map<std::string, Dom>> {
            std::vector<std::unordered_map<std::string, Dom>> filtered;
            for (const auto& map : maps) {
                auto it = map.find(x);
                if (it != map.end() && Dom::equal(it->second, d)) {
                    filtered.push_back(map);
                }
            }
            return filtered;
        };

        // Tabulate function: for each domain value d, recursively build PDT
        // OCaml: (fun d -> pdt_of tp r trms vars (find_maps d))
        auto tabulate_func = [&](const Dom& d) -> PdtT<int> {
            return pdt_of(vars_tail, find_maps(d));
        };

        // Zero case: PDT when no maps (empty assignment list)
        // OCaml: (pdt_of tp r trms vars [])
        PdtT<int> zero_pdt = pdt_of(vars_tail, {});

        // Build partition using tabulate_dedup
        // OCaml: Part.tabulate_dedup (Pdt.equal Proof.equal) (Set.of_list (module Dom) ds) ...
        auto pdt_eq = [](const PdtT<int>& a, const PdtT<int>& b) -> bool {
            auto int_eq = [](int x, int y) -> bool { return x == y; };
            return Pdt::equal(int_eq, a, b);
        };

        auto partition = Part::tabulate_dedup<PdtT<int>>(
            pdt_eq,
            ds,
            tabulate_func,
            zero_pdt
        );

        // Return node with variable name and partition
        // OCaml: Node (x, part)
        return Node<int>(x, partition);
    }


    /*
    #####################################################################
                                Auxiliary Function
    #####################################################################    
    */








    /*
    ################################ to_string ################################
    */
    template <typename A, typename F>
    static std::string to_string(F f, const std::string& indent, const PdtT<A>& pdt) {
        std::string indent_prime = "    " + indent;
        std::ostringstream out;

        if (isleaf(pdt)) {
            const auto& leaf_val = unleaf(pdt);
            out << indent_prime << "❮\n\n" << f(indent_prime, leaf_val) << "\n" << indent_prime << "❯";
            return out.str();
        }

        if (isnode(pdt)) {
            const auto& x       = Pdt::var(pdt);
            const auto& part    = Pdt::part(pdt);
            
            auto part_result = Part::to_string(indent_prime, Term::Var(x), 
                [&f](const std::string& ind) {
                    return [&f, ind](const PdtT<A>& sub_pdt) {
                        return to_string(f, ind, sub_pdt);
                    };
                }, 
                part);
            
            out << part_result;
            return out.str();
        }

        throw std::invalid_argument("Pdt::to_string: invalid variant");
    }

};


#endif // __PDT_H__