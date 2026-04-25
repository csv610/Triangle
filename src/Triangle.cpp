#include "Triangle.hpp"
#include <iostream>
#include <sstream>
#include <cmath>
#include <cstring>
#include <map>

// Define REAL before including triangle.h as it expects it
#ifndef REAL
#define REAL double
#endif

#ifndef VOID
#define VOID void
#endif

#ifndef ANSI_DECLARATORS
#define ANSI_DECLARATORS
#endif

// Include the original C header
extern "C" {
    #include "triangle.h"
}

// Define M_PI if not available
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace triangle {

TriangleMesh::TriangleMesh() {
    m_in = new struct triangulateio;
    m_out = new struct triangulateio;
    m_vorout = new struct triangulateio;
    
    initIO(m_in);
    initIO(m_out);
    initIO(m_vorout);
}

TriangleMesh::~TriangleMesh() {
    freeIO(m_out);
    freeIO(m_vorout);
    
    delete m_in;
    delete m_out;
    delete m_vorout;
}

void TriangleMesh::initIO(struct triangulateio* io) {
    std::memset(io, 0, sizeof(struct triangulateio));
}

void TriangleMesh::freeIO(struct triangulateio* io, bool sharedHoles) {
    if (io->pointlist) { trifree(io->pointlist); io->pointlist = NULL; }
    if (io->pointattributelist) { trifree(io->pointattributelist); io->pointattributelist = NULL; }
    if (io->pointmarkerlist) { trifree(io->pointmarkerlist); io->pointmarkerlist = NULL; }
    
    if (io->trianglelist) { trifree(io->trianglelist); io->trianglelist = NULL; }
    if (io->triangleattributelist) { trifree(io->triangleattributelist); io->triangleattributelist = NULL; }
    if (io->trianglearealist) { trifree(io->trianglearealist); io->trianglearealist = NULL; }
    if (io->neighborlist) { trifree(io->neighborlist); io->neighborlist = NULL; }
    
    if (io->segmentlist) { trifree(io->segmentlist); io->segmentlist = NULL; }
    if (io->segmentmarkerlist) { trifree(io->segmentmarkerlist); io->segmentmarkerlist = NULL; }
    
    if (!sharedHoles) {
        if (io->holelist) { trifree(io->holelist); io->holelist = NULL; }
        if (io->regionlist) { trifree(io->regionlist); io->regionlist = NULL; }
    }
    
    if (io->edgelist) { trifree(io->edgelist); io->edgelist = NULL; }
    if (io->edgemarkerlist) { trifree(io->edgemarkerlist); io->edgemarkerlist = NULL; }
    if (io->normlist) { trifree(io->normlist); io->normlist = NULL; }
    
    initIO(io);
}

void TriangleMesh::addPoint(double x, double y, int marker) {
    m_points.push_back(Point(x, y, marker));
}

void TriangleMesh::addSegment(int p1, int p2, int marker) {
    m_segments.push_back(Segment(p1, p2, marker));
}

void TriangleMesh::addHole(double x, double y) {
    m_holes.push_back(std::make_pair(x, y));
}

void TriangleMesh::addRegion(double x, double y, double attribute, double maxArea) {
    std::vector<double> region = {x, y, attribute, maxArea};
    m_regions.push_back(region);
}

void TriangleMesh::addPolygon(const std::vector<Point>& points, bool closed) {
    int startIdx = m_points.size();
    for (const auto& p : points) {
        addPoint(p.x, p.y, p.marker);
    }
    int count = points.size();
    if (count < 2) return;

    for (int i = 0; i < count - 1; ++i) {
        addSegment(startIdx + i, startIdx + i + 1, points[i].marker);
    }
    if (closed && count > 2) {
        addSegment(startIdx + count - 1, startIdx, points[count-1].marker);
    }
}

void TriangleMesh::addBoundingBox(double x1, double y1, double x2, double y2, int marker) {
    std::vector<Point> box;
    box.push_back(Point(x1, y1, marker));
    box.push_back(Point(x2, y1, marker));
    box.push_back(Point(x2, y2, marker));
    box.push_back(Point(x1, y2, marker));
    addPolygon(box, true);
}

void TriangleMesh::addCircle(double cx, double cy, double radius, int numSegments, int marker) {
    std::vector<Point> circle;
    for (int i = 0; i < numSegments; ++i) {
        double angle = 2.0 * M_PI * i / numSegments;
        circle.push_back(Point(cx + radius * std::cos(angle), cy + radius * std::sin(angle), marker));
    }
    addPolygon(circle, true);
}

void TriangleMesh::setTargetEdgeLength(double length) {
    // Area of equilateral triangle A = (sqrt(3)/4) * L^2
    m_targetArea = (std::sqrt(3.0) / 4.0) * length * length;
}

std::string TriangleMesh::buildSwitchString(const Options& opts) {
    std::stringstream ss;
    if (m_segments.size() > 0) ss << "p";
    if (opts.quality) {
        ss << "q";
        if (opts.minAngle > 0) ss << opts.minAngle;
    }
    if (opts.maxArea > 0) ss << "a" << opts.maxArea;
    else if (m_targetArea > 0) ss << "a" << m_targetArea;
    
    if (opts.voronoi) ss << "v";
    if (opts.conforming) ss << "D";
    if (opts.quiet) ss << "Q";
    if (opts.zeroBased) ss << "z";
    if (opts.computeNeighbors) ss << "n";
    if (opts.computeEdges) ss << "e";
    
    ss << opts.extraSwitches;
    return ss.str();
}

bool TriangleMesh::triangulate(const Options& opts) {
    // Clear previous output (but don't free holelist/regionlist since they are pointers to input)
    freeIO(m_out, true); // true = shared pointers
    freeIO(m_vorout, true);
    
    // Prepare input
    m_in->numberofpoints = m_points.size();
    m_in->pointlist = (REAL*)malloc(m_in->numberofpoints * 2 * sizeof(REAL));
    m_in->pointmarkerlist = (int*)malloc(m_in->numberofpoints * sizeof(int));
    for (int i = 0; i < m_in->numberofpoints; ++i) {
        m_in->pointlist[i * 2] = m_points[i].x;
        m_in->pointlist[i * 2 + 1] = m_points[i].y;
        m_in->pointmarkerlist[i] = m_points[i].marker;
    }

    if (!m_segments.empty()) {
        m_in->numberofsegments = m_segments.size();
        m_in->segmentlist = (int*)malloc(m_in->numberofsegments * 2 * sizeof(int));
        m_in->segmentmarkerlist = (int*)malloc(m_in->numberofsegments * sizeof(int));
        for (int i = 0; i < m_in->numberofsegments; ++i) {
            m_in->segmentlist[i * 2] = m_segments[i].p1;
            m_in->segmentlist[i * 2 + 1] = m_segments[i].p2;
            m_in->segmentmarkerlist[i] = m_segments[i].marker;
        }
    }

    if (!m_holes.empty()) {
        m_in->numberofholes = m_holes.size();
        m_in->holelist = (REAL*)malloc(m_in->numberofholes * 2 * sizeof(REAL));
        for (int i = 0; i < m_in->numberofholes; ++i) {
            m_in->holelist[i * 2] = m_holes[i].first;
            m_in->holelist[i * 2 + 1] = m_holes[i].second;
        }
    }

    if (!m_regions.empty()) {
        m_in->numberofregions = m_regions.size();
        m_in->regionlist = (REAL*)malloc(m_in->numberofregions * 4 * sizeof(REAL));
        for (int i = 0; i < m_in->numberofregions; ++i) {
            m_in->regionlist[i * 4] = m_regions[i][0];
            m_in->regionlist[i * 4 + 1] = m_regions[i][1];
            m_in->regionlist[i * 4 + 2] = m_regions[i][2];
            m_in->regionlist[i * 4 + 3] = m_regions[i][3];
        }
    }

    std::string switches = buildSwitchString(opts);
    
    char* sw = strdup(switches.c_str());
    ::triangulate(sw, m_in, m_out, opts.voronoi ? m_vorout : NULL);
    free(sw);

    // IMPORTANT: Triangle copies the pointers holelist and regionlist from 'in' to 'out'.
    // To avoid double freeing them (since we free m_in right after), we NULL them in 'out'.
    m_out->holelist = NULL;
    m_out->regionlist = NULL;

    // Clean up in for next run
    freeIO(m_in);

    return true;
}

size_t TriangleMesh::numPoints() const { return m_out->numberofpoints; }
TriangleMesh::Point TriangleMesh::getPoint(size_t index) const {
    return Point(m_out->pointlist[index * 2], m_out->pointlist[index * 2 + 1], 
            m_out->pointmarkerlist ? m_out->pointmarkerlist[index] : 0);
}

size_t TriangleMesh::numTriangles() const { return m_out->numberoftriangles; }
void TriangleMesh::getTriangle(size_t index, int& p1, int& p2, int& p3) const {
    p1 = m_out->trianglelist[index * 3];
    p2 = m_out->trianglelist[index * 3 + 1];
    p3 = m_out->trianglelist[index * 3 + 2];
}

size_t TriangleMesh::numEdges() const { return m_out->numberofedges; }
void TriangleMesh::getEdge(size_t index, int& p1, int& p2) const {
    p1 = m_out->edgelist[index * 2];
    p2 = m_out->edgelist[index * 2 + 1];
}

size_t TriangleMesh::numVoronoiEdges() const { return m_vorout->numberofedges; }
void TriangleMesh::getVoronoiEdge(size_t index, int& p1, int& p2) const {
    p1 = m_vorout->edgelist[index * 2];
    p2 = m_vorout->edgelist[index * 2 + 1];
}

void TriangleMesh::smooth(int iterations) {
    if (numPoints() == 0) return;

    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<double> next_x(numPoints(), 0.0);
        std::vector<double> next_y(numPoints(), 0.0);
        std::vector<int> count(numPoints(), 0);

        for (int i = 0; i < numTriangles(); ++i) {
            int p[3];
            getTriangle(i, p[0], p[1], p[2]);
            for (int j = 0; j < 3; ++j) {
                int self = p[j];
                int n1 = p[(j + 1) % 3];
                int n2 = p[(j + 2) % 3];
                
                next_x[self] += (m_out->pointlist[n1 * 2] + m_out->pointlist[n2 * 2]);
                next_y[self] += (m_out->pointlist[n1 * 2 + 1] + m_out->pointlist[n2 * 2 + 1]);
                count[self] += 2;
            }
        }

        for (int i = 0; i < numPoints(); ++i) {
            if (m_out->pointmarkerlist && m_out->pointmarkerlist[i] == 0 && count[i] > 0) {
                m_out->pointlist[i * 2] = next_x[i] / count[i];
                m_out->pointlist[i * 2 + 1] = next_y[i] / count[i];
            }
        }
    }
}

void TriangleMesh::clear() {
    m_points.clear();
    m_segments.clear();
    m_holes.clear();
    m_regions.clear();
    m_targetArea = -1.0;
}

} // namespace triangle
