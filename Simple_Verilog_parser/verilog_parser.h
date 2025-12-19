// verilog_parser.h
#ifndef VERILOG_PARSER_H
#define VERILOG_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <set>

namespace verilog_parser {

// Forward declarations
struct NetlistNode;
struct Connection;

// Enum for different gate types
enum class GateType {
    AND, OR, XOR, NAND, NOR, XNOR, NOT, BUF,
    INPUT, OUTPUT, WIRE, MODULE
};

// Structure representing a gate/instance
struct Gate {
    std::string name;
    GateType type;
    std::vector<std::string> ports;  // Ordered ports
    std::map<std::string, std::string> connections;  // port -> net
    
    Gate(const std::string& n, GateType t) : name(n), type(t) {}
    
    void add_connection(const std::string& port, const std::string& net) {
        connections[port] = net;
    }
};

// Structure representing a module
struct Module {
    std::string name;
    std::vector<std::string> ports;
    std::map<std::string, Gate> gates;
    std::set<std::string> wires;
    std::set<std::string> inputs;
    std::set<std::string> outputs;
    
    Module(const std::string& n) : name(n) {}
    
    void add_gate(const Gate& gate) {
        gates[gate.name] = gate;
    }
    
    void add_wire(const std::string& wire) {
        wires.insert(wire);
    }
    
    void add_input(const std::string& input) {
        inputs.insert(input);
        wires.insert(input);  // Inputs are also wires
    }
    
    void add_output(const std::string& output) {
        outputs.insert(output);
        wires.insert(output); // Outputs are also wires
    }
};

// Main parser class
class VerilogParser {
private:
    std::map<std::string, Module> modules;
    std::string current_module;
    
    // Helper methods
    std::vector<std::string> tokenize(const std::string& line);
    std::string clean_line(const std::string& line);
    GateType string_to_gate_type(const std::string& type_str);
    std::vector<std::string> parse_port_list(const std::string& port_str);
    std::map<std::string, std::string> parse_connections(
        const std::string& conn_str);
    
public:
    VerilogParser() = default;
    
    // Main parsing interface
    bool parse_file(const std::string& filename);
    bool parse_string(const std::string& verilog_code);
    
    // Output methods
    void print_netlist_json() const;
    void print_netlist_dot() const;
    void print_module_summary() const;
    
    // Accessors
    const std::map<std::string, Module>& get_modules() const {
        return modules;
    }
    
    const Module* get_module(const std::string& name) const {
        auto it = modules.find(name);
        return it != modules.end() ? &it->second : nullptr;
    }
    
    // Error handling
    struct ParseError : public std::runtime_error {
        ParseError(const std::string& msg, int line_num)
            : std::runtime_error("Line " + std::to_string(line_num) + 
                               ": " + msg) {}
    };
};

} // namespace verilog_parser

#endif // VERILOG_PARSER_H