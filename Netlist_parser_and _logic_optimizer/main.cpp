#include "netlist_optimizer.h"
int main() {
    NetlistOptimizer opt;
    opt.load_netlist("netlist.txt"); // Assume input file
    opt.optimize();
    opt.print_optimized();
    return 0;
}