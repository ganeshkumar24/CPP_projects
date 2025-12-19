Iris Flower Species Classifier using k-Nearest Neighbors (from scratch in C++)
Description:
Implementation of k-Nearest Neighbors (k-NN) classifier entirely in C++ (no external ML libraries) to classify iris flowers into three species (setosa, versicolor, virginica) based on four features: sepal length, sepal width, petal length, and petal width. This uses the famous Iris dataset (150 samples, classic ML benchmark). 

The program:
Loads the dataset from a built-in array (hardcoded for simplicity), Normalizes features, Trains on 120 samples, Tests on 30 held-out samples, Computes accuracy.
Allows custom queries (e.g., predict species for new measurements), Uses parallel execution for distance calculations (C++17 parallel algorithms), Prints a simple text-based confusion matrix.

Demonstrates core ML concepts (distance metrics, voting, normalization) in C++ with advanced features: templates, concepts, lambdas, std::execution (parallelism), std::priority_queue, ranges, structs, random.

C++ Features Used:

Templates & concepts
Lambdas & std::function
std::vector, std::array, std::ranges
std::priority_queue
std::execution::par for parallelism
Structs with constructors
Random number generation