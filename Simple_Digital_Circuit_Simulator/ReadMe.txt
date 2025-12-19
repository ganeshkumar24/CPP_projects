Description: Parses a simple netlist file (text format with gates like AND, OR, NOT) and simulates output for given inputs. Supports combinational circuits only (no clocks).
Complexity: Medium—graph representation + topological sort for evaluation. Easier by limiting to acyclic graphs.
C++ Features: Classes for Gate/Node, map<string, bool> for signals, queue for event-driven sim.
Outline:
Input: Text file like "INPUT A B; AND G1 A B C; OUTPUT C;".
Steps: Builds graph, evaluates gates in order, propagates values.
Output: Truth table or specific input simulation (e.g., A=1 B=0 -> C=0).