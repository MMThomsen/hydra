#include <cstdio>
#include <cstdlib>
#include <exception>
#include <stdexcept>

#include "constants.h"
#include "common.h"
#include "formula.h"
#include "parser.h"
#include "lexer.h"
#include "util.h"
#include "visitors.h"
#include "pdt.h"

#include "monitor.h"
#include "trie.h"

Formula *getAST(const char *formula)
{
    Formula *fmla;
    yyscan_t scanner;
    YY_BUFFER_STATE state;
 
    if (yylex_init(&scanner)) return NULL;
 
    state = yy_scan_string(formula, scanner);
 
    if (yyparse(&fmla, scanner)) return NULL;
 
    yy_delete_buffer(state, scanner);
    yylex_destroy(scanner);
 
    return fmla;
}

void printUsage()
{
    fprintf(stderr, "hydra MDL LOG [-grep] [-mdlaerial]\n");
    exit(EXIT_FAILURE);
}

struct TimePoint {
    timestamp ts;
    int tp;
    int off;

    TimePoint() : tp(-1) {}
    void update(timestamp new_ts) {
        if (new_ts > ts || tp == -1) {
            off = 0;
        } else {
            off++;
        }
        ts = new_ts;
        tp++;
    }
};
 
int main(int argc, char **argv)
{
    if (argc < 3) {
        printUsage();
    }
    for (int i = 3; i < argc; i++) {
        if (!strcmp(argv[3], "-grep")) {
            grep = 1;
        }
        if (!strcmp(argv[3], "-mdlaerial")) {
            mdlaerial = 1;
        }
    }

    FILE *mtl = fopen(argv[1], "r");
    if (mtl == NULL) {
        fprintf(stderr, "Error: formula file open\n");
        exit(EXIT_FAILURE);
    }
    char *line = NULL;
    size_t length = 0;
    if (getline(&line, &length, mtl) == -1) {
        fprintf(stderr, "Error: formula file read\n");
        exit(EXIT_FAILURE);
    }
    fclose(mtl);
    Formula *fmla;
    try {
        fmla = getAST(line);
    } catch(const std::runtime_error &e) {
        fprintf(stderr, "Error: %s\n", e.what());
        exit(EXIT_FAILURE);
    }
    free(line);

    InputReader *input_reader;

    if (grep) input_reader = new GrepInputReader(argv[2]);
    else input_reader = new MapInputReader(argv[2], &trie);

    std::vector<std::string> free_vars = fmla->free_variables();
    MonitorVisitor mv(input_reader, free_vars);
    fmla->accept(mv);
    Monitor *mon = mv.get_mon();
    TimePoint tp;
    
    do {
        try {
            BooleanVerdict v = mon->step(free_vars); 
            tp.update(v.ts);
            if (grep) {
                if (has_true_leaf(v.b)) printf("%d\n", tp.tp);
            } else {
                auto booleanToBool_printer = [](const std::string& indent, Boolean val) -> std::string {
                    return indent + (val == TRUE ? "true" : "false");
                };
                printf("%d:%d\nExplanation:\n%s\n", tp.ts, tp.off, Pdt::to_string(booleanToBool_printer, "", v.b).c_str());
            }
        } catch (const EOL &e) {
            break;
        }
    } while(true);

    delete fmla;
    delete mon;
    delete input_reader;

    return 0;
}
