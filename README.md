# Triangle: Modern 2D Quality Mesh Generator

This repository is a modernized, professional-grade C++11 port of the original **Triangle** library. It provides a clean, encapsulated interface for generating high-quality 2D meshes, Delaunay triangulations, and Voronoi diagrams while stripping away legacy dependencies like X11.

## Key Modern Features
*   **Professional Encapsulation**: Uses the PIMPL pattern to completely hide legacy C structures and implementation details from your project.
*   **Modern C++ API**: Inputs and outputs use standard `std::vector` containers and clean types like `Point2D`, `Triangle`, and `VoroFace`.
*   **Advanced Optimization Suite**:
    *   **Lloyd's Relaxation**: For geometric symmetry and "honeycomb" Voronoi cells.
    *   **Optimal Delaunay Triangulation (ODT)**: Minimizes interpolation error for physics simulations.
    *   **Centroidal Voronoi Tessellation (CVT)**: One-shot generator for organic point distributions.
*   **Per-Site Voronoi Access**: Access the Voronoi diagram as a collection of ordered, closed or open polygons.
*   **macOS & Linux Optimized**: Removed X11 dependencies for easy integration into modern development environments.

## Quick C++ Usage

```cpp
#include "src/Triangle.hpp"

triangle::TriangleMesh mesh;

// Define a domain
mesh.addBoundingBox(0, 0, 10, 10);
mesh.addCircle(5, 5, 2, 20); // Add a circular hole
mesh.addHole(5, 5);

// Generate 500 points with Centroidal Voronoi distribution
mesh.generateCVT(500, 20); 

// Access results directly
for (const auto& face : mesh.voroFaces()) {
    if (face.isClosed) {
        // Process internal Voronoi cell...
    }
}
```

## Credits
**Absolutely no credit is taken for the original algorithms.**
Original author: **Jonathan Richard Shewchuk** (UC Berkeley).
This port maintains all original copyright notices and focuses on modernization, documentation, and advanced optimization helpers.

## License
Please refer to `src/triangle.c` for the original licensing terms. This port is provided as-is for research and professional use.
