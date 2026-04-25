# Triangle: A Two-Dimensional Quality Mesh Generator

This repository is a **simple port** of the original **Triangle** library, a world-renowned tool for generating high-quality 2D meshes, Delaunay triangulations, and Voronoi diagrams.

## Disclaimer & Credits

**Absolutely no credit is taken for the original algorithms, logic, or implementation of Triangle.** 

The original code was authored by:
**Jonathan Richard Shewchuk**
University of California at Berkeley

All intellectual property rights and the core engineering remain with the original author. This repository serves only as a modernized and streamlined port for specific environments.

---

## Contributions in this Port

This version includes the following modifications and enhancements:

1.  **Removal of `showme` on macOS**: The `showme` visualization tool has been excluded from the macOS build process to focus on the core library functionality.
2.  **New Benchmark Suite**: Added a comprehensive set of benchmark codes to evaluate performance across different operations:
    *   `benchmark_delaunay`: Evaluates Delaunay triangulation performance.
    *   `benchmark_hull`: Benchmarks convex hull generation.
    *   `benchmark_voronoi`: Measures Voronoi diagram construction speed.
3.  **Removal of X11 Dependencies**: All X11-dependent code and build requirements have been removed, making the library easier to compile and integrate into modern, headless, or cross-platform environments.
4.  **Modern C++ Wrapper**: Added a high-level C++11 interface (`TriangleMesh`) providing:
    *   **RAII Memory Management**: Automatic cleanup using `trifree`.
    *   **High-Level Primitives**: Easy methods to add polygons, circles, and bounding boxes.
    *   **Centroidal Voronoi Relaxation (Lloyd's Algorithm)**: Smooth existing meshes into high-quality isotropic triangulations.
    *   **CVT Generation**: A "one-shot" generator to fill any domain with a perfectly distributed set of organic-looking cells.

---

## C++ Usage Example

```cpp
#include "src/Triangle.hpp"

triangle::TriangleMesh mesh;

// Define a domain
mesh.addBoundingBox(0, 0, 100, 100);
mesh.addCircle(50, 50, 20, 30); // Add a circular boundary
mesh.addHole(50, 50);          // Make the circle a hole

// Generate 500 points with Centroidal Voronoi distribution
mesh.generateCVT(500, 20); 

// Access results
std::cout << "Generated " << mesh.numTriangles() << " quality triangles." << std::endl;
```

## Original Features

Triangle remains one of the most robust tools for:
*   **Delaunay Triangulations**: Exact and constrained.
*   **Quality Meshing**: Generating meshes with no small or large angles, suitable for Finite Element Analysis (FEA).
*   **Voronoi Diagrams**: Reliable construction from point sets.
*   **Robust Arithmetic**: Using adaptive precision floating-point arithmetic for geometric predicates.

## Compilation

The project uses a standard `makefile`. To build the library and benchmarks:

```bash
make
```

The binaries will be generated in the `bin/` directory.

## Licensing

Please refer to the original `README` and the comments in `src/triangle.c` for licensing details. The original author's copyright notices must be preserved in all copies of the code.
