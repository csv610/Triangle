#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>

// Forward declaration of the C structure
struct triangulateio;

namespace triangle {

/**
 * @brief Configuration options for the Triangle mesh generator.
 */
struct Options {
    bool quality = true;          // -q: quality mesh generation
    double minAngle = 20.0;       // -q<angle>: minimum angle constraint
    double maxArea = -1.0;        // -a<area>: maximum area constraint
    bool voronoi = false;         // -v: generate Voronoi diagram
    bool delaunay = true;         // default Delaunay
    bool conforming = false;      // -D: conforming Delaunay
    bool quiet = true;            // -Q: quiet mode
    bool zeroBased = true;        // -z: 0-based indexing (recommended for C++)
    bool computeNeighbors = false;// -n: output triangle neighbors
    bool computeEdges = false;    // -e: output edges
    std::string extraSwitches = ""; // Any other custom switches
};

/**
 * @brief A high-level C++ wrapper for the Triangle mesh generator.
 */
class TriangleMesh {
public:
    struct Point {
        double x, y;
        int marker;
        Point(double _x, double _y, int _m = 0) : x(_x), y(_y), marker(_m) {}
    };

    struct Segment {
        int p1, p2;
        int marker;
        Segment(int _p1, int _p2, int _m = 0) : p1(_p1), p2(_p2), marker(_m) {}
    };

    TriangleMesh();
    ~TriangleMesh();

    // --- Input Methods ---
    void addPoint(double x, double y, int marker = 0);
    void addSegment(int p1, int p2, int marker = 0);
    void addHole(double x, double y);
    void addRegion(double x, double y, double attribute, double maxArea = -1.0);

    // --- High-Level Primitive Helpers ---
    void addPolygon(const std::vector<Point>& points, bool closed = true);
    void addBoundingBox(double x1, double y1, double x2, double y2, int marker = 1);
    void addCircle(double cx, double cy, double radius, int numSegments, int marker = 0);
    void setTargetEdgeLength(double length);

    // --- Execution ---
    bool triangulate(const Options& opts = Options());

    // --- Output Accessors ---
    size_t numPoints() const;
    Point getPoint(size_t index) const;
    size_t numTriangles() const;
    void getTriangle(size_t index, int& p1, int& p2, int& p3) const;
    size_t numEdges() const;
    void getEdge(size_t index, int& p1, int& p2) const;
    size_t numVoronoiEdges() const;
    void getVoronoiEdge(size_t index, int& p1, int& p2) const;
    
    // --- Advanced Features ---
    void smooth(int iterations = 1);
    void clear();

private:
    std::vector<Point> m_points;
    std::vector<Segment> m_segments;
    std::vector<std::pair<double, double>> m_holes;
    std::vector<std::vector<double>> m_regions;
    double m_targetArea = -1.0;

    struct triangulateio *m_in;
    struct triangulateio *m_out;
    struct triangulateio *m_vorout;

    void initIO(struct triangulateio* io);
    void freeIO(struct triangulateio* io, bool sharedHoles = false);
    std::string buildSwitchString(const Options& opts);
};

} // namespace triangle
