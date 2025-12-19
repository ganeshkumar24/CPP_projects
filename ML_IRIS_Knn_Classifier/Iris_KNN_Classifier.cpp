// main.cpp - Iris k-NN Classifier from scratch
#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <cmath>
#include <random>
#include <execution>
#include <queue>
#include <map>
#include <iomanip>

struct IrisSample {
    double sepal_length;
    double sepal_width;
    double petal_length;
    double petal_width;
    std::string species;

    IrisSample(double sl, double sw, double pl, double pw, std::string sp)
        : sepal_length(sl), sepal_width(sw), petal_length(pl), petal_width(pw), species(std::move(sp)) {}
};

class KNNClassifier {
private:
    std::vector<IrisSample> training_data;
    std::vector<double> means = {0,0,0,0};
    std::vector<double> stds  = {1,1,1,1};
    int k = 5;

    double euclidean_distance(const IrisSample& a, const IrisSample& b) const {
        auto norm = [&](double val, int i) { return (val - means[i]) / stds[i]; };
        double d = 0.0;
        d += std::pow(norm(a.sepal_length,0) - norm(b.sepal_length,0), 2);
        d += std::pow(norm(a.sepal_width,1)  - norm(b.sepal_width,1),  2);
        d += std::pow(norm(a.petal_length,2) - norm(b.petal_length,2), 2);
        d += std::pow(norm(a.petal_width,3)  - norm(b.petal_width,3),  2);
        return std::sqrt(d);
    }

public:
    void fit(const std::vector<IrisSample>& data) {
        training_data = data;

        // Compute mean and std for normalization
        int n = data.size();
        for (int i = 0; i < 4; ++i) {
            double sum = 0.0;
            for (const auto& s : data) {
                if (i==0) sum += s.sepal_length;
                else if (i==1) sum += s.sepal_width;
                else if (i==2) sum += s.petal_length;
                else sum += s.petal_width;
            }
            means[i] = sum / n;
        }

        for (int i = 0; i < 4; ++i) {
            double sum_sq = 0.0;
            for (const auto& s : data) {
                double val = (i==0 ? s.sepal_length : i==1 ? s.sepal_width : i==2 ? s.petal_length : s.petal_width);
                sum_sq += std::pow(val - means[i], 2);
            }
            stds[i] = std::sqrt(sum_sq / n);
            if (stds[i] == 0) stds[i] = 1.0;
        }
    }

    std::string predict(const IrisSample& query) const {
        using Neighbor = std::pair<double, std::string>;
        std::priority_queue<Neighbor, std::vector<Neighbor>, std::greater<>> pq; // min-heap for k smallest

        std::vector<double> distances(training_data.size());
        std::transform(std::execution::par, training_data.begin(), training_data.end(), distances.begin(),
            [&](const IrisSample& train) { return euclidean_distance(query, train); });

        for (size_t i = 0; i < training_data.size(); ++i) {
            double dist = distances[i];
            if (pq.size() < static_cast<size_t>(k)) {
                pq.emplace(dist, training_data[i].species);
            } else if (dist < pq.top().first) {
                pq.pop();
                pq.emplace(dist, training_data[i].species);
            }
        }

        std::map<std::string, int> votes;
        while (!pq.empty()) {
            votes[pq.top().second]++;
            pq.pop();
        }

        return std::max_element(votes.begin(), votes.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; })->first;
    }
};

int main() {
    // Hardcoded Iris dataset (150 samples)
    std::vector<IrisSample> full_data = {
        // ... (include all 150 - abbreviated here for space; full list below in actual code)
        IrisSample(5.1,3.5,1.4,0.2,"setosa"), IrisSample(4.9,3.0,1.4,0.2,"setosa"),
        // Add all 150 from standard Iris dataset (you can copy from UCI repo)
        // For brevity, assume you paste the full 150 here.
        IrisSample(7.0,3.2,4.7,1.4,"versicolor"), /* ... */
        IrisSample(6.3,3.3,6.0,2.5,"virginica") /* last one */
    };

    // Shuffle and split
    std::mt19937 rng(42);
    std::shuffle(full_data.begin(), full_data.end(), rng);
    std::vector<IrisSample> train(full_data.begin(), full_data.begin() + 120);
    std::vector<IrisSample> test(full_data.begin() + 120, full_data.end());

    KNNClassifier knn;
    knn.fit(train);

    // Evaluate
    int correct = 0;
    std::map<std::string, std::map<std::string, int>> confusion;
    for (const auto& sample : test) {
        std::string pred = knn.predict(sample);
        confusion[sample.species][pred]++;
        if (pred == sample.species) ++correct;
    }

    double accuracy = static_cast<double>(correct) / test.size() * 100;
    std::cout << "Test Accuracy: " << std::fixed << std::setprecision(2) << accuracy << "%\n\n";

    std::cout << "Confusion Matrix:\n";
    std::vector<std::string> species = {"setosa", "versicolor", "virginica"};
    std::cout << "          Predicted →\nActual ↓  ";
    for (const auto& s : species) std::cout << std::setw(12) << s;
    std::cout << "\n";
    for (const auto& true_s : species) {
        std::cout << std::setw(9) << true_s;
        for (const auto& pred_s : species) {
            std::cout << std::setw(12) << confusion[true_s][pred_s];
        }
        std::cout << "\n";
    }

    // Example query
    IrisSample query(5.9, 3.0, 5.1, 1.8, ""); // Should be virginica
    std::cout << "\nQuery (sepal_l=5.9, sepal_w=3.0, petal_l=5.1, petal_w=1.8) → "
              << knn.predict(query) << "\n";

    return 0;
}