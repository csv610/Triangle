# Triangle: An Undergraduate Guide to 2D Meshing

## Preface: The Art of Discretization
Welcome to the study of computational geometry and mesh generation. In the world of engineering and physics, we rarely solve problems on continuous surfaces. Instead, we break the world into small, manageable pieces—a process called **discretization**. 

This guide introduces you to **Triangle**, a foundational tool in this field, and its modern C++ interface. Whether you are studying Finite Element Method (FEM), computer graphics, or geographic information systems, understanding how to generate high-quality triangles is a critical skill.

---

## Chapter 1: The Foundations of Triangulation

### 1.1 The Delaunay Criterion
At the heart of this library is the **Delaunay Triangulation**. For a given set of points, a Delaunay triangulation is one where no point in the set falls inside the circumcircle of any triangle in the mesh. 

**Why does this matter?** Delaunay triangles tend to be "fat" rather than "skinny." Skinny triangles cause numerical instability in simulations. By maximizing the minimum angle of all triangles, Delaunay ensures a stable foundation for math.

### 1.2 The Voronoi Diagram
Every Delaunay triangulation has a "dual" structure called the **Voronoi Diagram**. A Voronoi cell for a point consists of all locations in the plane closer to that point than to any other. These are used in everything from territory mapping to analyzing the grain structure of metals.

---

## Chapter 2: Setting Up the Laboratory

### 2.1 Compilation
This project has been modernized to work seamlessly on macOS and Linux. To build the tools:

```bash
make
```

This will produce several binaries in the `bin/` folder:
*   `triangle`: The original robust C command-line tool.
*   `test_cpp_wrapper`: An example of the new C++ interface.
*   `benchmark_*`: Performance testing tools.

---

## Chapter 3: The Modern C++ Interface

While the original `triangle.c` is powerful, it uses complex "switch-based" commands (like `-pq33A`). We have wrapped this in a modern C++ class to make your code more readable.

### 3.1 Your First Mesh
Here is how you define a simple domain in C++:

```cpp
#include "src/Triangle.hpp"

triangle::TriangleMesh mesh;

// 1. Define the boundary (a square)
mesh.addBoundingBox(0, 0, 10, 10);

// 2. Add an internal point
mesh.addPoint(5, 5);

// 3. Triangulate!
mesh.triangulate();
```

### 3.2 High-Level Primitives
Instead of manually calculating vertex indices, use the helper methods:
*   `addPolygon(...)`: Connects a sequence of points.
*   `addCircle(...)`: Approximates a circle with segments.
*   `addHole(...)`: Marks an area that should not be meshed.

---

## Chapter 4: Achieving Mesh Quality

A "raw" triangulation is rarely enough for engineering. We need to optimize it.

### 4.1 Laplacian Smoothing
The simplest form of optimization. It moves each vertex to the average of its neighbors. It is fast but can occasionally degrade the Delaunay property.

### 4.2 Centroidal Voronoi Relaxation (Lloyd's Method)
This moves each vertex to the **Centroid** of its Voronoi cell.
*   **Method**: `mesh.relaxVoronoi(iterations)`
*   **Result**: A beautiful, organic, "honeycomb" mesh. It is excellent for uniform point sampling.

### 4.3 Optimal Delaunay Triangulation (ODT)
Based on Hodge-Optimization theories, ODT moves vertices to the area-weighted average of triangle circumcenters.
*   **Method**: `mesh.relaxODT(iterations)`
*   **Result**: Mathematically superior for Finite Element Analysis. It minimizes the error in linear interpolation across the mesh.

---

## Chapter 5: Advanced Generation (CVT)

If you don't want to place points manually, you can use the **Centroidal Voronoi Tessellation (CVT)** generator.

```cpp
// Fill the domain with 1000 optimized points
mesh.generateCVT(1000, 20); 
```

This "one-shot" function automatically handles the random seeding, iterative triangulation, and relaxation required to fill a complex shape with a perfectly distributed set of triangles.

---

## Appendix: Honor and Attribution

As scientists, we stand on the shoulders of giants. The core logic of this project is the work of **Jonathan Richard Shewchuk** (UC Berkeley). This port adds modern C++ conveniences and benchmark tools, but the brilliant robust arithmetic and triangulation logic remain his.

**Absolutely no credit is taken for the original algorithms.** Use this tool to build great things, and always acknowledge the original source.
