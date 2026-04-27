#include "MeshOptimizer.hpp"
#include <cmath>
#include <algorithm>
#include <vector>

namespace triangle {

void MeshOptimizer::smooth(std::vector<Point2D>& points, const std::vector<Triangle>& triangles, int iterations, FixedPredicate isFixed) {
    if (points.empty()) return;
    auto fixed = isFixed ? isFixed : [](const Point2D& p) { return p.isFixed || p.marker != 0; };
    
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<double> nx(points.size(), 0), ny(points.size(), 0);
        std::vector<int> count(points.size(), 0);
        for (const auto& t : triangles) {
            for (int j = 0; j < 3; ++j) {
                int s = t.p[j], n1 = t.p[(j+1)%3], n2 = t.p[(j+2)%3];
                nx[s] += points[n1].x + points[n2].x;
                ny[s] += points[n1].y + points[n2].y;
                count[s] += 2;
            }
        }
        for (size_t i = 0; i < points.size(); ++i) {
            if (!fixed(points[i]) && count[i] > 0) {
                points[i].x = nx[i] / count[i];
                points[i].y = ny[i] / count[i];
            }
        }
    }
}

void MeshOptimizer::relaxVoronoi(std::vector<Point2D>& points, const std::vector<Triangle>& triangles, int iterations, FixedPredicate isFixed) {
    if (points.empty() || triangles.empty()) return;
    auto fixed = isFixed ? isFixed : [](const Point2D& p) { return p.isFixed || p.marker != 0; };

    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<std::vector<int>> v2t(points.size());
        for (size_t i = 0; i < triangles.size(); ++i) {
            for (int j=0; j<3; ++j) v2t[triangles[i].p[j]].push_back(i);
        }
        std::vector<Point2D> circumcenters;
        for (const auto& t : triangles) {
            double ax = points[t.p[0]].x, ay = points[t.p[0]].y;
            double bx = points[t.p[1]].x, by = points[t.p[1]].y;
            double cx = points[t.p[2]].x, cy = points[t.p[2]].y;
            double d = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
            if (std::abs(d) < 1e-9) circumcenters.emplace_back((ax+bx+cx)/3, (ay+by+cy)/3);
            else {
                double a2 = ax*ax+ay*ay, b2 = bx*bx+by*by, c2 = cx*cx+cy*cy;
                circumcenters.emplace_back((a2*(by-cy)+b2*(cy-ay)+c2*(ay-by))/d, (a2*(cx-bx)+b2*(ax-cx)+c2*(bx-ax))/d);
            }
        }
        for (size_t i = 0; i < points.size(); ++i) {
            if (fixed(points[i]) || v2t[i].empty()) continue;
            std::vector<std::pair<double, int>> sorted;
            for (int tid : v2t[i]) sorted.push_back({std::atan2(circumcenters[tid].y - points[i].y, circumcenters[tid].x - points[i].x), tid});
            std::sort(sorted.begin(), sorted.end());
            double area = 0, cx = 0, cy = 0;
            for (size_t j = 0; j < sorted.size(); ++j) {
                const auto& p1 = circumcenters[sorted[j].second];
                const auto& p2 = circumcenters[sorted[(j+1)%sorted.size()].second];
                double cross = (p1.x * p2.y - p2.x * p1.y);
                area += cross; cx += (p1.x + p2.x) * cross; cy += (p1.y + p2.y) * cross;
            }
            if (std::abs(area) > 1e-9) { points[i].x = cx / (3.0 * area); points[i].y = cy / (3.0 * area); }
        }
    }
}

void MeshOptimizer::relaxODT(std::vector<Point2D>& points, const std::vector<Triangle>& triangles, int iterations, FixedPredicate isFixed) {
    if (points.empty() || triangles.empty()) return;
    auto fixed = isFixed ? isFixed : [](const Point2D& p) { return p.isFixed || p.marker != 0; };

    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<std::vector<int>> v2t(points.size());
        for (size_t i = 0; i < triangles.size(); ++i) {
            for (int j=0; j<3; ++j) v2t[triangles[i].p[j]].push_back(i);
        }
        std::vector<Point2D> cc;
        std::vector<double> areas;
        for (const auto& t : triangles) {
            double ax = points[t.p[0]].x, ay = points[t.p[0]].y;
            double bx = points[t.p[1]].x, by = points[t.p[1]].y;
            double cx = points[t.p[2]].x, cy = points[t.p[2]].y;
            areas.push_back(0.5 * std::abs(ax*(by-cy)+bx*(cy-ay)+cx*(ay-by)));
            double d = 2.0 * (ax*(by-cy)+bx*(cy-ay)+cx*(ay-by));
            if (std::abs(d) < 1e-9) cc.emplace_back((ax+bx+cx)/3, (ay+by+cy)/3);
            else {
                double a2 = ax*ax+ay*ay, b2 = bx*bx+by*by, c2 = cx*cx+cy*cy;
                cc.emplace_back((a2*(by-cy)+b2*(cy-ay)+c2*(ay-by))/d, (a2*(cx-bx)+b2*(ax-cx)+c2*(bx-ax))/d);
            }
        }
        for (size_t i = 0; i < points.size(); ++i) {
            if (fixed(points[i]) || v2t[i].empty()) continue;
            double sa = 0, scx = 0, scy = 0;
            for (int tid : v2t[i]) { double a = areas[tid]; scx += cc[tid].x * a; scy += cc[tid].y * a; sa += a; }
            if (sa > 1e-9) { points[i].x = scx / sa; points[i].y = scy / sa; }
        }
    }
}

} // namespace triangle
