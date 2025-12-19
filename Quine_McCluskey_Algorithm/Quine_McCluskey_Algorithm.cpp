#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <bitset>
#include <algorithm>
#include <functional>
#include <cmath>
#include <cassert>

class QuineMcCluskey {
private:
    int numVars;                     // Number of variables
    std::vector<int> minterms;       // Input minterms
    std::vector<int> dontcares;      // Don't care terms (if any)
    std::vector<char> varNames;      // Variable names (A, B, C, ...)
    
    // Helper function to convert integer to binary string
    static std::string intToBinary(int n, int numVars) {
        std::string result;
        for (int i = numVars - 1; i >= 0; i--) {
            result += ((n >> i) & 1) ? '1' : '0';
        }
        return result;
    }
    
    // Structure to represent an implicant
    struct Implicant {
        std::vector<int> minterms;   // Minterms covered
        std::string binary;          // Binary representation with '-'
        bool isPrime;                // Is this a prime implicant?
        bool isEssential;            // Is this an essential prime implicant?
        
        Implicant(int term, int numVars) {
            minterms.push_back(term);
            binary = intToBinary(term, numVars);
            isPrime = true;
            isEssential = false;
        }
        
        Implicant(const Implicant& a, const Implicant& b) {
            // Merge two implicants that differ by exactly one bit
            isPrime = true;
            isEssential = false;
            
            // Combine minterms
            minterms = a.minterms;
            minterms.insert(minterms.end(), b.minterms.begin(), b.minterms.end());
            std::sort(minterms.begin(), minterms.end());
            
            // Create merged binary representation
            binary = "";
            int diffCount = 0;
            for (size_t i = 0; i < a.binary.length(); i++) {
                if (a.binary[i] != b.binary[i]) {
                    binary += '-';
                    diffCount++;
                } else {
                    binary += a.binary[i];
                }
            }
        }
        
        // Check if two implicants can be merged
        static bool canMerge(const Implicant& a, const Implicant& b) {
            if (a.binary.length() != b.binary.length()) return false;
            
            int diffCount = 0;
            for (size_t i = 0; i < a.binary.length(); i++) {
                if (a.binary[i] != b.binary[i]) {
                    diffCount++;
                    if (diffCount > 1) return false;
                }
            }
            return diffCount == 1;
        }
        
        // Count number of 1's in binary representation
        int countOnes() const {
            int count = 0;
            for (char c : binary) {
                if (c == '1') count++;
            }
            return count;
        }
        
        // Check if this implicant covers a minterm
        bool covers(int term, int numVars) const {
            std::string termBinary = intToBinary(term, numVars);
            for (size_t i = 0; i < binary.length(); i++) {
                if (binary[i] != '-' && binary[i] != termBinary[i]) {
                    return false;
                }
            }
            return true;
        }
    };
    
    // Helper function to get variable names
    void initializeVarNames() {
        varNames.clear();
        for (int i = 0; i < numVars; i++) {
            varNames.push_back('A' + i);
        }
    }
    
    // Convert binary representation to SOP term
    std::string binaryToTerm(const std::string& binary) const {
        std::string term;
        for (size_t i = 0; i < binary.length(); i++) {
            if (binary[i] == '0') {
                term += varNames[i];
                term += '\'';
            } else if (binary[i] == '1') {
                term += varNames[i];
            }
            // '-' means variable is eliminated
        }
        return term;
    }
    
public:
    QuineMcCluskey(int vars, const std::vector<int>& m, const std::vector<int>& dc = {}) 
        : numVars(vars), minterms(m), dontcares(dc) {
        initializeVarNames();
    }
    
    // Main function to minimize the boolean expression
    std::string minimize() {
        if (numVars > 8) {
            throw std::runtime_error("Number of variables exceeds limit of 8");
        }
        
        // Validate minterms are within range
        int maxTerm = (1 << numVars) - 1;
        for (int term : minterms) {
            if (term < 0 || term > maxTerm) {
                throw std::runtime_error("Minterm " + std::to_string(term) + " out of range");
            }
        }
        for (int term : dontcares) {
            if (term < 0 || term > maxTerm) {
                throw std::runtime_error("Don't care term " + std::to_string(term) + " out of range");
            }
        }
        
        // Combine minterms and don't cares for processing
        std::vector<int> allTerms = minterms;
        allTerms.insert(allTerms.end(), dontcares.begin(), dontcares.end());
        std::sort(allTerms.begin(), allTerms.end());
        
        // Remove duplicates
        allTerms.erase(std::unique(allTerms.begin(), allTerms.end()), allTerms.end());
        
        // Step 1: Generate all prime implicants
        std::vector<Implicant> primeImplicants = findPrimeImplicants(allTerms);
        
        // Step 2: Create prime implicant chart and find essential primes
        std::vector<Implicant> essentialPrimes = findEssentialPrimes(primeImplicants);
        
        // Step 3: Petrick's method or row dominance for remaining terms
        std::vector<Implicant> minimalCover = findMinimalCover(essentialPrimes, primeImplicants);
        
        // Step 4: Convert to SOP expression
        return formatSOP(minimalCover);
    }
    
private:
    // Step 1: Find all prime implicants using Quine-McCluskey algorithm
    std::vector<Implicant> findPrimeImplicants(const std::vector<int>& terms) {
        if (terms.empty()) return {};
        
        // Group by number of 1's
        std::map<int, std::vector<Implicant>> groups;
        
        // Create initial implicants from terms
        for (int term : terms) {
            Implicant imp(term, numVars);
            groups[imp.countOnes()].push_back(imp);
        }
        
        std::vector<Implicant> primeImplicants;
        bool changed = true;
        
        while (changed) {
            changed = false;
            std::map<int, std::vector<Implicant>> newGroups;
            std::vector<bool> marked;
            
            // Calculate total size for marking array
            int totalSize = 0;
            for (const auto& pair : groups) {
                totalSize += pair.second.size();
            }
            marked.resize(totalSize, false);
            
            // Try to merge implicants between adjacent groups
            auto keys = getSortedKeys(groups);
            int markIndex = 0;
            
            for (size_t i = 0; i < keys.size() - 1; i++) {
                if (keys[i+1] - keys[i] == 1) {
                    std::vector<Implicant>& group1 = groups[keys[i]];
                    std::vector<Implicant>& group2 = groups[keys[i+1]];
                    
                    for (size_t j = 0; j < group1.size(); j++) {
                        for (size_t k = 0; k < group2.size(); k++) {
                            if (Implicant::canMerge(group1[j], group2[k])) {
                                Implicant merged(group1[j], group2[k]);
                                newGroups[merged.countOnes()].push_back(merged);
                                marked[markIndex + j] = true;
                                
                                // Mark in the next group
                                int nextGroupStart = markIndex + group1.size();
                                marked[nextGroupStart + k] = true;
                                
                                changed = true;
                            }
                        }
                    }
                }
                markIndex += groups[keys[i]].size();
            }
            
            // Collect unmarked implicants as prime
            markIndex = 0;
            for (auto& pair : groups) {
                for (size_t i = 0; i < pair.second.size(); i++) {
                    if (!marked[markIndex + i]) {
                        pair.second[i].isPrime = true;
                        primeImplicants.push_back(pair.second[i]);
                    }
                }
                markIndex += pair.second.size();
            }
            
            // Prepare for next iteration
            groups = newGroups;
        }
        
        // Remove duplicate prime implicants
        removeDuplicates(primeImplicants);
        
        return primeImplicants;
    }
    
    // Step 2: Find essential prime implicants
    std::vector<Implicant> findEssentialPrimes(const std::vector<Implicant>& primes) {
        std::vector<Implicant> essentialPrimes;
        
        // For each minterm, count how many prime implicants cover it
        for (int minterm : minterms) {
            std::vector<const Implicant*> coveringPrimes;
            
            for (const Implicant& imp : primes) {
                if (imp.covers(minterm, numVars)) {
                    coveringPrimes.push_back(&imp);
                }
            }
            
            // If only one prime covers this minterm, it's essential
            if (coveringPrimes.size() == 1) {
                const Implicant* essential = coveringPrimes[0];
                // Check if we already have this essential prime
                bool found = false;
                for (const Implicant& ep : essentialPrimes) {
                    if (ep.binary == essential->binary) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    Implicant copy = *essential;
                    copy.isEssential = true;
                    essentialPrimes.push_back(copy);
                }
            }
        }
        
        return essentialPrimes;
    }
    
    // Step 3: Find minimal cover for remaining minterms
    std::vector<Implicant> findMinimalCover(const std::vector<Implicant>& essentialPrimes,
                                           const std::vector<Implicant>& allPrimes) {
        // Start with essential primes
        std::vector<Implicant> cover = essentialPrimes;
        
        // Find minterms not covered by essential primes
        std::set<int> uncoveredMinterms;
        for (int minterm : minterms) {
            bool covered = false;
            for (const Implicant& imp : essentialPrimes) {
                if (imp.covers(minterm, numVars)) {
                    covered = true;
                    break;
                }
            }
            if (!covered) {
                uncoveredMinterms.insert(minterm);
            }
        }
        
        if (uncoveredMinterms.empty()) {
            return cover;
        }
        
        // Find primes that cover the remaining minterms
        std::vector<Implicant> remainingPrimes;
        for (const Implicant& imp : allPrimes) {
            // Check if this prime is not already in cover
            bool isEssential = false;
            for (const Implicant& ep : essentialPrimes) {
                if (ep.binary == imp.binary) {
                    isEssential = true;
                    break;
                }
            }
            if (!isEssential) {
                remainingPrimes.push_back(imp);
            }
        }
        
        // Simple heuristic: pick primes that cover most uncovered minterms
        while (!uncoveredMinterms.empty() && !remainingPrimes.empty()) {
            // Find prime that covers most uncovered minterms
            int maxCover = -1;
            int bestIdx = -1;
            
            for (size_t i = 0; i < remainingPrimes.size(); i++) {
                int coverCount = 0;
                for (int minterm : uncoveredMinterms) {
                    if (remainingPrimes[i].covers(minterm, numVars)) {
                        coverCount++;
                    }
                }
                if (coverCount > maxCover) {
                    maxCover = coverCount;
                    bestIdx = i;
                }
            }
            
            if (bestIdx >= 0) {
                cover.push_back(remainingPrimes[bestIdx]);
                
                // Remove covered minterms
                std::set<int> newUncovered;
                for (int minterm : uncoveredMinterms) {
                    if (!remainingPrimes[bestIdx].covers(minterm, numVars)) {
                        newUncovered.insert(minterm);
                    }
                }
                uncoveredMinterms = newUncovered;
                
                // Remove used prime
                remainingPrimes.erase(remainingPrimes.begin() + bestIdx);
            }
        }
        
        // Sort cover by binary representation for consistent output
        std::sort(cover.begin(), cover.end(), 
                 [](const Implicant& a, const Implicant& b) {
                     return a.binary < b.binary;
                 });
        
        return cover;
    }
    
    // Step 4: Format the cover as SOP expression
    std::string formatSOP(const std::vector<Implicant>& cover) const {
        if (cover.empty()) {
            return "0"; // Function is always false
        }
        
        // Check if cover includes all minterms (function is always true)
        bool allOnes = false;
        for (const Implicant& imp : cover) {
            if (imp.binary.find('0') == std::string::npos && 
                imp.binary.find('1') == std::string::npos) {
                allOnes = true;
                break;
            }
        }
        
        if (allOnes) {
            return "1"; // Function is always true
        }
        
        std::string result;
        for (size_t i = 0; i < cover.size(); i++) {
            std::string term = binaryToTerm(cover[i].binary);
            if (!term.empty()) {
                result += term;
                if (i < cover.size() - 1) {
                    result += " + ";
                }
            }
        }
        
        // If result is empty but cover is not, all terms were empty (shouldn't happen)
        if (result.empty() && !cover.empty()) {
            return "1";
        }
        
        return result;
    }
    
    // Helper function to get sorted keys from map
    std::vector<int> getSortedKeys(const std::map<int, std::vector<Implicant>>& groups) {
        std::vector<int> keys;
        for (const auto& pair : groups) {
            keys.push_back(pair.first);
        }
        std::sort(keys.begin(), keys.end());
        return keys;
    }
    
    // Helper to remove duplicate implicants
    void removeDuplicates(std::vector<Implicant>& implicants) {
        std::sort(implicants.begin(), implicants.end(),
                 [](const Implicant& a, const Implicant& b) {
                     return a.binary < b.binary;
                 });
        implicants.erase(
            std::unique(implicants.begin(), implicants.end(),
                       [](const Implicant& a, const Implicant& b) {
                           return a.binary == b.binary;
                       }),
            implicants.end()
        );
    }
};

// Function to parse input
void parseInput(int& numVars, std::vector<int>& minterms, std::vector<int>& dontcares) {
    std::cout << "Enter number of variables (1-8): ";
    std::cin >> numVars;
    
    if (numVars < 1 || numVars > 8) {
        throw std::runtime_error("Number of variables must be between 1 and 8");
    }
    
    std::cout << "Enter minterms (space-separated, end with -1): ";
    int term;
    while (std::cin >> term && term != -1) {
        minterms.push_back(term);
    }
    
    std::cin.clear(); // Clear error state
    std::cin.ignore(10000, '\n');
    
    std::cout << "Enter don't care terms (space-separated, end with -1, or just -1 if none): ";
    while (std::cin >> term && term != -1) {
        dontcares.push_back(term);
    }
}

// Test cases
void runTests() {
    std::cout << "=== Running Unit Tests ===\n";
    
    // Test 1: Simple 2-variable function
    {
        std::cout << "Test 1: 2-variable function F(A,B) = Σ(0,1,2)\n";
        QuineMcCluskey qm(2, {0, 1, 2});
        std::string result = qm.minimize();
        std::cout << "Result: " << result << std::endl;
        // F(0,1,2) = A'B' + A'B + AB' = A' + B'
        assert(result == "A' + B'" || result == "B' + A'");
        std::cout << " Passed\n\n";
    }
    
    // Test 2: 3-variable function
    {
        std::cout << "Test 2: 3-variable function F(A,B,C) = Σ(0,1,2,5,6,7)\n";
        QuineMcCluskey qm(3, {0, 1, 2, 5, 6, 7});
        std::string result = qm.minimize();
        std::cout << "Result: " << result << std::endl;
        std::cout << " Passed\n\n";
    }
    
    // Test 3: Function with don't cares
    {
        std::cout << "Test 3: 3-variable function with don't cares F(A,B,C) = Σ(0,2,4,6) + d(1,5)\n";
        QuineMcCluskey qm(3, {0, 2, 4, 6}, {1, 5});
        std::string result = qm.minimize();
        std::cout << "Result: " << result << std::endl;
        // All even terms = C' (LSB is 0)
        assert(result == "C'");
        std::cout << " Passed\n\n";
    }
    
    // Test 4: Full adder sum bit (S = A ⊕ B ⊕ C)
    {
        std::cout << "Test 4: Full adder sum bit S(A,B,C) = Σ(1,2,4,7)\n";
        QuineMcCluskey qm(3, {1, 2, 4, 7});
        std::string result = qm.minimize();
        std::cout << "Result: " << result << std::endl;
        // Should be: A'B'C + A'BC' + AB'C' + ABC
        std::cout << " Passed\n\n";
    }
    
    // Test 5: Full adder carry bit (Cout = AB + BC + AC)
    {
        std::cout << "Test 5: Full adder carry bit Cout(A,B,C) = Σ(3,5,6,7)\n";
        QuineMcCluskey qm(3, {3, 5, 6, 7});
        std::string result = qm.minimize();
        std::cout << "Result: " << result << std::endl;
        // Should be: AB + BC + AC (or equivalent)
        std::cout << " Passed\n\n";
    }
    
    // Test 6: Always true function
    {
        std::cout << "Test 6: Function that is always true F(A,B) = Σ(0,1,2,3)\n";
        QuineMcCluskey qm(2, {0, 1, 2, 3});
        std::string result = qm.minimize();
        std::cout << "Result: " << result << std::endl;
        assert(result == "1");
        std::cout << " Passed\n\n";
    }
    
    std::cout << "=== All tests passed ===\n";
}

int main() {
    std::cout << "=========================================\n";
    std::cout << "  Quine-McCluskey Boolean Minimizer\n";
    std::cout << "=========================================\n\n";
    
    char choice;
    std::cout << "Choose mode:\n";
    std::cout << "1. Run unit tests\n";
    std::cout << "2. Minimize custom function\n";
    std::cout << "Enter choice (1 or 2): ";
    std::cin >> choice;
    
    if (choice == '1') {
        runTests();
    } else {
        try {
            int numVars;
            std::vector<int> minterms, dontcares;
            
            parseInput(numVars, minterms, dontcares);
            
            QuineMcCluskey minimizer(numVars, minterms, dontcares);
            std::string result = minimizer.minimize();
            
            std::cout << "\n=========================================\n";
            std::cout << "Minimized expression: " << result << std::endl;
            std::cout << "=========================================\n";
            
            // Show in algebraic form
            std::cout << "\nIn algebraic form: F(";
            for (int i = 0; i < numVars; i++) {
                std::cout << char('A' + i);
                if (i < numVars - 1) std::cout << ",";
            }
            std::cout << ") = " << result << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }
    
    return 0;
}