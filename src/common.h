#ifndef __COMMON_H__
#define __COMMON_H__

#include "trie.h"
#include "util.h"
#include "dom.h" 

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <optional>
#include <vector>
#include <exception>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
using namespace std;

struct EOL : std::exception {};

struct Event {
    size_t pos;
    timestamp ts;
    int tp;
    int eof;

    int c;
    int ap_cnt;
    vector<vector<vector<Dom>>> ap_lookup;  // [pred_id][tuple_index][arg_position]

    //Event(int ap_cnt = 0) : pos(0), ts(0), tp(-1), eof(0), c(-1), ap_cnt(ap_cnt), ap_lookup(ap_cnt) {}
    Event(int ap_cnt = 0) : pos(0), ts(0), tp(-1), eof(0), c(-1), ap_cnt(ap_cnt) {
    ap_lookup.resize(ap_cnt);
    }
    Event(const Event *e) : pos(e->pos), ts(e->ts), tp(e->tp), eof(e->eof), c(e->c), ap_cnt(e->ap_cnt), ap_lookup(e->ap_lookup) {}
    bool operator<(const Event &e) const {
        return c < e.c || (c == e.c && ap_lookup < e.ap_lookup);
    }
    Event *clone() const {
        return new Event(this);
    }
    int eval(int fid) const {
        CHECK(0 <= fid && fid < ap_cnt);
        return !ap_lookup[fid].empty() ? 1 : 0;
    }                                           // also args
    int evalAtom(const char *pred_name, int pred) const { // It is here that we can use tabulate function to create part -> pdt
        if (c == -1) {
            CHECK(0 <= pred && pred < ap_cnt);
            return !ap_lookup[pred].empty() ? 1 : 0;
        } else {
            return c == pred_name[0];
        }
    }
};

class InputReader {
public:
    virtual ~InputReader() {}

    virtual Event *open_handle() = 0;
    virtual void read_handle(Event *e) = 0;
};

class MapInputReader : public InputReader {
    Trie *trie;

    int fd;
    struct stat f_stat;
    size_t f_size;
    const char *mapped;

    int fsm(const char *line, size_t *pos) {
        TrieNode<int> *t = &trie->root;
        size_t i = *pos;                                                          // Added below -  Stop at '(' too    
        while (i < f_size && line[i] != ' ' && line[i] != '\r' && line[i] != '\n' && line[i] != '(') {  
            if (line[i] & 0x80) throw std::runtime_error("log file format");
            if (t->next[line[i]] == NULL) {
                *pos = i;
                return -1;
            }
            t = t->next[line[i++]];
        }
        *pos = i;
        return t->value;
    }

public:
    MapInputReader(const char *fname, Trie *trie) : trie(trie) {
        fd = open(fname, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "Error: log file open\n");
            exit(EXIT_FAILURE);
        }
        int status = fstat(fd, &f_stat);
        CHECK(status >= 0);
        f_size = f_stat.st_size;
        mapped = (char *)mmap(NULL, f_size, PROT_READ, MAP_PRIVATE, fd, 0);
    }
    ~MapInputReader() override {
        munmap((void *)mapped, f_size);
        close(fd);
    }

    Event *open_handle() override {
        return new Event(trie->cnt);
    }
    void read_handle(Event *e) override {
        CHECK(e != NULL);
        if (e->eof) return;
        size_t pos = e->pos;
        timestamp ts;
        while (pos < f_size && (mapped[pos] == '\r' || mapped[pos] == '\n')) pos++;
        if (pos == f_size) {
            e->eof = 1;
            e->tp++;
            return;
        }
        if (mapped[pos++] != '@') throw std::runtime_error("log file format");
        if (parseNumber(mapped, &pos, &ts)) throw std::runtime_error("timestamp");
        e->ts = ts;
        e->tp++;
        //for (int i = 0; i < e->ap_cnt; i++) e->ap_lookup[i] = 0;
        for (int i = 0; i < e->ap_cnt; i++) {
            e->ap_lookup[i].clear();  // Clear tuple list for each predicate
        }
        while (pos < f_size && mapped[pos] != '\r' && mapped[pos] != '\n') {
            if (mapped[pos] == ' ') {
                pos++;
            } else {
                int value = fsm(mapped, &pos);
                if (value == -1) {
                    while(pos < f_size && mapped[pos] != ' ' && mapped[pos] != '\r' && mapped[pos] != '\n') pos++;
                } else {
                    //e->ap_lookup[value] = 1;           Change Structure to store tuples
                    vector<Dom> tuple_args;              // Arguments for this tuple

                    // Added - parse arguments 
                    if (pos < f_size && mapped[pos] == '(') {
                        pos++;

                        while (pos < f_size && mapped[pos] != ')') {
                            while (pos < f_size && mapped[pos] == ' ') pos++;
                           
                            // Capture argument value
                            size_t arg_start = pos;
                            while (pos < f_size && mapped[pos] != ',' && 
                                    mapped[pos] != ')' && mapped[pos] != ' ') {
                                pos++;
                            }

                            if (pos > arg_start) {
                                // Extract argument string
                                string arg_str(mapped + arg_start, pos - arg_start);
                                
                                // Try to parse as integer, then float, else string
                                try {
                                    size_t idx;
                                    int int_val = std::stoi(arg_str, &idx);
                                    if (idx == arg_str.length()) {
                                        tuple_args.push_back(Dom::Int(int_val));
                                    } else {
                                        throw std::invalid_argument("not an int");
                                    }
                                } catch (...) {
                                    try {
                                        size_t idx;
                                        double float_val = std::stod(arg_str, &idx);
                                        if (idx == arg_str.length()) {
                                            tuple_args.push_back(Dom::Float(float_val));
                                        } else {
                                            throw std::invalid_argument("not a float");
                                        }
                                    } catch (...) {
                                        // Default to string
                                        tuple_args.push_back(Dom::Str(arg_str));
                                    }
                                }
                            }                          
                           
                           while (pos < f_size && mapped[pos] == ' ') pos++;
                           
                           if (pos < f_size && mapped[pos] == ',') {
                               pos++;
                           }
                       }
                       
                       if (pos < f_size && mapped[pos] == ')') {
                           pos++;
                       }
                   } 
                   
                   // Store the tuple for this predicate
                   e->ap_lookup[value].push_back(tuple_args);
               }
           }
        }
        while (pos < f_size && (mapped[pos] == '\r' || mapped[pos] == '\n')) pos++;
        e->pos = pos;
    }
};

class GrepInputReader : public InputReader {
    int fd;
    struct stat f_stat;
    size_t f_size;
    const char *mapped;

public:
    GrepInputReader(const char *fname) {
        fd = open(fname, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "Error: input file open\n");
            exit(EXIT_FAILURE);
        }
        int status = fstat(fd, &f_stat);
        CHECK(status >= 0);
        f_size = f_stat.st_size;
        mapped = (char *)mmap(NULL, f_size, PROT_READ, MAP_PRIVATE, fd, 0);
    }
    ~GrepInputReader() override {
        munmap((void *)mapped, f_size);
        close(fd);
    }

    Event *open_handle() override {
        return new Event(0);
    }
    void read_handle(Event *e) override {
        CHECK(e != NULL);
        if (e->eof) return;
        if (e->pos == f_size) {
            e->eof = 1;
            e->tp++;
            return;
        }
        e->ts++;
        e->tp++;
        e->c = mapped[e->pos++];
    }
};

#endif /* __COMMON_H__ */
