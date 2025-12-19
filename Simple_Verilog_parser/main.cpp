// main.cpp
#include "verilog_parser.h"
#include <iostream>

int main(int argc, char* argv[]) {
    using namespace verilog_parser;
    
    VerilogParser parser;
    
    // Example 1: Parse a simple Verilog string
    std::cout << "=== Example 1: Simple AND-OR circuit ===\n";
    
    const char* verilog_code = R"(
// Simple 2-bit AND-OR circuit
module simple_circuit(a, b, c, d, out);
    input a, b, c, d;
    output out;
    wire w1, w2;
    
    and u1(w1, a, b);
    or u2(w2, c, d);
    and u3(out, w1, w2);
endmodule
)";
    
    if (parser.parse_string(verilog_code)) {
        std::cout << "Parsed successfully!\n\n";
        parser.print_module_summary();
        
        std::cout << "\n=== JSON Output ===\n";
        parser.print_netlist_json();
        
        std::cout << "\n=== DOT Graph Output ===\n";
        parser.print_netlist_dot();
    }
    
    std::cout << "\n\n=== Example 2: Full Adder ===\n";
    
    const char* full_adder = R"(
module full_adder(a, b, cin, sum, cout);
    input a, b, cin;
    output sum, cout;
    wire s1, s2, s3;
    
    xor g1(s1, a, b);
    xor g2(sum, s1, cin);
    and g3(s2, a, b);
    and g4(s3, s1, cin);
    or g5(cout, s2, s3);
endmodule
)";
    
    VerilogParser parser2;
    if (parser2.parse_string(full_adder)) {
        parser2.print_module_summary();
    }
    
    // Example 3: Parse from file if provided
    if (argc > 1) {
        std::cout << "\n=== Parsing file: " << argv[1] << " ===\n";
        VerilogParser file_parser;
        if (file_parser.parse_file(argv[1])) {
            file_parser.print_module_summary();
        }
    }
    
    return 0;
}