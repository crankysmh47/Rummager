#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <numeric>

// --- CONFIGURATION ---
const double DAMPING_FACTOR = 0.85;
const int MAX_ITERATIONS = 50;
const double CONVERGENCE_THRESHOLD = 1e-9;

struct Node {
    int id;
    std::vector<int> outbound_links;
};

int main() {
    std::cout << "Loading Graph..." << std::endl;
    std::ifstream infile("C:\\Users\\Hank47\\Sem3\\Rummager\\graph.txt");
    if (!infile.is_open()) {
        std::cerr << "Error: Could not open graph.txt" << std::endl;
        return 1;
    }

    int N; // Total nodes
    infile >> N; 

    // Adjacency list
    std::vector<std::vector<int>> adj(N);
    // Track out-degree for standard calculation
    std::vector<int> out_degree(N, 0);

    int u, degree, v;
    while (infile >> u >> degree) {
        out_degree[u] = degree;
        for (int i = 0; i < degree; i++) {
            infile >> v;
            adj[u].push_back(v);
        }
    }
    infile.close();

    // Initialize PageRank: Everyone gets 1/N
    std::vector<double> PR(N, 1.0 / N);
    std::vector<double> next_PR(N, 0.0);

    std::cout << "Graph Loaded. Nodes: " << N << ". Starting PageRank..." << std::endl;

    for (int iter = 0; iter < MAX_ITERATIONS; iter++) {
        // Reset next_PR to the base "teleport" probability
        std::fill(next_PR.begin(), next_PR.end(), (1.0 - DAMPING_FACTOR) / N);

        double dangling_sum = 0.0;

        // Distribute scores
        for (int i = 0; i < N; i++) {
            if (out_degree[i] == 0) {
                // Dangling node: Its score is distributed evenly to everyone
                dangling_sum += PR[i];
            } else {
                // Normal node: Distribute score to neighbors
                double share = PR[i] / out_degree[i];
                for (int neighbor : adj[i]) {
                    next_PR[neighbor] += DAMPING_FACTOR * share;
                }
            }
        }

        // Add the dangling node share to everyone
        double dangling_share = (DAMPING_FACTOR * dangling_sum) / N;
        for (int i = 0; i < N; i++) {
            next_PR[i] += dangling_share;
        }

        // Check convergence (Manhattan distance)
        double error = 0.0;
        for (int i = 0; i < N; i++) {
            error += std::abs(next_PR[i] - PR[i]);
        }

        PR = next_PR; // Update for next round

        std::cout << "Iteration " << iter + 1 << " Error: " << error << std::endl;
        if (error < CONVERGENCE_THRESHOLD) break;
    }

    // Save Output
    std::cout << "Saving Scores..." << std::endl;
    std::ofstream outfile("pagerank_scores.txt");
    // Format: IntID Score
    for (int i = 0; i < N; i++) {
        outfile << i << " " << PR[i] << "\n";
    }
    outfile.close();

    std::cout << "Done." << std::endl;
    return 0;
}