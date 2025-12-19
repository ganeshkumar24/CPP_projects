 Limitations:
Does not support behavioral constructs (always@, assign)
No support for parameters or generate blocks
Limited comment handling
Simple tokenization (not full Verilog lexer)


File names and what they do:
verilog_parser.h - Header with class definitions
verilog_parser.cpp - Implementation with parsing logic
main.cpp - Example usage and driver
Uses modern C++11 features (regex, smart pointers, etc.)

Design Notes:
Uses a simple state machine for parsing
Tokenization handles basic Verilog syntax
Gate connectivity is extracted to port->net mapping
Error reporting includes line numbers
Modular design for easy extension