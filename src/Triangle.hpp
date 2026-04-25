#pragma once

#include <vector>
#include <string>

namespace triangle {

/**
 * @brief Clean, modern data structures for mesh data.
 */
struct Point2D {
    double x, y;
    int marker;
    Point2D(double _x = 0, double _y = 0, int _m = 0) : x(_x), y(_y), marker(_m) {}
};

struct Triangle {
    int p[3];
    Triangle(int p1 = 0, int p2 = 0, int p3 = 0) { p[0] = p1; p[1] = p2; p[2] = p3; }
};

struct Edge {
    int p[2];
    int marker;
    Edge(int p1 = 0, int p2 = 0, int _m = 0) : marker(_m) { p[0] = p1; p[1] = p2; }
};

/**
 * @brief Configuration options using modern types.
 */
struct Options {
    bool quality = true;
    double minAngle = 20.0;
    double maxArea = -1.0;
    bool voronoi = false;
    bool conforming = false;
    bool quiet = true;
    std::string extraSwitches = "";
};

/**
 * @brief The TriangleMesh class. 
 * Completely hides the legacy C 'triangulateio' structures.
 */
class TriangleMesh {
public:
    TriangleMesh();
    ~TriangleMesh();

    // --- Input API (Modern Types) ---
    void addPoint(double x, double y, int marker = 0);
    void addPoint(const Point2D& p) { addPoint(p.x, p.y, p.marker); }
    void addSegment(int p1, int p2, int marker = 0);
    void addHole(double x, double y);
    
    // --- High-Level Helpers ---
    void addPolygon(const std::vector<Point2D>& points, bool closed = true);
    void addBoundingBox(double x1, double y1, double x2, double y2, int marker = 1);
    void addCircle(double cx, double cy, double radius, int segments, int marker = 0);

    // --- Execution ---
    bool triangulate(const Options& opts = Options());

    // --- Optimization ---
    void smooth(int iterations = 1);
    void relaxVoronoi(int iterations = 1);
    void relaxODT(int iterations = 1);
    void generateCVT(int numPoints, int iterations = 20);

    // --- Bulk Output API (The "Clean" Way) ---
    const std::vector<Point2D>&  points()    const { return m_outPoints; }
    const std::vector<Triangle>& triangles() const { return m_outTriangles; }
    const std::vector<Edge>&     edges()     const { return m_outEdges; }
    const std::vector<Edge>&     voronoi()   const { return m_outVoronoi; }

    void clear();

private:
    // Implementation details hidden via opaque pointers or private members
    struct Impl;
    Impl* pImpl;

    // Staging vectors
    std::vector<Point2D> m_inPoints;
    std::vector<Edge>    m_inSegments;
    std::vector<Point2D> m_inHoles;

    // Result vectors
    std::vector<Point2D>  m_outPoints;
    std::vector<Triangle> m_outTriangles;
    std::vector<Edge>     m_outEdges;
    std::vector<Edge>     m_outVoronoi;

    void syncOutput();
};

} // namespace triangle
