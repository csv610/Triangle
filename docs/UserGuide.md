# Triangle: An Undergraduate Guide to 2D Meshing

## Preface: The Art of Discretization
Welcome to the study of computational geometry. This guide introduces you to **Triangle**, a world-class tool for mesh generation, now wrapped in a professional C++11 interface.

---

## Chapter 1: The Modern API
The new C++ interface is designed for safety and ease of use. It uses the PIMPL pattern to hide legacy "gory details" and provides clean data structures.

### 1.1 Key Data Structures
*   `Point2D`: A simple $(x, y)$ coordinate with a boundary marker.
*   `Triangle`: A triplet of indices into the point list.
*   `VoroFace`: A site-specific Voronoi cell containing an ordered list of vertices.

### 1.2 Accessing Your Results
After calling `triangulate()`, you can access your results through simple constant references:
```cpp
const auto& points = mesh.points();
const auto& triangles = mesh.triangles();
const auto& voroCells = mesh.voroFaces();
```

---

## Chapter 2: Voronoi Analysis
The Voronoi diagram is the geometric dual of the Delaunay triangulation.

### 2.1 Closed vs. Open Cells
A **Closed** cell is a finite polygon entirely contained within your domain. An **Open** cell is one where the site is on the boundary, and the cell theoretically extends to infinity. 
*   `face.isClosed`: Returns `true` if the cell is a bounded polygon.
*   `face.vertices`: An ordered `std::vector<Point2D>` of the cell's corners.

---

## Chapter 3: Advanced Optimization
Triangle provides three distinct methods for improving mesh quality.

### 3.1 Lloyd's Relaxation (CVT)
Lloyd's method moves each vertex to the **centroid of its Voronoi cell**. 
*   **Goal**: Geometric regularity.
*   **Result**: Triangles become nearly equilateral, and Voronoi cells become regular hexagons.

### 3.2 Optimal Delaunay Triangulation (ODT)
ODT moves vertices to the **area-weighted average of circumcenters**.
*   **Goal**: Numerical accuracy.
*   **Result**: Minimizes interpolation errors, making it the gold standard for Finite Element Analysis (FEA).

### 3.3 Laplacian Smoothing
A fast, topological smoother that moves vertices to the average of their neighbors. 

---

## Chapter 4: Automated Generation
The `generateCVT()` method allows you to fill a complex domain with a specified number of points. It automatically iterates through triangulation and relaxation to find the optimal distribution.

```cpp
mesh.generateCVT(numPoints, iterations);
```

---

## Appendix: Licensing and Ethics
The core logic is the intellectual property of **Jonathan Richard Shewchuk**. This guide and port are intended to honor that work by making it more accessible to the modern engineering community. Always cite the original author in your research.
