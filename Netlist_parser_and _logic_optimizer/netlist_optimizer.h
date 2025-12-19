#pragma once
#include <concepts>
#include <variant>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <regex>
#include <filesystem>
#include <ranges>
#include <iostream>

enum class GateType { AND, OR, NOT, INPUT, OUTPUT };

struct Node {
    GateType type;
    std::string name;
    std::vector<std::shared_ptr<Node>> inputs;
    std::vector<std::weak_ptr<Node>> outputs;
    std::variant<bool, std::string> value; // Constant or variable
};

template <typename T>
concept NodePtr = std::same_as<T, std::shared_ptr<Node>>;

class NetlistOptimizer {
private:
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::vector<std::shared_ptr<Node>> primaries;

    void parse_line(const std::string& line);
    void propagate_constants();
    double compute_scoap(const std::shared_ptr<Node>& node); // Simplified controllability

public:
    void load_netlist(const std::filesystem::path& path);
    void optimize();
    void print_optimized() const;
};