#include "netlist_optimizer.h"
#include <fstream>
#include <queue>
#include <algorithm>

void NetlistOptimizer::parse_line(const std::string& line) {
    static std::regex gate_regex(R"((\w+)\s+(\w+)\s+([\w\s]+);)");
    std::smatch matches;
    if (std::regex_match(line, matches, gate_regex)) {
        std::string type_str = matches[1];
        std::string name = matches[2];
        std::string inputs_str = matches[3];

        GateType type;
        if (type_str == "AND") type = GateType::AND;
        else if (type_str == "OR") type = GateType::OR;
        else if (type_str == "NOT") type = GateType::NOT;
        else if (type_str == "INPUT") type = GateType::INPUT;
        else if (type_str == "OUTPUT") type = GateType::OUTPUT;
        else return;

        auto node = std::make_shared<Node>();
        node->type = type;
        node->name = name;
        nodes[name] = node;

        if (type == GateType::INPUT || type == GateType::OUTPUT) primaries.push_back(node);

        std::istringstream iss(inputs_str);
        std::string input_name;
        while (iss >> input_name) {
            if (nodes.contains(input_name)) {
                node->inputs.push_back(nodes[input_name]);
                nodes[input_name]->outputs.push_back(node);
            }
        }
    }
}

void NetlistOptimizer::load_netlist(const std::filesystem::path& path) {
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) parse_line(line);
}

void NetlistOptimizer::propagate_constants() {
    std::queue<std::shared_ptr<Node>> q;
    for (auto& p : primaries | std::views::filter([](const auto& n) { return n->type == GateType::INPUT; })) {
        if (std::holds_alternative<bool>(p->value)) q.push(p);
    }

    auto eval_gate = [](const std::shared_ptr<Node>& n) -> std::optional<bool> {
        if (n->type == GateType::NOT) {
            if (auto val = std::get_if<bool>(&n->inputs[0]->value)) return !*val;
        } else if (n->type == GateType::AND) {
            bool res = true;
            for (const auto& in : n->inputs) if (auto val = std::get_if<bool>(&in->value)) res &= *val; else return {};
            return res;
        } else if (n->type == GateType::OR) {
            bool res = false;
            for (const auto& in : n->inputs) if (auto val = std::get_if<bool>(&in->value)) res |= *val; else return {};
            return res;
        }
        return {};
    };

    while (!q.empty()) {
        auto node = q.front(); q.pop();
        for (auto out_weak : node->outputs) {
            if (auto out = out_weak.lock()) {
                if (auto new_val = eval_gate(out)) {
                    out->value = *new_val;
                    q.push(out);
                }
            }
        }
    }
}

double NetlistOptimizer::compute_scoap(const std::shared_ptr<Node>& node) {
    if (node->type == GateType::INPUT) return 1.0;
    double cc = 0.0;
    for (const auto& in : node->inputs) cc += compute_scoap(in);
    return cc / node->inputs.size(); // Simplified average
}

void NetlistOptimizer::optimize() {
    propagate_constants();
    // Remove redundant (constant) gates - example: erase if constant output
    for (auto it = nodes.begin(); it != nodes.end(); ) {
        auto node = it->second;
        if (std::holds_alternative<bool>(node->value) && node->type != GateType::OUTPUT) {
            bool val = std::get<bool>(node->value);
            for (auto out_weak : node->outputs) {
                if (auto out = out_weak.lock()) {
                    auto& ins = out->inputs;
                    std::erase_if(ins, [&](const auto& in) { return in == node; });
                    ins.push_back(std::make_shared<Node>(Node{GateType::INPUT, "const_" + std::to_string(val), {}, {}, val}));
                }
            }
            it = nodes.erase(it);
        } else ++it;
    }
}

void NetlistOptimizer::print_optimized() const {
    for (const auto& [name, node] : nodes) {
        std::cout << name << " SCOAP: " << compute_scoap(node) << "\n";
    }
}