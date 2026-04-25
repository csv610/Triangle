#include "Triangle.hpp"
#include <iostream>
#include <sstream>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <random>

#ifndef REAL
#define REAL double
#endif
#ifndef VOID
#define VOID void
#endif
#ifndef ANSI_DECLARATORS
#define ANSI_DECLARATORS
#endif

extern "C" {
    #include "triangle.h"
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace triangle {

/**
 * @brief Internal implementation structure to hide legacy C types.
 */
struct TriangleMesh::Impl {
    struct triangulateio in, out, vorout;

    Impl() {
        std::memset(&in, 0, sizeof(in));
        std::memset(&out, 0, sizeof(out));
        std::memset(&vorout, 0, sizeof(vorout));
    }

    void freeIO(struct triangulateio* io, bool sharedHoles = false) {
        if (io->pointlist) { trifree(io->pointlist); io->pointlist = NULL; }
        if (io->pointmarkerlist) { trifree(io->pointmarkerlist); io->pointmarkerlist = NULL; }
        if (io->trianglelist) { trifree(io->trianglelist); io->trianglelist = NULL; }
        if (io->segmentlist) { trifree(io->segmentlist); io->segmentlist = NULL; }
        if (io->segmentmarkerlist) { trifree(io->segmentmarkerlist); io->segmentmarkerlist = NULL; }
        if (io->edgelist) { trifree(io->edgelist); io->edgelist = NULL; }
        if (io->edgemarkerlist) { trifree(io->edgemarkerlist); io->edgemarkerlist = NULL; }
        if (io->normlist) { trifree(io->normlist); io->normlist = NULL; }
        
        if (!sharedHoles) {
            if (io->holelist) { trifree(io->holelist); io->holelist = NULL; }
        }
    }

    ~Impl() {
        freeIO(&in);
        freeIO(&out);
        freeIO(&vorout);
    }
};

TriangleMesh::TriangleMesh() : pImpl(new Impl()) {}
TriangleMesh::~TriangleMesh() { delete pImpl; }

void TriangleMesh::addPoint(double x, double y, int marker) {
    m_inPoints.emplace_back(x, y, marker);
}

void TriangleMesh::addSegment(int p1, int p2, int marker) {
    m_inSegments.emplace_back(p1, p2, marker);
}

void TriangleMesh::addHole(double x, double y) {
    m_inHoles.emplace_back(x, y, 0);
}

void TriangleMesh::addPolygon(const std::vector<Point2D>& points, bool closed) {
    int start = m_inPoints.size();
    for (const auto& p : points) addPoint(p);
    for (size_t i = 0; i < points.size() - 1; ++i) addSegment(start + i, start + i + 1, points[i].marker);
    if (closed && points.size() > 2) addSegment(start + points.size() - 1, start, points.back().marker);
}

void TriangleMesh::addBoundingBox(double x1, double y1, double x2, double y2, int marker) {
    std::vector<Point2D> box = {{x1, y1, marker}, {x2, y1, marker}, {x2, y2, marker}, {x1, y2, marker}};
    addPolygon(box, true);
}

void TriangleMesh::addCircle(double cx, double cy, double radius, int segments, int marker) {
    std::vector<Point2D> circle;
    for (int i = 0; i < segments; ++i) {
        double a = 2.0 * M_PI * i / segments;
        circle.emplace_back(cx + radius * std::cos(a), cy + radius * std::sin(a), marker);
    }
    addPolygon(circle, true);
}

bool TriangleMesh::triangulate(const Options& opts) {
    pImpl->freeIO(&pImpl->out, true);
    pImpl->freeIO(&pImpl->vorout, true);

    // Prepare In
    pImpl->in.numberofpoints = m_inPoints.size();
    pImpl->in.pointlist = (REAL*)malloc(pImpl->in.numberofpoints * 2 * sizeof(REAL));
    pImpl->in.pointmarkerlist = (int*)malloc(pImpl->in.numberofpoints * sizeof(int));
    for (int i = 0; i < pImpl->in.numberofpoints; ++i) {
        pImpl->in.pointlist[i * 2] = m_inPoints[i].x;
        pImpl->in.pointlist[i * 2 + 1] = m_inPoints[i].y;
        pImpl->in.pointmarkerlist[i] = m_inPoints[i].marker;
    }

    if (!m_inSegments.empty()) {
        pImpl->in.numberofsegments = m_inSegments.size();
        pImpl->in.segmentlist = (int*)malloc(pImpl->in.numberofsegments * 2 * sizeof(int));
        pImpl->in.segmentmarkerlist = (int*)malloc(pImpl->in.numberofsegments * sizeof(int));
        for (int i = 0; i < pImpl->in.numberofsegments; ++i) {
            pImpl->in.segmentlist[i * 2] = m_inSegments[i].p[0];
            pImpl->in.segmentlist[i * 2 + 1] = m_inSegments[i].p[1];
            pImpl->in.segmentmarkerlist[i] = m_inSegments[i].marker;
        }
    }

    if (!m_inHoles.empty()) {
        pImpl->in.numberofholes = m_inHoles.size();
        pImpl->in.holelist = (REAL*)malloc(pImpl->in.numberofholes * 2 * sizeof(REAL));
        for (int i = 0; i < pImpl->in.numberofholes; ++i) {
            pImpl->in.holelist[i * 2] = m_inHoles[i].x;
            pImpl->in.holelist[i * 2 + 1] = m_inHoles[i].y;
        }
    }

    std::stringstream ss;
    if (!m_inSegments.empty()) ss << "p";
    if (opts.quality) { ss << "q" << opts.minAngle; }
    if (opts.maxArea > 0) ss << "a" << opts.maxArea;
    if (opts.voronoi) ss << "v";
    if (opts.conforming) ss << "D";
    if (opts.quiet) ss << "Q";
    ss << "ez"; // Always edges and zero-based
    ss << opts.extraSwitches;

    char* sw = strdup(ss.str().c_str());
    ::triangulate(sw, &pImpl->in, &pImpl->out, opts.voronoi ? &pImpl->vorout : NULL);
    free(sw);

    pImpl->out.holelist = NULL; // Shared pointer protection
    pImpl->freeIO(&pImpl->in);
    
    syncOutput();
    return true;
}

void TriangleMesh::syncOutput() {
    m_outPoints.clear();
    for (int i = 0; i < pImpl->out.numberofpoints; ++i) {
        m_outPoints.emplace_back(pImpl->out.pointlist[i * 2], pImpl->out.pointlist[i * 2 + 1], 
                               pImpl->out.pointmarkerlist ? pImpl->out.pointmarkerlist[i] : 0);
    }

    m_outTriangles.clear();
    for (int i = 0; i < pImpl->out.numberoftriangles; ++i) {
        m_outTriangles.emplace_back(pImpl->out.trianglelist[i * 3], 
                                  pImpl->out.trianglelist[i * 3 + 1], 
                                  pImpl->out.trianglelist[i * 3 + 2]);
    }

    m_outEdges.clear();
    for (int i = 0; i < pImpl->out.numberofedges; ++i) {
        m_outEdges.emplace_back(pImpl->out.edgelist[i * 2], pImpl->out.edgelist[i * 2 + 1], 
                              pImpl->out.edgemarkerlist ? pImpl->out.edgemarkerlist[i] : 0);
    }

    m_outVoronoi.clear();
    if (pImpl->vorout.edgelist) {
        for (int i = 0; i < pImpl->vorout.numberofedges; ++i) {
            m_outVoronoi.emplace_back(pImpl->vorout.edgelist[i * 2], pImpl->vorout.edgelist[i * 2 + 1]);
        }
    }

    // --- Voronoi Face Reconstruction ---
    m_outVoroFaces.clear();
    if (m_outPoints.empty() || m_outTriangles.empty()) return;

    // 1. Map vertices to incident triangles
    std::vector<std::vector<int>> v2t(m_outPoints.size());
    for (size_t i = 0; i < m_outTriangles.size(); ++i) {
        for (int j = 0; j < 3; ++j) v2t[m_outTriangles[i].p[j]].push_back(i);
    }

    // 2. Precompute all triangle circumcenters (these are the Voronoi vertices)
    std::vector<Point2D> circumcenters;
    for (const auto& t : m_outTriangles) {
        double ax = m_outPoints[t.p[0]].x, ay = m_outPoints[t.p[0]].y;
        double bx = m_outPoints[t.p[1]].x, by = m_outPoints[t.p[1]].y;
        double cx = m_outPoints[t.p[2]].x, cy = m_outPoints[t.p[2]].y;
        double d = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
        if (std::abs(d) < 1e-9) circumcenters.emplace_back((ax+bx+cx)/3, (ay+by+cy)/3);
        else {
            double a2 = ax*ax+ay*ay, b2 = bx*bx+by*by, c2 = cx*cx+cy*cy;
            circumcenters.emplace_back((a2*(by-cy)+b2*(cy-ay)+c2*(ay-by))/d, (a2*(cx-bx)+b2*(ax-cx)+c2*(bx-ax))/d);
        }
    }

    // 3. For each point, build its Voronoi face
    for (size_t i = 0; i < m_outPoints.size(); ++i) {
        const auto& tris = v2t[i];
        if (tris.empty()) continue;

        VoroFace face;
        face.siteIndex = i;
        face.isClosed = (m_outPoints[i].marker == 0);

        // Sort triangles around point i to get ordered Voronoi vertices
        std::vector<std::pair<double, int>> sorted;
        for (int tid : tris) {
            double angle = std::atan2(circumcenters[tid].y - m_outPoints[i].y, 
                                     circumcenters[tid].x - m_outPoints[i].x);
            sorted.push_back({angle, tid});
        }
        std::sort(sorted.begin(), sorted.end());

        for (const auto& s : sorted) {
            face.vertices.push_back(circumcenters[s.second]);
        }
        
        m_outVoroFaces.push_back(face);
    }
}

void TriangleMesh::smooth(int iterations) {
    if (m_outPoints.empty()) return;
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<double> nx(m_outPoints.size(), 0), ny(m_outPoints.size(), 0);
        std::vector<int> count(m_outPoints.size(), 0);
        for (const auto& t : m_outTriangles) {
            for (int j = 0; j < 3; ++j) {
                int s = t.p[j], n1 = t.p[(j+1)%3], n2 = t.p[(j+2)%3];
                nx[s] += m_outPoints[n1].x + m_outPoints[n2].x;
                ny[s] += m_outPoints[n1].y + m_outPoints[n2].y;
                count[s] += 2;
            }
        }
        for (size_t i = 0; i < m_outPoints.size(); ++i) {
            if (m_outPoints[i].marker == 0 && count[i] > 0) {
                m_outPoints[i].x = nx[i] / count[i];
                m_outPoints[i].y = ny[i] / count[i];
            }
        }
    }
}

void TriangleMesh::relaxVoronoi(int iterations) {
    if (m_outPoints.empty() || m_outTriangles.empty()) return;
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<std::vector<int>> v2t(m_outPoints.size());
        for (size_t i = 0; i < m_outTriangles.size(); ++i) {
            for (int j=0; j<3; ++j) v2t[m_outTriangles[i].p[j]].push_back(i);
        }
        std::vector<Point2D> circumcenters;
        for (const auto& t : m_outTriangles) {
            double ax = m_outPoints[t.p[0]].x, ay = m_outPoints[t.p[0]].y;
            double bx = m_outPoints[t.p[1]].x, by = m_outPoints[t.p[1]].y;
            double cx = m_outPoints[t.p[2]].x, cy = m_outPoints[t.p[2]].y;
            double d = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
            if (std::abs(d) < 1e-9) circumcenters.emplace_back((ax+bx+cx)/3, (ay+by+cy)/3);
            else {
                double a2 = ax*ax+ay*ay, b2 = bx*bx+by*by, c2 = cx*cx+cy*cy;
                circumcenters.emplace_back((a2*(by-cy)+b2*(cy-ay)+c2*(ay-by))/d, (a2*(cx-bx)+b2*(ax-cx)+c2*(bx-ax))/d);
            }
        }
        for (size_t i = 0; i < m_outPoints.size(); ++i) {
            if (m_outPoints[i].marker != 0 || v2t[i].empty()) continue;
            std::vector<std::pair<double, int>> sorted;
            for (int tid : v2t[i]) sorted.push_back({std::atan2(circumcenters[tid].y - m_outPoints[i].y, circumcenters[tid].x - m_outPoints[i].x), tid});
            std::sort(sorted.begin(), sorted.end());
            double area = 0, cx = 0, cy = 0;
            for (size_t j = 0; j < sorted.size(); ++j) {
                const auto& p1 = circumcenters[sorted[j].second];
                const auto& p2 = circumcenters[sorted[(j+1)%sorted.size()].second];
                double cross = (p1.x * p2.y - p2.x * p1.y);
                area += cross; cx += (p1.x + p2.x) * cross; cy += (p1.y + p2.y) * cross;
            }
            if (std::abs(area) > 1e-9) { m_outPoints[i].x = cx / (3.0 * area); m_outPoints[i].y = cy / (3.0 * area); }
        }
    }
}

void TriangleMesh::relaxODT(int iterations) {
    if (m_outPoints.empty() || m_outTriangles.empty()) return;
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<std::vector<int>> v2t(m_outPoints.size());
        for (size_t i = 0; i < m_outTriangles.size(); ++i) {
            for (int j=0; j<3; ++j) v2t[m_outTriangles[i].p[j]].push_back(i);
        }
        std::vector<Point2D> cc;
        std::vector<double> areas;
        for (const auto& t : m_outTriangles) {
            double ax = m_outPoints[t.p[0]].x, ay = m_outPoints[t.p[0]].y;
            double bx = m_outPoints[t.p[1]].x, by = m_outPoints[t.p[1]].y;
            double cx = m_outPoints[t.p[2]].x, cy = m_outPoints[t.p[2]].y;
            areas.push_back(0.5 * std::abs(ax*(by-cy)+bx*(cy-ay)+cx*(ay-by)));
            double d = 2.0 * (ax*(by-cy)+bx*(cy-ay)+cx*(ay-by));
            if (std::abs(d) < 1e-9) cc.emplace_back((ax+bx+cx)/3, (ay+by+cy)/3);
            else {
                double a2 = ax*ax+ay*ay, b2 = bx*bx+by*by, c2 = cx*cx+cy*cy;
                cc.emplace_back((a2*(by-cy)+b2*(cy-ay)+c2*(ay-by))/d, (a2*(cx-bx)+b2*(ax-cx)+c2*(bx-ax))/d);
            }
        }
        for (size_t i = 0; i < m_outPoints.size(); ++i) {
            if (m_outPoints[i].marker != 0 || v2t[i].empty()) continue;
            double sa = 0, scx = 0, scy = 0;
            for (int tid : v2t[i]) { double a = areas[tid]; scx += cc[tid].x * a; scy += cc[tid].y * a; sa += a; }
            if (sa > 1e-9) { m_outPoints[i].x = scx / sa; m_outPoints[i].y = scy / sa; }
        }
    }
}

void TriangleMesh::generateCVT(int numPoints, int iterations) {
    if (m_inPoints.empty()) addBoundingBox(0, 0, 1, 1);
    double minX = 1e30, minY = 1e30, maxX = -1e30, maxY = -1e30;
    for (const auto& p : m_inPoints) { minX = std::min(minX, p.x); minY = std::min(minY, p.y); maxX = std::max(maxX, p.x); maxY = std::max(maxY, p.y); }
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dx(minX, maxX), dy(minY, maxY);
    for (int i = 0; i < numPoints; ++i) addPoint(dx(rng), dy(rng), 0);
    Options opts; opts.quiet = true; opts.quality = false;
    for (int i = 0; i < iterations; ++i) {
        triangulate(opts); relaxVoronoi(1);
        std::vector<Point2D> next;
        for (const auto& p : m_inPoints) if (p.marker != 0) next.push_back(p);
        for (const auto& p : m_outPoints) if (p.marker == 0) next.push_back(p);
        m_inPoints = next;
    }
    opts.quality = true; triangulate(opts);
}

void TriangleMesh::clear() {
    m_inPoints.clear(); m_inSegments.clear(); m_inHoles.clear();
    m_outPoints.clear(); m_outTriangles.clear(); m_outEdges.clear(); m_outVoronoi.clear(); m_outVoroFaces.clear();
}

} // namespace triangle
