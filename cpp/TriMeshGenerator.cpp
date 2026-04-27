#include "TriMeshGenerator.hpp"
#include "MeshOptimizer.hpp"
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

void TriangleMesh::addPoint(double x, double y, int marker, bool isFixed) {
    m_inPoints.emplace_back(x, y, marker, isFixed);
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

void TriangleMesh::refineSegments(double maxLength) {
    if (maxLength <= 0) return;
    std::vector<Edge> nextSegments;
    for (const auto& s : m_inSegments) {
        Point2D p1 = m_inPoints[s.p[0]];
        Point2D p2 = m_inPoints[s.p[1]];
        double dx = p2.x - p1.x;
        double dy = p2.y - p1.y;
        double len = std::sqrt(dx*dx + dy*dy);
        
        if (len > maxLength) {
            int numSubSegments = std::ceil(len / maxLength);
            int lastIdx = s.p[0];
            for (int i = 1; i < numSubSegments; ++i) {
                double t = (double)i / numSubSegments;
                int newIdx = (int)m_inPoints.size();
                m_inPoints.emplace_back(p1.x + t*dx, p1.y + t*dy, s.marker);
                nextSegments.emplace_back(lastIdx, newIdx, s.marker);
                lastIdx = newIdx;
            }
            nextSegments.emplace_back(lastIdx, s.p[1], s.marker);
        } else {
            nextSegments.push_back(s);
        }
    }
    m_inSegments = nextSegments;
}

bool TriangleMesh::triangulate(const Options& opts) {
    if (m_inPoints.size() < 3) {
        // Clear previous output so we don't render stale data
        m_outPoints.clear();
        m_outTriangles.clear();
        m_outEdges.clear();
        m_outSegments.clear();
        m_outVoronoi.clear();
        m_outVoroFaces.clear();
        return false;
    }
    resolveIntersections();
    
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
    if (pImpl->out.pointlist == NULL && pImpl->out.numberofpoints > 0) {
        return;
    }
    for (int i = 0; i < pImpl->out.numberofpoints; ++i) {
        bool fixed = (i < (int)m_inPoints.size()) ? m_inPoints[i].isFixed : false;
        m_outPoints.emplace_back(pImpl->out.pointlist[i * 2], pImpl->out.pointlist[i * 2 + 1], 
                               pImpl->out.pointmarkerlist ? pImpl->out.pointmarkerlist[i] : 0,
                               fixed);
    }

    m_outTriangles.clear();
    if (pImpl->out.trianglelist != NULL) {
        for (int i = 0; i < pImpl->out.numberoftriangles; ++i) {
            m_outTriangles.emplace_back(pImpl->out.trianglelist[i * 3], 
                                      pImpl->out.trianglelist[i * 3 + 1], 
                                      pImpl->out.trianglelist[i * 3 + 2]);
        }
    }

    m_outEdges.clear();
    if (pImpl->out.edgelist != NULL) {
        for (int i = 0; i < pImpl->out.numberofedges; ++i) {
            m_outEdges.emplace_back(pImpl->out.edgelist[i * 2], pImpl->out.edgelist[i * 2 + 1], 
                                  pImpl->out.edgemarkerlist ? pImpl->out.edgemarkerlist[i] : 0);
        }
    }

    m_outSegments.clear();
    if (pImpl->out.segmentlist != NULL) {
        for (int i = 0; i < pImpl->out.numberofsegments; ++i) {
            m_outSegments.emplace_back(pImpl->out.segmentlist[i * 2], pImpl->out.segmentlist[i * 2 + 1], 
                                     pImpl->out.segmentmarkerlist ? pImpl->out.segmentmarkerlist[i] : 0);
        }
    }

    m_outVoronoi.clear();
    if (pImpl->vorout.edgelist != NULL) {
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

void TriangleMesh::generateCVT(int numPoints, int iterations) {
    if (m_inPoints.empty()) addBoundingBox(0, 0, 1, 1);
    double minX = 1e30, minY = 1e30, maxX = -1e30, maxY = -1e30;
    for (const auto& p : m_inPoints) { minX = std::min(minX, p.x); minY = std::min(minY, p.y); maxX = std::max(maxX, p.x); maxY = std::max(maxY, p.y); }
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dx(minX, maxX), dy(minY, maxY);
    for (int i = 0; i < numPoints; ++i) addPoint(dx(rng), dy(rng), 0);
    Options opts; opts.quiet = true; opts.quality = false;
    for (int i = 0; i < iterations; ++i) {
        triangulate(opts); MeshOptimizer::relaxVoronoi(m_outPoints, m_outTriangles, 1);
        std::vector<Point2D> next;
        for (const auto& p : m_inPoints) if (p.isFixed || p.marker != 0) next.push_back(p);
        for (const auto& p : m_outPoints) if (!p.isFixed && p.marker == 0) next.push_back(p);
        m_inPoints = next;
    }
    opts.quality = true; triangulate(opts);
}


void TriangleMesh::clear() {
    m_inPoints.clear(); m_inSegments.clear(); m_inHoles.clear();
    m_outPoints.clear(); m_outTriangles.clear(); m_outEdges.clear(); m_outSegments.clear(); m_outVoronoi.clear(); m_outVoroFaces.clear();
}

void TriangleMesh::resolveIntersections() {
    if (m_inSegments.empty()) return;

    bool foundIntersection = true;
    while (foundIntersection) {
        foundIntersection = false;
        for (size_t i = 0; i < m_inSegments.size(); ++i) {
            for (size_t j = i + 1; j < m_inSegments.size(); ++j) {
                // Get points for segment i
                const auto& s1 = m_inSegments[i];
                const auto& s2 = m_inSegments[j];
                
                Point2D a = m_inPoints[s1.p[0]];
                Point2D b = m_inPoints[s1.p[1]];
                Point2D c = m_inPoints[s2.p[0]];
                Point2D d = m_inPoints[s2.p[1]];

                // Line-line intersection formula
                double det = (b.x - a.x) * (d.y - c.y) - (b.y - a.y) * (d.x - c.x);
                if (std::abs(det) < 1e-10) continue; // Parallel or nearly parallel

                double t = ((c.x - a.x) * (d.y - c.y) - (c.y - a.y) * (d.x - c.x)) / det;
                double u = ((c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x)) / det;

                // Check if intersection is strictly inside both segments
                const double eps = 1e-7;
                if (t > eps && t < 1.0 - eps && u > eps && u < 1.0 - eps) {
                    Point2D intersect(a.x + t * (b.x - a.x), a.y + t * (b.y - a.y), s1.marker);
                    
                    int newIdx = (int)m_inPoints.size();
                    m_inPoints.push_back(intersect);

                    Edge e1 = m_inSegments[i];
                    Edge e2 = m_inSegments[j];

                    // Split segment i into (p0, new) and (new, p1)
                    m_inSegments[i].p[1] = newIdx;
                    m_inSegments.emplace_back(newIdx, e1.p[1], e1.marker);

                    // Split segment j into (p0, new) and (new, p1)
                    m_inSegments[j].p[1] = newIdx;
                    m_inSegments.emplace_back(newIdx, e2.p[1], e2.marker);

                    foundIntersection = true;
                    break; // Restart search as indices might have changed or new intersections created
                }
            }
            if (foundIntersection) break;
        }
    }
}

} // namespace triangle
