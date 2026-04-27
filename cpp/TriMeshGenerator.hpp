#pragma once

#include <vector>
#include <string>
#include <functional>

namespace triangle {

/**
 * @brief Clean, modern data structures for mesh data.
 */
struct Point2D {
    double x, y;
    int marker;
    bool isFixed;
    Point2D(double _x = 0, double _y = 0, int _m = 0, bool _f = false) 
        : x(_x), y(_y), marker(_m), isFixed(_f) {}
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

struct VoroFace {
    int siteIndex;                  // Index of the input point (site)
    std::vector<Point2D> vertices;  // Ordered vertices of the Voronoi cell
    bool isClosed;                  // False if the cell extends to infinity
    VoroFace() : siteIndex(-1), isClosed(true) {}
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
    void addPoint(double x, double y, int marker = 0, bool isFixed = false);
    void addPoint(const Point2D& p) { addPoint(p.x, p.y, p.marker, p.isFixed); }
    void addSegment(int p1, int p2, int marker = 0);
    void addHole(double x, double y);
    
    // --- High-Level Helpers ---
    void addPolygon(const std::vector<Point2D>& points, bool closed = true);
    void addBoundingBox(double x1, double y1, double x2, double y2, int marker = 1);
    void addCircle(double cx, double cy, double radius, int segments, int marker = 0);

    // --- Refinement ---
    void refineSegments(double maxLength);

    // --- Execution ---
    bool triangulate(const Options& opts = Options());

    // --- High-Level Algorithms ---
    void generateCVT(int numPoints, int iterations = 20);

    // --- Bulk Output API (The "Clean" Way) ---
    const std::vector<Point2D>&  points()    const { return m_outPoints; }
    const std::vector<Triangle>& triangles() const { return m_outTriangles; }
    const std::vector<Edge>&     edges()     const { return m_outEdges; }
    const std::vector<Edge>&     segments()  const { return m_outSegments; }
    const std::vector<Edge>&     voronoi()   const { return m_outVoronoi; }
    const std::vector<VoroFace>& voroFaces() const { return m_outVoroFaces; }

    void clear();

    // Query Input State
    int numInputPoints()   const { return (int)m_inPoints.size(); }
    int numInputSegments() const { return (int)m_inSegments.size(); }
    const std::vector<Point2D>& inputPoints()   const { return m_inPoints; }
    const std::vector<Edge>&    inputSegments() const { return m_inSegments; }

    // Grant access to internal state for editor
    std::vector<Point2D>&  mutablePoints()      { return m_outPoints; }
    std::vector<Point2D>&  mutableInputPoints() { return m_inPoints; }

private:
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
    std::vector<Edge>     m_outSegments;
    std::vector<Edge>     m_outVoronoi;
    std::vector<VoroFace> m_outVoroFaces;

    void syncOutput();
    void resolveIntersections();
};

} // namespace triangle
