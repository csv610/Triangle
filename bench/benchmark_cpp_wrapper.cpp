#include "TriMeshGenerator.hpp"
#include <iostream>
#include <chrono>
#include <vector>

using namespace triangle;

int main() {
    TriangleMesh mesh;
    mesh.addBoundingBox(0, 0, 100, 100);

    const int numPoints = 10000;
    const int iterations = 10;

    std::cout << "Starting Benchmarks (Points: " << numPoints << ", Iterations: " << iterations << ")" << std::endl;

    // 1. CVT Generation Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    mesh.generateCVT(numPoints, iterations);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "generateCVT (10000 pts, 10 steps): " << diff.count() << " seconds" << std::endl;

    // 2. Lloyd Relaxation Benchmark
    start = std::chrono::high_resolution_clock::now();
    MeshOptimizer::relaxVoronoi(mesh.mutablePoints(), mesh.triangles(), iterations);
    end = std::chrono::high_resolution_clock::now();
    diff = end - start;
    std::cout << "relaxVoronoi (10 steps): " << diff.count() << " seconds" << std::endl;

    // 3. ODT Relaxation Benchmark
    start = std::chrono::high_resolution_clock::now();
    MeshOptimizer::relaxODT(mesh.mutablePoints(), mesh.triangles(), iterations);
    end = std::chrono::high_resolution_clock::now();
    diff = end - start;
    std::cout << "relaxODT (10 steps): " << diff.count() << " seconds" << std::endl;

    // 4. Per-site Voronoi Reconstruction Benchmark
    start = std::chrono::high_resolution_clock::now();
    mesh.triangulate(); // Syncs output including faces
    end = std::chrono::high_resolution_clock::now();
    diff = end - start;
    std::cout << "Triangulate + Voronoi Face Sync: " << diff.count() << " seconds" << std::endl;

    return 0;
}
