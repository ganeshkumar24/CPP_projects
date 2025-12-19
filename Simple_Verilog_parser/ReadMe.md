# Simple Verilog Parser for Netlist Extraction

A C++ parser for extracting netlist information from Verilog files (subset).

## Features

- Parses basic Verilog modules with gates (AND, OR, XOR, NAND, NOR, NOT, BUF)
- Extracts input/output ports, wires, and gate instances
- Handles module instantiations with named port connections
- Outputs netlist in JSON-like format and DOT graph format
- Error handling with line number reporting
- Clean, modular C++ implementation

## Supported Verilog Constructs

```verilog
// Module declaration
module module_name (port1, port2, ...);

// Port declarations
input a, b, c;
output x, y, z;
wire w1, w2;

// Gate instantiations
and g1(out, in1, in2);
or g2(out, in1, in2);
xor g3(out, in1, in2);
not g4(out, in);
buf g5(out, in);

// Module instantiations (with named ports)
submodule u1(.A(a), .B(b), .C(c));