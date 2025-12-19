// verilog_parser.cpp
#include "verilog_parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <regex>

namespace verilog_parser {

// Helper function to trim whitespace
static inline std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        ++start;
    }
    
    auto end = s.end();
    do {
        --end;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
    
    return std::string(start, end + 1);
}

// Clean line: remove comments and extra whitespace
std::string VerilogParser::clean_line(const std::string& line) {
    std::string result = line;
    
    // Remove single-line comments
    size_t comment_pos = result.find("//");
    if (comment_pos != std::string::npos) {
        result = result.substr(0, comment_pos);
    }
    
    // Remove block comments (simple version)
    size_t block_start;
    while ((block_start = result.find("/*")) != std::string::npos) {
        size_t block_end = result.find("*/", block_start);
        if (block_end != std::string::npos) {
            result.erase(block_start, block_end - block_start + 2);
        } else {
            break; // Malformed comment
        }
    }
    
    // Replace tabs with spaces
    std::replace(result.begin(), result.end(), '\t', ' ');
    
    return trim(result);
}

// Tokenize a line into words/symbols
std::vector<std::string> VerilogParser::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token;
    bool in_quotes = false;
    bool in_brackets = 0;
    
    for (char c : line) {
        if (c == '"') {
            in_quotes = !in_quotes;
            token += c;
        } else if (c == '[') {
            in_brackets++;
            token += c;
        } else if (c == ']') {
            in_brackets--;
            token += c;
        } else if (!in_quotes && in_brackets == 0 && 
                  (c == ' ' || c == ',' || c == ';' || c == '(' || 
                   c == ')' || c == '#' || c == '.' || c == '=')) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            if (c != ' ' && c != ',') {
                tokens.push_back(std::string(1, c));
            }
        } else {
            token += c;
        }
    }
    
    if (!token.empty()) {
        tokens.push_back(token);
    }
    
    return tokens;
}

// Convert string to GateType enum
GateType VerilogParser::string_to_gate_type(const std::string& type_str) {
    static const std::map<std::string, GateType> gate_map = {
        {"and", GateType::AND},
        {"or", GateType::OR},
        {"xor", GateType::XOR},
        {"nand", GateType::NAND},
        {"nor", GateType::NOR},
        {"xnor", GateType::XNOR},
        {"not", GateType::NOT},
        {"buf", GateType::BUF},
        {"input", GateType::INPUT},
        {"output", GateType::OUTPUT},
        {"wire", GateType::WIRE}
    };
    
    std::string lower = type_str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    auto it = gate_map.find(lower);
    if (it != gate_map.end()) {
        return it->second;
    }
    
    // If not a standard gate, assume it's a module instance
    return GateType::MODULE;
}

// Parse port list like (a, b, c)
std::vector<std::string> VerilogParser::parse_port_list(
    const std::string& port_str) {
    std::vector<std::string> ports;
    std::string cleaned = port_str;
    
    // Remove parentheses if present
    if (cleaned.front() == '(' && cleaned.back() == ')') {
        cleaned = cleaned.substr(1, cleaned.length() - 2);
    }
    
    std::stringstream ss(cleaned);
    std::string port;
    
    while (std::getline(ss, port, ',')) {
        port = trim(port);
        if (!port.empty()) {
            ports.push_back(port);
        }
    }
    
    return ports;
}

// Parse connections like .A(net1), .B(net2)
std::map<std::string, std::string> VerilogParser::parse_connections(
    const std::string& conn_str) {
    std::map<std::string, std::string> connections;
    
    // Use regex for more robust parsing
    std::regex conn_regex("\\.(\\w+)\\s*\\(\\s*(\\w+)\\s*\\)");
    std::smatch matches;
    std::string search_str = conn_str;
    
    while (std::regex_search(search_str, matches, conn_regex)) {
        if (matches.size() == 3) {
            std::string port = matches[1].str();
            std::string net = matches[2].str();
            connections[port] = net;
        }
        search_str = matches.suffix();
    }
    
    return connections;
}

// Main parsing function for files
bool VerilogParser::parse_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse_string(buffer.str());
}

// Main parsing function for strings
bool VerilogParser::parse_string(const std::string& verilog_code) {
    std::stringstream ss(verilog_code);
    std::string line;
    int line_num = 0;
    
    Module* current_module_ptr = nullptr;
    bool in_module = false;
    
    try {
        while (std::getline(ss, line)) {
            line_num++;
            std::string cleaned = clean_line(line);
            if (cleaned.empty()) continue;
            
            std::vector<std::string> tokens = tokenize(cleaned);
            if (tokens.empty()) continue;
            
            // Check for module declaration
            if (tokens[0] == "module") {
                if (tokens.size() < 2) {
                    throw ParseError("Invalid module declaration", line_num);
                }
                
                current_module = tokens[1];
                modules[current_module] = Module(current_module);
                current_module_ptr = &modules[current_module];
                in_module = true;
                
                // Parse ports if available
                for (size_t i = 2; i < tokens.size(); i++) {
                    if (tokens[i] == "(") {
                        // Find closing paren
                        std::string port_list;
                        while (i < tokens.size() && tokens[i] != ")") {
                            if (tokens[i] != "(") {
                                port_list += tokens[i] + " ";
                            }
                            i++;
                        }
                        
                        if (!port_list.empty()) {
                            std::vector<std::string> ports = 
                                parse_port_list(port_list);
                            current_module_ptr->ports = ports;
                        }
                        break;
                    }
                }
            }
            // Check for endmodule
            else if (tokens[0] == "endmodule") {
                in_module = false;
                current_module_ptr = nullptr;
            }
            // Inside module parsing
            else if (in_module && current_module_ptr) {
                // Check for input/output/wire declarations
                if (tokens[0] == "input" || tokens[0] == "output" || 
                    tokens[0] == "wire") {
                    
                    for (size_t i = 1; i < tokens.size(); i++) {
                        if (tokens[i] != "," && tokens[i] != ";") {
                            if (tokens[0] == "input") {
                                current_module_ptr->add_input(tokens[i]);
                            } else if (tokens[0] == "output") {
                                current_module_ptr->add_output(tokens[i]);
                            } else if (tokens[0] == "wire") {
                                current_module_ptr->add_wire(tokens[i]);
                            }
                        }
                    }
                }
                // Check for gate instantiations
                else if (tokens.size() >= 3) {
                    std::string type_str = tokens[0];
                    std::string instance_name = tokens[1];
                    
                    // Skip if it's a parameter (#)
                    if (type_str == "#") continue;
                    
                    GateType type = string_to_gate_type(type_str);
                    Gate gate(instance_name, type);
                    
                    // Parse connections
                    std::string conn_str;
                    for (size_t i = 2; i < tokens.size(); i++) {
                        conn_str += tokens[i] + " ";
                    }
                    
                    if (type == GateType::MODULE) {
                        // For module instances, parse named connections
                        std::map<std::string, std::string> connections = 
                            parse_connections(conn_str);
                        gate.connections = connections;
                    } else {
                        // For primitive gates, parse ordered ports
                        std::vector<std::string> ports = 
                            parse_port_list(conn_str);
                        gate.ports = ports;
                        
                        // For basic gates, assign default port names
                        if (type == GateType::NOT || type == GateType::BUF) {
                            if (ports.size() >= 2) {
                                gate.connections["in"] = ports[0];
                                gate.connections["out"] = ports[1];
                            }
                        } else if (ports.size() >= 3) {
                            // Assume last port is output for basic gates
                            for (size_t i = 0; i < ports.size() - 1; i++) {
                                gate.connections["in" + std::to_string(i+1)] = 
                                    ports[i];
                            }
                            gate.connections["out"] = ports.back();
                        }
                    }
                    
                    current_module_ptr->add_gate(gate);
                }
            }
        }
        
        return true;
    } catch (const ParseError& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error at line " << line_num << ": " << e.what() << std::endl;
        return false;
    }
}

// Output netlist in JSON-like format
void VerilogParser::print_netlist_json() const {
    std::cout << "{\n";
    std::cout << "  \"modules\": [\n";
    
    bool first_module = true;
    for (const auto& module_pair : modules) {
        const Module& module = module_pair.second;
        
        if (!first_module) std::cout << ",\n";
        first_module = false;
        
        std::cout << "    {\n";
        std::cout << "      \"name\": \"" << module.name << "\",\n";
        
        // Print ports
        std::cout << "      \"ports\": [";
        for (size_t i = 0; i < module.ports.size(); i++) {
            if (i > 0) std::cout << ", ";
            std::cout << "\"" << module.ports[i] << "\"";
        }
        std::cout << "],\n";
        
        // Print inputs
        std::cout << "      \"inputs\": [";
        bool first = true;
        for (const auto& input : module.inputs) {
            if (!first) std::cout << ", ";
            std::cout << "\"" << input << "\"";
            first = false;
        }
        std::cout << "],\n";
        
        // Print outputs
        std::cout << "      \"outputs\": [";
        first = true;
        for (const auto& output : module.outputs) {
            if (!first) std::cout << ", ";
            std::cout << "\"" << output << "\"";
            first = false;
        }
        std::cout << "],\n";
        
        // Print wires
        std::cout << "      \"wires\": [";
        first = true;
        for (const auto& wire : module.wires) {
            // Skip inputs and outputs
            if (module.inputs.find(wire) == module.inputs.end() &&
                module.outputs.find(wire) == module.outputs.end()) {
                if (!first) std::cout << ", ";
                std::cout << "\"" << wire << "\"";
                first = false;
            }
        }
        std::cout << "],\n";
        
        // Print gates
        std::cout << "      \"gates\": [\n";
        bool first_gate = true;
        for (const auto& gate_pair : module.gates) {
            const Gate& gate = gate_pair.second;
            
            if (!first_gate) std::cout << ",\n";
            first_gate = false;
            
            std::cout << "        {\n";
            std::cout << "          \"name\": \"" << gate.name << "\",\n";
            
            // Convert gate type to string
            std::string type_str;
            switch (gate.type) {
                case GateType::AND: type_str = "AND"; break;
                case GateType::OR: type_str = "OR"; break;
                case GateType::XOR: type_str = "XOR"; break;
                case GateType::NAND: type_str = "NAND"; break;
                case GateType::NOR: type_str = "NOR"; break;
                case GateType::XNOR: type_str = "XNOR"; break;
                case GateType::NOT: type_str = "NOT"; break;
                case GateType::BUF: type_str = "BUF"; break;
                case GateType::MODULE: type_str = "MODULE_INST"; break;
                default: type_str = "UNKNOWN";
            }
            std::cout << "          \"type\": \"" << type_str << "\",\n";
            
            // Print connections
            std::cout << "          \"connections\": {\n";
            bool first_conn = true;
            for (const auto& conn : gate.connections) {
                if (!first_conn) std::cout << ",\n";
                first_conn = false;
                std::cout << "            \"" << conn.first << "\": \"" 
                         << conn.second << "\"";
            }
            std::cout << "\n          }\n";
            std::cout << "        }";
        }
        std::cout << "\n      ]\n";
        std::cout << "    }";
    }
    
    std::cout << "\n  ]\n";
    std::cout << "}\n";
}

// Output netlist in DOT format for Graphviz
void VerilogParser::print_netlist_dot() const {
    if (modules.empty()) {
        std::cout << "// No modules found\n";
        return;
    }
    
    // For simplicity, just show the first module
    const Module& module = modules.begin()->second;
    
    std::cout << "digraph " << module.name << " {\n";
    std::cout << "  rankdir=LR;\n";
    std::cout << "  node [shape=box];\n\n";
    
    // Create nodes for gates
    for (const auto& gate_pair : module.gates) {
        const Gate& gate = gate_pair.second;
        std::string shape;
        
        switch (gate.type) {
            case GateType::AND: shape = "and"; break;
            case GateType::OR: shape = "or"; break;
            case GateType::XOR: shape = "xor"; break;
            case GateType::NAND: shape = "nand"; break;
            case GateType::NOR: shape = "nor"; break;
            case GateType::NOT: shape = "invtriangle"; break;
            default: shape = "box";
        }
        
        std::cout << "  \"" << gate.name << "\" [shape=" << shape 
                 << ", label=\"" << gate.name << "\"];\n";
    }
    
    std::cout << "\n";
    
    // Create edges based on connections
    for (const auto& gate_pair : module.gates) {
        const Gate& gate = gate_pair.second;
        
        for (const auto& conn : gate.connections) {
            // Simple heuristic: if connection contains net that's also a gate output
            for (const auto& other_gate_pair : module.gates) {
                if (other_gate_pair.first == gate.name) continue;
                
                // Check if this gate drives the net
                const Gate& other_gate = other_gate_pair.second;
                for (const auto& other_conn : other_gate.connections) {
                    if (other_conn.second == conn.second && 
                        other_conn.first == "out") {
                        std::cout << "  \"" << other_gate.name << "\" -> \"" 
                                 << gate.name << "\" [label=\"" 
                                 << conn.second << "\"];\n";
                    }
                }
            }
        }
    }
    
    // Add I/O ports
    for (const auto& input : module.inputs) {
        std::cout << "  \"" << input << "\" [shape=circle];\n";
        // Connect inputs to gates that use them
        for (const auto& gate_pair : module.gates) {
            const Gate& gate = gate_pair.second;
            for (const auto& conn : gate.connections) {
                if (conn.second == input) {
                    std::cout << "  \"" << input << "\" -> \"" 
                             << gate.name << "\";\n";
                }
            }
        }
    }
    
    for (const auto& output : module.outputs) {
        std::cout << "  \"" << output << "\" [shape=doublecircle];\n";
        // Connect gates that drive outputs
        for (const auto& gate_pair : module.gates) {
            const Gate& gate = gate_pair.second;
            for (const auto& conn : gate.connections) {
                if (conn.second == output && conn.first == "out") {
                    std::cout << "  \"" << gate.name << "\" -> \"" 
                             << output << "\";\n";
                }
            }
        }
    }
    
    std::cout << "}\n";
}

// Print module summary
void VerilogParser::print_module_summary() const {
    if (modules.empty()) {
        std::cout << "No modules parsed.\n";
        return;
    }
    
    std::cout << "Parsed " << modules.size() << " module(s):\n\n";
    
    for (const auto& module_pair : modules) {
        const Module& module = module_pair.second;
        
        std::cout << "Module: " << module.name << "\n";
        std::cout << "  Ports: ";
        for (size_t i = 0; i < module.ports.size(); i++) {
            if (i > 0) std::cout << ", ";
            std::cout << module.ports[i];
        }
        std::cout << "\n";
        
        std::cout << "  Inputs (" << module.inputs.size() << "): ";
        for (const auto& input : module.inputs) {
            std::cout << input << " ";
        }
        std::cout << "\n";
        
        std::cout << "  Outputs (" << module.outputs.size() << "): ";
        for (const auto& output : module.outputs) {
            std::cout << output << " ";
        }
        std::cout << "\n";
        
        std::cout << "  Wires (" << module.wires.size() << "): ";
        for (const auto& wire : module.wires) {
            // Don't show I/O as wires
            if (module.inputs.find(wire) == module.inputs.end() &&
                module.outputs.find(wire) == module.outputs.end()) {
                std::cout << wire << " ";
            }
        }
        std::cout << "\n";
        
        std::cout << "  Gates (" << module.gates.size() << "):\n";
        for (const auto& gate_pair : module.gates) {
            const Gate& gate = gate_pair.second;
            std::cout << "    - " << gate.name << " (";
            
            switch (gate.type) {
                case GateType::AND: std::cout << "AND"; break;
                case GateType::OR: std::cout << "OR"; break;
                case GateType::XOR: std::cout << "XOR"; break;
                case GateType::NAND: std::cout << "NAND"; break;
                case GateType::NOR: std::cout << "NOR"; break;
                case GateType::XNOR: std::cout << "XNOR"; break;
                case GateType::NOT: std::cout << "NOT"; break;
                case GateType::BUF: std::cout << "BUF"; break;
                case GateType::MODULE: std::cout << "MODULE"; break;
                default: std::cout << "UNKNOWN";
            }
            
            std::cout << ") connections: ";
            for (const auto& conn : gate.connections) {
                std::cout << conn.first << "->" << conn.second << " ";
            }
            std::cout << "\n";
        }
        
        std::cout << "\n";
    }
}

} // namespace verilog_parser