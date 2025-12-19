Description: Implement a program that takes a Boolean function (as minterms or truth table) and outputs the minimized sum-of-products form. Handle up to 8 variables (doable limit to keep it simple).
Key C++ Features: Use bitsets for terms, vectors of sets for grouping, recursion or loops for essential primes.
Outline: https://en.wikipedia.org/wiki/Quine%E2%80%93McCluskey_algorithm
Input: Number of variables, list of minterms (e.g., "4 0 2 3 5 7").
Steps: Generate implicants, group by 1's count, merge adjacent, find essentials.
Output: Minimized expression (e.g., "A'B + AB' + C").
  Input: 3 1 2 4 7
  Expected: A'B'C + A'B C' + A B' C' + A B C

 Test 2: Simple 2-variable OR
 Input: 2 1 2 3
 Expected: A + B' (or equivalent, depending on vars)

 Test 3: 4-variable example from query
 Input: 4 0 2 3 5 7
 Expected: Something like A'B' + A'B C D + A'B C' D (run to check)

For function F(A,B,C) = Σ(0,2,4,6) + d(1,5):

Enter number of variables: 3
Enter minterms: 0 2 4 6 -1
Enter don't care terms: 1 5 -1
Result: C'

For Full Adder sum bit

Enter number of variables: 3
Enter minterms: 1 2 4 7 -1
Enter don't care terms: -1
Result: A'B'C + A'BC' + AB'C' + ABC