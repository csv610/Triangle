#pragma once

#include "TriMeshGenerator.hpp"
#include <vector>
#include <functional>

namespace triangle {

/**
 * @brief Specialized class for mesh optimization and smoothing.
 */
class MeshOptimizer {
public:
    /// Returns true if the point should remain fixed (not moved).
    using FixedPredicate = std::function<bool(const Point2D&)>;

    static void smooth(std::vector<Point2D>& points, 
                      const std::vector<Triangle>& triangles, 
                      int iterations = 1,
                      FixedPredicate isFixed = nullptr);

    static void relaxVoronoi(std::vector<Point2D>& points, 
                            const std::vector<Triangle>& triangles, 
                            int iterations = 1,
                            FixedPredicate isFixed = nullptr);

    static void relaxODT(std::vector<Point2D>& points, 
                        const std::vector<Triangle>& triangles, 
                        int iterations = 1,
                        FixedPredicate isFixed = nullptr);
};

} // namespace triangle
