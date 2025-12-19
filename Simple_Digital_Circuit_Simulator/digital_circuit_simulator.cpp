/**
 * Simple Digital Circuit Simulator (Gate-Level Netlist)
 * 
 * A C++ implementation for simulating combinational logic circuits
 * from gate-level netlists. Supports basic gates: AND, OR, NOT, NAND, NOR, XOR
 * 
 * Author: [Your Name]
 * Date: [Current Date]
 * 
 * This simulator reads a netlist file, builds a circuit graph,
 * performs topological sorting for evaluation order, and simulates
 * output values for given input patterns.
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <memory>
#include <algorithm>
#include <cassert>

// ============================================================================
//                             GATE TYPE ENUMERATION
// ============================================================================

/**
 * Enumeration of supported gate types
 */
enum class GateType {
    INPUT,      // Primary input
    OUTPUT,     // Primary output
    AND,        // AND gate
    OR,         // OR gate
    NOT,        // NOT gate (1 input)
    NAND,       // NAND gate
    NOR,        // NOR gate
    XOR,        // XOR gate
    BUFFER      // Buffer (pass-through)
};

/**
 * Convert GateType to string for display
 */
std::string gateTypeToString(GateType type) {
    switch (type) {
        case GateType::INPUT:   return "INPUT";
        case GateType::OUTPUT:  return "OUTPUT";
        case GateType::AND:     return "AND";
        case GateType::OR:      return "OR";
        case GateType::NOT:     return "NOT";
        case GateType::NAND:    return "NAND";
        case GateType::NOR:     return "NOR";
        case GateType::XOR:     return "XOR";
        case GateType::BUFFER:  return "BUFFER";
        default:                return "UNKNOWN";
    }
}

// ============================================================================
//                             NODE CLASS
// ============================================================================

/**
 * Class representing a node (wire/signal) in the circuit
 */
class Node {
public:
    std::string name;
    bool value;                 // Current logical value (true=1, false=0)
    bool visited;               // For graph traversal algorithms
    int level;                  // Level in the circuit (for topological sort)
    
    // Constructor
    Node(const std::string& nodeName) 
        : name(nodeName), value(false), visited(false), level(-1) {}
    
    // Get value as character ('0' or '1')
    char getValueChar() const {
        return value ? '1' : '0';
    }
    
    // Set value from bool
    void setValue(bool val) {
        value = val;
    }
    
    // Set value from char ('0' or '1')
    void setValueFromChar(char c) {
        value = (c == '1');
    }
};

// ============================================================================
//                             GATE CLASS
// ============================================================================

/**
 * Class representing a gate in the circuit
 */
class Gate {
public:
    std::string name;
    GateType type;
    std::vector<std::shared_ptr<Node>> inputs;  // Input nodes
    std::shared_ptr<Node> output;               // Output node
    
    // Constructor
    Gate(const std::string& gateName, GateType gateType) 
        : name(gateName), type(gateType) {}
    
    /**
     * Evaluate the gate output based on input values
     * Returns true if output changed
     */
    bool evaluate() {
        bool oldValue = output->value;
        bool newValue = false;
        
        switch (type) {
            case GateType::AND:
                newValue = true;
                for (const auto& input : inputs) {
                    newValue = newValue && input->value;
                    if (!newValue) break;
                }
                break;
                
            case GateType::OR:
                newValue = false;
                for (const auto& input : inputs) {
                    newValue = newValue || input->value;
                    if (newValue) break;
                }
                break;
                
            case GateType::NOT:
                if (inputs.size() >= 1) {
                    newValue = !inputs[0]->value;
                }
                break;
                
            case GateType::NAND:
                newValue = true;
                for (const auto& input : inputs) {
                    newValue = newValue && input->value;
                }
                newValue = !newValue;
                break;
                
            case GateType::NOR:
                newValue = false;
                for (const auto& input : inputs) {
                    newValue = newValue || input->value;
                }
                newValue = !newValue;
                break;
                
            case GateType::XOR:
                if (inputs.size() >= 2) {
                    newValue = inputs[0]->value != inputs[1]->value;
                    for (size_t i = 2; i < inputs.size(); i++) {
                        newValue = newValue != inputs[i]->value;
                    }
                }
                break;
                
            case GateType::BUFFER:
                if (inputs.size() >= 1) {
                    newValue = inputs[0]->value;
                }
                break;
                
            default:
                // For INPUT and OUTPUT gates, do nothing
                return false;
        }
        
        output->setValue(newValue);
        return (oldValue != newValue);
    }
    
    /**
     * Get string representation of the gate
     */
    std::string toString() const {
        std::string result = gateTypeToString(type) + " " + name + " (";
        
        // List inputs
        for (size_t i = 0; i < inputs.size(); i++) {
            if (i > 0) result += ", ";
            result += inputs[i]->name;
        }
        
        result += ") -> " + output->name;
        return result;
    }
};

// ============================================================================
//                             CIRCUIT CLASS
// ============================================================================

/**
 * Main circuit class representing the entire netlist
 */
class Circuit {
private:
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::unordered_map<std::string, std::shared_ptr<Gate>> gates;
    std::vector<std::shared_ptr<Node>> primaryInputs;
    std::vector<std::shared_ptr<Node>> primaryOutputs;
    std::vector<std::shared_ptr<Gate>> gateList;  // Gates in evaluation order
    
    /**
     * Get or create a node with the given name
     */
    std::shared_ptr<Node> getOrCreateNode(const std::string& name) {
        if (nodes.find(name) == nodes.end()) {
            nodes[name] = std::make_shared<Node>(name);
        }
        return nodes[name];
    }
    
    /**
     * Perform topological sort to determine evaluation order
     * Uses Kahn's algorithm
     */
    bool topologicalSort() {
        // Clear previous ordering
        gateList.clear();
        
        // Calculate in-degree for each gate
        std::unordered_map<std::shared_ptr<Gate>, int> inDegree;
        std::unordered_map<std::shared_ptr<Node>, std::vector<std::shared_ptr<Gate>>> nodeToFanoutGates;
        
        // Initialize node to fanout gates mapping
        for (const auto& gatePair : gates) {
            const auto& gate = gatePair.second;
            for (const auto& inputNode : gate->inputs) {
                nodeToFanoutGates[inputNode].push_back(gate);
            }
        }
        
        // Calculate initial in-degree for each gate
        for (const auto& gatePair : gates) {
            const auto& gate = gatePair.second;
            inDegree[gate] = 0;
        }
        
        for (const auto& gatePair : gates) {
            const auto& gate = gatePair.second;
            for (const auto& inputNode : gate->inputs) {
                // If the input node is driven by another gate, increase in-degree
                for (const auto& drivingGate : gates) {
                    if (drivingGate.second->output == inputNode) {
                        inDegree[gate]++;
                        break;
                    }
                }
            }
        }
        
        // Queue for nodes with zero in-degree
        std::queue<std::shared_ptr<Gate>> zeroInDegreeQueue;
        
        for (const auto& entry : inDegree) {
            if (entry.second == 0) {
                zeroInDegreeQueue.push(entry.first);
            }
        }
        
        // Process the queue
        int processedCount = 0;
        while (!zeroInDegreeQueue.empty()) {
            auto currentGate = zeroInDegreeQueue.front();
            zeroInDegreeQueue.pop();
            
            gateList.push_back(currentGate);
            processedCount++;
            
            // Reduce in-degree of neighbors
            auto outputNode = currentGate->output;
            if (nodeToFanoutGates.find(outputNode) != nodeToFanoutGates.end()) {
                for (const auto& fanoutGate : nodeToFanoutGates[outputNode]) {
                    inDegree[fanoutGate]--;
                    if (inDegree[fanoutGate] == 0) {
                        zeroInDegreeQueue.push(fanoutGate);
                    }
                }
            }
        }
        
        // Check for cycles (if not all gates were processed)
        if (processedCount != gates.size()) {
            std::cerr << "Error: Circuit contains cycles (not combinational)!" << std::endl;
            return false;
        }
        
        return true;
    }
    
    /**
     * Assign levels to gates based on topological order
     */
    void assignGateLevels() {
        // Reset all node levels
        for (auto& nodePair : nodes) {
            nodePair.second->level = -1;
        }
        
        // Input nodes are at level 0
        for (const auto& input : primaryInputs) {
            input->level = 0;
        }
        
        // Assign levels based on gate evaluation order
        for (const auto& gate : gateList) {
            int maxInputLevel = -1;
            for (const auto& input : gate->inputs) {
                if (input->level > maxInputLevel) {
                    maxInputLevel = input->level;
                }
            }
            
            int gateLevel = maxInputLevel + 1;
            gate->output->level = gateLevel;
        }
    }

public:
    Circuit() = default;
    
    /**
     * Parse a netlist from a file
     */
    bool parseNetlist(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return false;
        }
        
        std::string line;
        int lineNumber = 0;
        
        while (std::getline(file, line)) {
            lineNumber++;
            
            // Remove comments (anything after //)
            size_t commentPos = line.find("//");
            if (commentPos != std::string::npos) {
                line = line.substr(0, commentPos);
            }
            
            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            
            // Skip empty lines
            if (line.empty()) {
                continue;
            }
            
            // Parse the line
            std::istringstream iss(line);
            std::string token;
            iss >> token;
            
            if (token == "INPUT" || token == "input") {
                // Parse input declaration: INPUT A B C;
                std::string inputName;
                while (iss >> inputName) {
                    // Remove trailing semicolon if present
                    if (inputName.back() == ';') {
                        inputName.pop_back();
                    }
                    
                    auto node = getOrCreateNode(inputName);
                    primaryInputs.push_back(node);
                    
                    // Create an INPUT gate for this primary input
                    auto inputGate = std::make_shared<Gate>("INPUT_" + inputName, GateType::INPUT);
                    inputGate->output = node;
                    gates[inputGate->name] = inputGate;
                }
            }
            else if (token == "OUTPUT" || token == "output") {
                // Parse output declaration: OUTPUT X Y Z;
                std::string outputName;
                while (iss >> outputName) {
                    // Remove trailing semicolon if present
                    if (outputName.back() == ';') {
                        outputName.pop_back();
                    }
                    
                    auto node = getOrCreateNode(outputName);
                    primaryOutputs.push_back(node);
                    
                    // Create an OUTPUT gate for this primary output
                    auto outputGate = std::make_shared<Gate>("OUTPUT_" + outputName, GateType::OUTPUT);
                    outputGate->inputs.push_back(node);
                    outputGate->output = node;  // Self-loop for consistency
                    gates[outputGate->name] = outputGate;
                }
            }
            else {
                // Parse gate definition: AND G1 A B C;
                GateType type;
                if (token == "AND" || token == "and") type = GateType::AND;
                else if (token == "OR" || token == "or") type = GateType::OR;
                else if (token == "NOT" || token == "not") type = GateType::NOT;
                else if (token == "NAND" || token == "nand") type = GateType::NAND;
                else if (token == "NOR" || token == "nor") type = GateType::NOR;
                else if (token == "XOR" || token == "xor") type = GateType::XOR;
                else if (token == "BUFFER" || token == "buffer") type = GateType::BUFFER;
                else {
                    std::cerr << "Error at line " << lineNumber << ": Unknown gate type '" << token << "'" << std::endl;
                    return false;
                }
                
                // Read gate name
                std::string gateName;
                if (!(iss >> gateName)) {
                    std::cerr << "Error at line " << lineNumber << ": Expected gate name" << std::endl;
                    return false;
                }
                
                // Create the gate
                auto gate = std::make_shared<Gate>(gateName, type);
                
                // Read input and output nodes
                std::string nodeName;
                std::vector<std::string> nodeNames;
                
                while (iss >> nodeName) {
                    // Remove trailing semicolon if present
                    if (nodeName.back() == ';') {
                        nodeName.pop_back();
                    }
                    nodeNames.push_back(nodeName);
                }
                
                // Validate node count based on gate type
                if (nodeNames.size() < 2) {
                    std::cerr << "Error at line " << lineNumber << ": Too few nodes for gate " << gateName << std::endl;
                    return false;
                }
                
                // Last node is the output
                auto outputNode = getOrCreateNode(nodeNames.back());
                gate->output = outputNode;
                
                // All other nodes are inputs
                for (size_t i = 0; i < nodeNames.size() - 1; i++) {
                    auto inputNode = getOrCreateNode(nodeNames[i]);
                    gate->inputs.push_back(inputNode);
                }
                
                // Special case: NOT gate should have exactly 1 input and 1 output
                if (type == GateType::NOT && gate->inputs.size() != 1) {
                    std::cerr << "Error at line " << lineNumber << ": NOT gate " << gateName 
                              << " must have exactly 1 input" << std::endl;
                    return false;
                }
                
                // Store the gate
                gates[gateName] = gate;
            }
        }
        
        file.close();
        
        // Perform topological sort to get evaluation order
        if (!topologicalSort()) {
            return false;
        }
        
        // Assign gate levels for visualization
        assignGateLevels();
        
        return true;
    }
    
    /**
     * Set input values from a map of input name to value
     */
    void setInputs(const std::map<std::string, bool>& inputValues) {
        // Reset all nodes to false first
        for (auto& nodePair : nodes) {
            nodePair.second->setValue(false);
        }
        
        // Set primary input values
        for (const auto& input : primaryInputs) {
            auto it = inputValues.find(input->name);
            if (it != inputValues.end()) {
                input->setValue(it->second);
            }
        }
    }
    
    /**
     * Set input values from a string like "A=1 B=0"
     */
    bool setInputsFromString(const std::string& inputString) {
        std::istringstream iss(inputString);
        std::string assignment;
        std::map<std::string, bool> inputValues;
        
        while (iss >> assignment) {
            size_t equalsPos = assignment.find('=');
            if (equalsPos == std::string::npos) {
                std::cerr << "Error: Invalid input assignment '" << assignment 
                          << "'. Expected format: name=value" << std::endl;
                return false;
            }
            
            std::string inputName = assignment.substr(0, equalsPos);
            std::string valueStr = assignment.substr(equalsPos + 1);
            
            bool value;
            if (valueStr == "1" || valueStr == "true" || valueStr == "TRUE") {
                value = true;
            } else if (valueStr == "0" || valueStr == "false" || valueStr == "FALSE") {
                value = false;
            } else {
                std::cerr << "Error: Invalid value '" << valueStr 
                          << "' for input '" << inputName 
                          << "'. Expected 0 or 1." << std::endl;
                return false;
            }
            
            inputValues[inputName] = value;
        }
        
        setInputs(inputValues);
        return true;
    }
    
    /**
     * Evaluate the circuit (combinational logic only)
     */
    void evaluate() {
        // Evaluate gates in topological order
        for (const auto& gate : gateList) {
            gate->evaluate();
        }
    }
    
    /**
     * Print the current state of the circuit
     */
    void printState() const {
        std::cout << "=== Circuit State ===" << std::endl;
        
        std::cout << "\nPrimary Inputs:" << std::endl;
        for (const auto& input : primaryInputs) {
            std::cout << "  " << input->name << " = " << input->getValueChar() << std::endl;
        }
        
        std::cout << "\nPrimary Outputs:" << std::endl;
        for (const auto& output : primaryOutputs) {
            std::cout << "  " << output->name << " = " << output->getValueChar() << std::endl;
        }
        
        std::cout << "\nAll Nodes:" << std::endl;
        for (const auto& nodePair : nodes) {
            std::cout << "  " << nodePair.first << " = " << nodePair.second->getValueChar() 
                      << " (Level: " << nodePair.second->level << ")" << std::endl;
        }
    }
    
    /**
     * Print the circuit structure
     */
    void printCircuit() const {
        std::cout << "=== Circuit Structure ===" << std::endl;
        
        std::cout << "\nPrimary Inputs (" << primaryInputs.size() << "):" << std::endl;
        for (const auto& input : primaryInputs) {
            std::cout << "  " << input->name << std::endl;
        }
        
        std::cout << "\nPrimary Outputs (" << primaryOutputs.size() << "):" << std::endl;
        for (const auto& output : primaryOutputs) {
            std::cout << "  " << output->name << std::endl;
        }
        
        std::cout << "\nGates (" << gates.size() << "):" << std::endl;
        for (const auto& gatePair : gates) {
            std::cout << "  " << gatePair.second->toString() << std::endl;
        }
        
        std::cout << "\nEvaluation Order:" << std::endl;
        for (size_t i = 0; i < gateList.size(); i++) {
            std::cout << "  " << i+1 << ". " << gateList[i]->toString() << std::endl;
        }
    }
    
    /**
     * Generate truth table for the circuit
     */
    void generateTruthTable() const {
        int numInputs = primaryInputs.size();
        if (numInputs == 0) {
            std::cout << "No primary inputs defined." << std::endl;
            return;
        }
        
        if (numInputs > 10) {
            std::cout << "Warning: " << numInputs << " inputs would generate " 
                      << (1 << numInputs) << " rows. This may take a while." << std::endl;
            std::cout << "Press Enter to continue or Ctrl+C to cancel..." << std::endl;
            std::cin.ignore();
        }
        
        std::cout << "\n=== Truth Table ===" << std::endl;
        
        // Print header
        for (const auto& input : primaryInputs) {
            std::cout << input->name << " ";
        }
        std::cout << "| ";
        for (const auto& output : primaryOutputs) {
            std::cout << output->name << " ";
        }
        std::cout << std::endl;
        
        // Print separator
        for (int i = 0; i < numInputs; i++) {
            std::cout << "--";
        }
        std::cout << "-+-";
        for (size_t i = 0; i < primaryOutputs.size(); i++) {
            std::cout << "--";
        }
        std::cout << std::endl;
        
        // Create a mutable copy of the circuit for simulation
        Circuit* mutableThis = const_cast<Circuit*>(this);
        
        // Generate all input combinations
        for (int i = 0; i < (1 << numInputs); i++) {
            // Set input values
            std::map<std::string, bool> inputValues;
            for (int j = 0; j < numInputs; j++) {
                bool value = (i >> (numInputs - 1 - j)) & 1;
                inputValues[primaryInputs[j]->name] = value;
                std::cout << value << " ";
            }
            
            mutableThis->setInputs(inputValues);
            mutableThis->evaluate();
            
            std::cout << "| ";
            for (const auto& output : primaryOutputs) {
                std::cout << output->getValueChar() << " ";
            }
            std::cout << std::endl;
        }
    }
    
    /**
     * Get output values as a map
     */
    std::map<std::string, bool> getOutputValues() const {
        std::map<std::string, bool> outputValues;
        for (const auto& output : primaryOutputs) {
            outputValues[output->name] = output->value;
        }
        return outputValues;
    }
    
    /**
     * Get a string representation of output values
     */
    std::string getOutputString() const {
        std::string result;
        for (const auto& output : primaryOutputs) {
            if (!result.empty()) result += " ";
            result += output->name + "=" + output->getValueChar();
        }
        return result;
    }
};

// ============================================================================
//                             HELPER FUNCTIONS
// ============================================================================

/**
 * Create sample netlist files for testing
 */
void createSampleNetlists() {
    // Sample 1: AND-OR circuit
    std::ofstream file1("sample_and_or.net");
    file1 << "// Sample AND-OR circuit\n";
    file1 << "INPUT A B C;\n";
    file1 << "AND G1 A B X;\n";
    file1 << "OR G2 X C Y;\n";
    file1 << "OUTPUT Y;\n";
    file1.close();
    std::cout << "Created sample_and_or.net\n";
    
    // Sample 2: XOR gate (using AND, OR, NOT)
    std::ofstream file2("sample_xor.net");
    file2 << "// XOR gate implementation\n";
    file2 << "INPUT A B;\n";
    file2 << "NOT N1 A NA;\n";
    file2 << "NOT N2 B NB;\n";
    file2 << "AND A1 A NB X1;\n";
    file2 << "AND A2 NA B X2;\n";
    file2 << "OR O1 X1 X2 XOR_OUT;\n";
    file2 << "OUTPUT XOR_OUT;\n";
    file2.close();
    std::cout << "Created sample_xor.net\n";
    
    // Sample 3: Half Adder
    std::ofstream file3("half_adder.net");
    file3 << "// Half Adder Circuit\n";
    file3 << "// Sum = A XOR B, Carry = A AND B\n";
    file3 << "INPUT A B;\n";
    file3 << "XOR X1 A B SUM;\n";
    file3 << "AND A1 A B CARRY;\n";
    file3 << "OUTPUT SUM CARRY;\n";
    file3.close();
    std::cout << "Created half_adder.net\n";
    
    // Sample 4: Full Adder
    std::ofstream file4("full_adder.net");
    file4 << "// Full Adder Circuit\n";
    file4 << "INPUT A B Cin;\n";
    file4 << "XOR X1 A B S1;\n";
    file4 << "XOR X2 S1 Cin SUM;\n";
    file4 << "AND A1 A B C1;\n";
    file4 << "AND A2 S1 Cin C2;\n";
    file4 << "OR O1 C1 C2 CARRY;\n";
    file4 << "OUTPUT SUM CARRY;\n";
    file4.close();
    std::cout << "Created full_adder.net\n";
}

/**
 * Print usage information
 */
void printUsage() {
    std::cout << "Digital Circuit Simulator - Usage:" << std::endl;
    std::cout << "  ./circuit_sim <netlist_file> [input_values]" << std::endl;
    std::cout << "  ./circuit_sim --samples        Create sample netlists" << std::endl;
    std::cout << "  ./circuit_sim --help           Show this help" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  ./circuit_sim half_adder.net A=1 B=0" << std::endl;
    std::cout << "  ./circuit_sim full_adder.net A=1 B=1 Cin=0" << std::endl;
    std::cout << "  ./circuit_sim circuit.net --truth-table" << std::endl;
}

// ============================================================================
//                             MAIN FUNCTION
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << "===============================================" << std::endl;
    std::cout << "   Digital Circuit Simulator (Gate-Level)     " << std::endl;
    std::cout << "===============================================" << std::endl;
    
    // Handle command line arguments
    if (argc < 2) {
        printUsage();
        return 1;
    }
    
    std::string netlistFile = argv[1];
    
    // Special commands
    if (netlistFile == "--samples" || netlistFile == "-s") {
        createSampleNetlists();
        return 0;
    }
    
    if (netlistFile == "--help" || netlistFile == "-h") {
        printUsage();
        return 0;
    }
    
    // Parse the netlist file
    Circuit circuit;
    if (!circuit.parseNetlist(netlistFile)) {
        std::cerr << "Failed to parse netlist file: " << netlistFile << std::endl;
        return 1;
    }
    
    std::cout << "Successfully parsed netlist: " << netlistFile << std::endl;
    
    // Check for truth table generation
    bool generateTruthTable = false;
    std::string inputString;
    
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--truth-table" || arg == "-t") {
            generateTruthTable = true;
        } else if (arg.find('=') != std::string::npos) {
            if (!inputString.empty()) inputString += " ";
            inputString += arg;
        }
    }
    
    // Print circuit structure
    circuit.printCircuit();
    
    if (generateTruthTable) {
        // Generate full truth table
        circuit.generateTruthTable();
    } else if (!inputString.empty()) {
        // Simulate with specific input values
        if (circuit.setInputsFromString(inputString)) {
            circuit.evaluate();
            std::cout << "\n=== Simulation Result ===" << std::endl;
            std::cout << "Inputs: " << inputString << std::endl;
            std::cout << "Outputs: " << circuit.getOutputString() << std::endl;
            
            // Print detailed state
            circuit.printState();
        }
    } else {
        // Interactive mode
        std::cout << "\n=== Interactive Mode ===" << std::endl;
        std::cout << "Enter input values (e.g., 'A=1 B=0') or 'truth' for truth table or 'quit' to exit:" << std::endl;
        
        std::string command;
        while (true) {
            std::cout << "\n> ";
            std::getline(std::cin, command);
            
            if (command == "quit" || command == "exit") {
                break;
            } else if (command == "truth" || command == "table") {
                circuit.generateTruthTable();
            } else if (command == "structure") {
                circuit.printCircuit();
            } else if (command == "state") {
                circuit.printState();
            } else if (!command.empty()) {
                if (circuit.setInputsFromString(command)) {
                    circuit.evaluate();
                    std::cout << "Outputs: " << circuit.getOutputString() << std::endl;
                }
            }
        }
    }
    
    return 0;
}