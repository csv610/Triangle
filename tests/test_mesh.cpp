#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "TriMeshGenerator.hpp"

using namespace triangle;

TEST_CASE("TriangleMesh Basic Triangulation", "[mesh]") {
    TriangleMesh mesh;
    
    SECTION("Empty mesh should return false") {
        Options opts;
        opts.quiet = true;
        REQUIRE_FALSE(mesh.triangulate(opts));
        REQUIRE(mesh.points().empty());
        REQUIRE(mesh.triangles().empty());
    }

    SECTION("Simple triangle (3 points)") {
        mesh.addPoint(0, 0);
        mesh.addPoint(1, 0);
        mesh.addPoint(0, 1);
        
        Options opts;
        opts.quiet = true;
        bool result = mesh.triangulate(opts);
        REQUIRE(result);
        
        REQUIRE(mesh.points().size() == 3);
        REQUIRE(mesh.triangles().size() == 1);
        
        const auto& t = mesh.triangles()[0];
        std::vector<int> indices = {t.p[0], t.p[1], t.p[2]};
        std::sort(indices.begin(), indices.end());
        REQUIRE(indices[0] == 0);
        REQUIRE(indices[1] == 1);
        REQUIRE(indices[2] == 2);
    }

    SECTION("Square with 4 points") {
        mesh.addPoint(0, 0);
        mesh.addPoint(1, 0);
        mesh.addPoint(1, 1);
        mesh.addPoint(0, 1);
        
        Options opts;
        opts.quiet = true;
        REQUIRE(mesh.triangulate(opts));
        
        REQUIRE(mesh.points().size() == 4);
        REQUIRE(mesh.triangles().size() == 2);
    }
}

TEST_CASE("TriangleMesh Helpers", "[mesh]") {
    TriangleMesh mesh;

    SECTION("Bounding Box") {
        mesh.addBoundingBox(0, 0, 10, 10);
        REQUIRE(mesh.numInputPoints() == 4);
        REQUIRE(mesh.numInputSegments() == 4);
        
        Options opts;
        opts.quiet = true;
        REQUIRE(mesh.triangulate(opts));
        REQUIRE(mesh.points().size() >= 4);
        REQUIRE(mesh.triangles().size() >= 2);
    }

    SECTION("Circle") {
        mesh.addCircle(0, 0, 1.0, 8);
        REQUIRE(mesh.numInputPoints() == 8);
        REQUIRE(mesh.numInputSegments() == 8);
        
        Options opts;
        opts.quiet = true;
        REQUIRE(mesh.triangulate(opts));
        REQUIRE(mesh.points().size() >= 8);

        // Verify all output points are on the boundary
        // For a circle with no interior points, every point is a boundary point
        for (const auto& p : mesh.points()) {
            CHECK(p.marker != 0);
            double dist = std::sqrt(p.x * p.x + p.y * p.y);
            CHECK_THAT(dist, Catch::Matchers::WithinAbs(1.0, 1e-7));
        }
    }

    SECTION("Square Boundary Points") {
        // Test that a square with interior points only has 4 boundary points
        mesh.addPoint(0, 0, 1);
        mesh.addPoint(10, 0, 1);
        mesh.addPoint(10, 10, 1);
        mesh.addPoint(0, 10, 1);
        mesh.addPoint(5, 5, 0); // Interior
        
        Options opts;
        opts.quiet = true;
        REQUIRE(mesh.triangulate(opts));
        
        int boundaryCount = 0;
        for (const auto& p : mesh.points()) {
            if (p.marker != 0) boundaryCount++;
        }
        CHECK(boundaryCount == 4);
    }

    SECTION("Hole Handling") {
        // Outer square
        mesh.addBoundingBox(0, 0, 10, 10);
        // Inner square (hole)
        mesh.addBoundingBox(3, 3, 7, 7, 2);
        mesh.addHole(5, 5);

        Options opts;
        opts.quiet = true;
        REQUIRE(mesh.triangulate(opts));

        // Ensure no points were generated strictly inside the hole
        // (Points on the boundary of the hole are fine, so we check > 3.1 and < 6.9)
        for (const auto& p : mesh.points()) {
            bool insideHole = (p.x > 3.01 && p.x < 6.99 && p.y > 3.01 && p.y < 6.99);
            CHECK_FALSE(insideHole);
        }
    }

    SECTION("Max Area Constraint") {
        mesh.addBoundingBox(0, 0, 10, 10); // Area = 100
        
        Options opts;
        opts.quiet = true;
        opts.maxArea = 1.0; // Force subdivision
        REQUIRE(mesh.triangulate(opts));

        // An area of 100 with max element area 1.0 should produce at least 100 triangles
        REQUIRE(mesh.triangles().size() >= 100);
        
        // Verify area of the first few triangles
        const auto& pts = mesh.points();
        for (size_t i = 0; i < std::min(mesh.triangles().size(), size_t(10)); ++i) {
            const auto& t = mesh.triangles()[i];
            double ax = pts[t.p[0]].x, ay = pts[t.p[0]].y;
            double bx = pts[t.p[1]].x, by = pts[t.p[1]].y;
            double cx = pts[t.p[2]].x, cy = pts[t.p[2]].y;
            double area = 0.5 * std::abs(ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
            CHECK(area <= 1.0001);
        }
    }

    SECTION("Minimum Angle Constraint") {
        mesh.addBoundingBox(0, 0, 10, 10);
        mesh.addPoint(5, 0.1); // Creates a very skinny triangle near the bottom edge
        
        Options opts;
        opts.quiet = true;
        opts.quality = true;
        opts.minAngle = 30.0; // Enforce high quality
        REQUIRE(mesh.triangulate(opts));

        const auto& pts = mesh.points();
        for (const auto& t : mesh.triangles()) {
            double coords[3][2] = {
                {pts[t.p[0]].x, pts[t.p[0]].y},
                {pts[t.p[1]].x, pts[t.p[1]].y},
                {pts[t.p[2]].x, pts[t.p[2]].y}
            };
            
            for (int i = 0; i < 3; ++i) {
                double x1 = coords[i][0], y1 = coords[i][1];
                double x2 = coords[(i+1)%3][0], y2 = coords[(i+1)%3][1];
                double x3 = coords[(i+2)%3][0], y3 = coords[(i+2)%3][1];
                
                double v1x = x2 - x1, v1y = y2 - y1;
                double v2x = x3 - x1, v2y = y3 - y1;
                
                double dot = v1x * v2x + v1y * v2y;
                double mag1 = std::sqrt(v1x*v1x + v1y*v1y);
                double mag2 = std::sqrt(v2x*v2x + v2y*v2y);
                double angle = std::acos(dot / (mag1 * mag2)) * 180.0 / M_PI;
                
                CHECK(angle >= 29.0); // Allow slight tolerance for edge cases
            }
        }
    }

    SECTION("Arbitrary Non-Convex Polygon") {
        // Create an L-shaped polygon
        std::vector<Point2D> lShape = {
            {0, 0}, {5, 0}, {5, 2}, {2, 2}, {2, 5}, {0, 5}
        };
        mesh.addPolygon(lShape, true);
        
        Options opts;
        opts.quiet = true;
        REQUIRE(mesh.triangulate(opts));
        
        // Ensure no triangles were generated outside the L-shape 
        // (e.g., in the concave "corner" at 4, 4)
        for (const auto& t : mesh.triangles()) {
            const auto& pts = mesh.points();
            double cx = (pts[t.p[0]].x + pts[t.p[1]].x + pts[t.p[2]].x) / 3.0;
            double cy = (pts[t.p[0]].y + pts[t.p[1]].y + pts[t.p[2]].y) / 3.0;
            
            bool insideL = (cx >= 0 && cx <= 5 && cy >= 0 && cy <= 2) ||
                           (cx >= 0 && cx <= 2 && cy >= 2 && cy <= 5);
            CHECK(insideL);
        }
    }

    SECTION("Conforming Delaunay") {
        mesh.addBoundingBox(0, 0, 10, 10);
        mesh.addSegment(0, 2); // Diagonal segment
        
        Options opts;
        opts.quiet = true;
        opts.conforming = true; // Use -D switch
        REQUIRE(mesh.triangulate(opts));
        
        // Conforming Delaunay might add Steiner points on segments to ensure 
        // all triangles are Delaunay. 
        // We just verify it runs and produces a valid mesh.
        REQUIRE(mesh.points().size() >= 4);
        REQUIRE(mesh.triangles().size() >= 2);
    }
}

TEST_CASE("TriangleMesh Advanced", "[mesh]") {
    TriangleMesh mesh;

    SECTION("CVT Generation") {
        mesh.addBoundingBox(0, 0, 10, 10);
        mesh.generateCVT(100, 5);
        
        REQUIRE(mesh.points().size() >= 100);
        REQUIRE(!mesh.triangles().empty());
    }

    SECTION("Voronoi Diagram") {
        mesh.addPoint(0, 0);
        mesh.addPoint(1, 0);
        mesh.addPoint(0, 1);
        mesh.addPoint(1, 1);
        
        Options opts;
        opts.voronoi = true;
        opts.quiet = true;
        REQUIRE(mesh.triangulate(opts));
        
        REQUIRE(!mesh.voronoi().empty());
        REQUIRE(!mesh.voroFaces().empty());
    }
}

TEST_CASE("TriangleMesh Utils", "[mesh]") {
    TriangleMesh mesh;

    SECTION("refineSegments") {
        mesh.addPoint(0, 0);
        mesh.addPoint(10, 0);
        mesh.addSegment(0, 1);
        
        mesh.refineSegments(1.0);
        REQUIRE(mesh.numInputPoints() == 11);
        REQUIRE(mesh.numInputSegments() == 10);
    }

    SECTION("resolveIntersections") {
        mesh.addPoint(0, 0);
        mesh.addPoint(10, 10);
        mesh.addSegment(0, 1);

        mesh.addPoint(10, 0);
        mesh.addPoint(0, 10);
        mesh.addSegment(2, 3);
        
        Options opts;
        opts.quiet = true;
        REQUIRE(mesh.triangulate(opts));
        
        // Should find intersection at (5, 5) and split segments
        REQUIRE(mesh.points().size() >= 5);
        bool foundCenter = false;
        for (const auto& p : mesh.points()) {
            if (std::abs(p.x - 5.0) < 1e-7 && std::abs(p.y - 5.0) < 1e-7) {
                foundCenter = true;
                break;
            }
        }
        REQUIRE(foundCenter);
    }
}

TEST_CASE("TriangleMesh Lifecycle", "[mesh]") {
    TriangleMesh mesh;

    SECTION("Clear resets state") {
        mesh.addBoundingBox(0, 0, 10, 10);
        Options opts;
        opts.quiet = true;
        mesh.triangulate(opts);
        
        REQUIRE(mesh.points().size() > 0);
        REQUIRE(mesh.numInputPoints() == 4);

        mesh.clear();
        
        REQUIRE(mesh.points().size() == 0);
        REQUIRE(mesh.triangles().size() == 0);
        REQUIRE(mesh.numInputPoints() == 0);
        REQUIRE(mesh.numInputSegments() == 0);
    }
}

TEST_CASE("MeshOptimizer", "[optimizer]") {
    TriangleMesh mesh;
    mesh.addBoundingBox(0, 0, 10, 10);
    // Add a point that is clearly not in the center to ensure it moves
    mesh.addPoint(1, 1);
    
    Options opts;
    opts.quiet = true;
    mesh.triangulate(opts);
    
    SECTION("Smooth") {
        auto points = mesh.points();
        auto originalPoints = points;
        auto triangles = mesh.triangles();
        
        MeshOptimizer::smooth(points, triangles, 10);
        
        REQUIRE(points.size() == originalPoints.size());
        
        // At least the point at (1, 1) should have moved
        bool anyMoved = false;
        for (size_t i = 0; i < points.size(); ++i) {
            if (std::abs(points[i].x - originalPoints[i].x) > 1e-5 ||
                std::abs(points[i].y - originalPoints[i].y) > 1e-5) {
                anyMoved = true;
                break;
            }
        }
        CHECK(anyMoved);
    }

    SECTION("Relax Voronoi") {
        auto points = mesh.points();
        auto originalPoints = points;
        auto triangles = mesh.triangles();
        
        MeshOptimizer::relaxVoronoi(points, triangles, 10);
        
        REQUIRE(points.size() == originalPoints.size());
        
        bool anyMoved = false;
        for (size_t i = 0; i < points.size(); ++i) {
            if (std::abs(points[i].x - originalPoints[i].x) > 1e-5 ||
                std::abs(points[i].y - originalPoints[i].y) > 1e-5) {
                anyMoved = true;
                break;
            }
        }
        CHECK(anyMoved);
    }

    SECTION("Relax ODT") {
        auto points = mesh.points();
        auto originalPoints = points;
        auto triangles = mesh.triangles();
        
        MeshOptimizer::relaxODT(points, triangles, 10);
        
        REQUIRE(points.size() == originalPoints.size());
        
        bool anyMoved = false;
        for (size_t i = 0; i < points.size(); ++i) {
            if (std::abs(points[i].x - originalPoints[i].x) > 1e-5 ||
                std::abs(points[i].y - originalPoints[i].y) > 1e-5) {
                anyMoved = true;
                break;
            }
        }
        CHECK(anyMoved);
    }

    SECTION("Fixed points stay put") {
        TriangleMesh meshFixed;
        meshFixed.addBoundingBox(0, 0, 10, 10);
        // Add a fixed point and a non-fixed point
        meshFixed.addPoint(1, 1, 0, true);  // isFixed = true
        meshFixed.addPoint(2, 2, 0, false); // isFixed = false
        
        Options opts;
        opts.quiet = true;
        meshFixed.triangulate(opts);
        
        auto points = meshFixed.points();
        auto originalPoints = points;
        auto triangles = meshFixed.triangles();
        
        // Find the index of the fixed point (1,1) in the output
        int fixedIdx = -1;
        int nonFixedIdx = -1;
        for (size_t i = 0; i < points.size(); ++i) {
            if (std::abs(points[i].x - 1.0) < 1e-7 && std::abs(points[i].y - 1.0) < 1e-7) {
                if (points[i].isFixed) fixedIdx = i;
            }
            if (std::abs(points[i].x - 2.0) < 1e-7 && std::abs(points[i].y - 2.0) < 1e-7) {
                if (!points[i].isFixed) nonFixedIdx = i;
            }
        }
        
        REQUIRE(fixedIdx != -1);
        REQUIRE(nonFixedIdx != -1);
        
        MeshOptimizer::smooth(points, triangles, 10);
        
        // Fixed point should NOT have moved
        CHECK(std::abs(points[fixedIdx].x - 1.0) < 1e-9);
        CHECK(std::abs(points[fixedIdx].y - 1.0) < 1e-9);
        
        // Non-fixed point SHOULD have moved (or at least it's allowed to)
        // Given the symmetry of the box, it should move towards the center
        bool nonFixedMoved = (std::abs(points[nonFixedIdx].x - 2.0) > 1e-5 ||
                             std::abs(points[nonFixedIdx].y - 2.0) > 1e-5);
        CHECK(nonFixedMoved);
    }
}
