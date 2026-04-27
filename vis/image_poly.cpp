#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <string>
#include <map>
#include <list>

struct Point {
    double x, y;
};

struct Segment {
    int p1, p2;
};

enum Mode { MODE_NONE, MODE_MOVE, MODE_ADD, MODE_DELETE };

class ImagePolyGL {
public:
    cv::Mat original;
    GLuint textureID = 0;
    int imgW = 0, imgH = 0;

    std::vector<Point> points;
    std::vector<Segment> segments;
    
    int cannyThreshold1 = 100, cannyThreshold2 = 200;
    double epsilon = 0.5;
    float lineWidth = 3.0f;
    float nodeRadius = 4.0f;
    bool onlyLargest = false;
    bool useThinning = true;

    static constexpr double HIT_RADIUS = 10.0;
    static constexpr double SEGMENT_HIT_RADIUS = 5.0;
    static constexpr double SPUR_LENGTH_THRESHOLD = 5.0;
    static constexpr double NODE_DIST_THRESHOLD = 2.5;
    static constexpr double CORNER_SNAP_DIST = 2.5;
    static constexpr double NEIGHBOR_DIST_SQ = 1e-6;
    static constexpr double CLOSE_POLY_DIST = 1.0;
    static constexpr int CIRCLE_SEGMENTS = 32;
    Mode currentMode = MODE_NONE;
    int selectedPt = -1;
    int lastPtIdx = -1;

    double zoom = 1.0;
    double panX = 0, panY = 0;
    bool isPanning = false;
    double lastMouseX = 0, lastMouseY = 0;
    bool showImage = true;
    bool showGrid = false;
    bool showNodeNumbers = false;
    double gridStep = 50.0;

    std::map<int, GLuint> textTextures;
    std::map<int, cv::Size> textSizes;

    void load(const std::string& path) {
        std::cerr << "Loading: " << path << std::endl;
        
        original = cv::imread(path);
        std::cerr << "imread done, size: " << original.size() << std::endl;
        if (original.empty()) {
            std::cerr << "Error: Could not load image at " << path << std::endl;
            return;
        }
        imgW = original.cols;
        imgH = original.rows;
        panX = imgW / 2.0;
        panY = imgH / 2.0;

        if (glfwGetCurrentContext() == nullptr) {
            return;
        }

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        cv::Mat rgb;
        cv::cvtColor(original, rgb, cv::COLOR_BGR2RGB);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imgW, imgH, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data);
    }


    void screenToWorld(GLFWwindow* window, double x, double y, double& wx, double& wy) {
        int w, h; glfwGetWindowSize(window, &w, &h);
        double nx = (2.0 * x / w) - 1.0;
        double ny = 1.0 - (2.0 * y / h);
        double halfW = imgW / 2.0 / zoom;
        double halfH = imgH / 2.0 / zoom;
        wx = nx * halfW + panX;
        wy = ny * halfH + panY;
    }

    void draw(GLFWwindow* window) {
        int w, h; glfwGetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (imgW <= 0 || imgH <= 0) {
            glfwSwapBuffers(window);
            return;
        }

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        
        double winAspect = (double)w / h;
        double imgAspect = (double)imgW / imgH;
        double halfW, halfH;
        
        if (winAspect > imgAspect) {
            halfH = imgH / 2.0 / zoom;
            halfW = halfH * winAspect;
        } else {
            halfW = imgW / 2.0 / zoom;
            halfH = halfW / winAspect;
        }
        
        glOrtho(panX - halfW, panX + halfW, panY - halfH, panY + halfH, -1, 1);
        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        if (showImage && textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor3f(1, 1, 1);
            glBegin(GL_QUADS);
            glTexCoord2f(0, 1); glVertex2f(0, 0);
            glTexCoord2f(1, 1); glVertex2f(imgW, 0);
            glTexCoord2f(1, 0); glVertex2f(imgW, imgH);
            glTexCoord2f(0, 0); glVertex2f(0, imgH);
            glEnd();
            glDisable(GL_TEXTURE_2D);
        }

        drawGrid();

        // Draw Segments
        glLineWidth(lineWidth);
        glColor3f(1, 0, 0);
        glBegin(GL_LINES);
        for (const auto& s : segments) {
            glVertex2f(points[s.p1].x, points[s.p1].y);
            glVertex2f(points[s.p2].x, points[s.p2].y);
        }
        glEnd();

        // Draw Points
        for (int i = 0; i < (int)points.size(); ++i) {
            if (i == selectedPt) glColor3f(0, 1, 1); else glColor3f(0, 1, 0);
            drawCircle(points[i].x, points[i].y, nodeRadius / zoom);
        }

        if (showNodeNumbers) {
            for (int i = 0; i < (int)points.size(); ++i) {
                double offset = 15.0 / zoom;
                drawText(points[i].x + offset, points[i].y + offset, i);
            }
        }
    }

    void drawCircle(float cx, float cy, float r) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for (int i = 0; i <= CIRCLE_SEGMENTS; ++i) {
            float a = 2.0f * 3.14159f * i / CIRCLE_SEGMENTS;
            glVertex2f(cx + r * cosf(a), cy + r * sinf(a));
        }
        glEnd();
    }

    void drawGrid() {
        if (!showGrid) return;
        glLineWidth(1.0f);
        glColor4f(0.0f, 0.5f, 0.0f, 0.8f);
        glBegin(GL_LINES);
        for (double x = 0; x <= imgW; x += gridStep) {
            glVertex2f(x, 0); glVertex2f(x, imgH);
        }
        for (double y = 0; y <= imgH; y += gridStep) {
            glVertex2f(0, y); glVertex2f(imgW, y);
        }
        glEnd();
    }

    void drawText(double x, double y, int n) {
        if (textTextures.find(n) == textTextures.end()) {
            std::string s = std::to_string(n);
            cv::Mat textImg(24, 60, CV_8UC4, cv::Scalar(0,0,0,0));
            cv::putText(textImg, s, cv::Point(2, 18), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255,255,255,255), 2, cv::LINE_AA);
            
            GLuint tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textImg.cols, textImg.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, textImg.data);
            textTextures[n] = tex;
            textSizes[n] = textImg.size();
        }
        
        cv::Size ts = textSizes[n];
        double w = ts.width / zoom;
        double h = ts.height / zoom;
        
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textTextures[n]);
        glColor3f(1, 1, 0);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 1); glVertex2d(x, y);
        glTexCoord2f(1, 1); glVertex2d(x + w, y);
        glTexCoord2f(1, 0); glVertex2d(x + w, y + h);
        glTexCoord2f(0, 0); glVertex2d(x, y + h);
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }

    void onMouseButton(GLFWwindow* window, int button, int action, int mods) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        double wx, wy;
        screenToWorld(window, xpos, ypos, wx, wy);

        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS) {
                isPanning = true;
                lastMouseX = xpos; lastMouseY = ypos;
            } else isPanning = false;
            return;
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (currentMode == MODE_MOVE) {
                if (action == GLFW_PRESS) {
                    selectedPt = -1;
                    for (int i = 0; i < (int)points.size(); ++i) {
                        if (std::hypot(points[i].x - wx, points[i].y - wy) < HIT_RADIUS / zoom) {
                            selectedPt = i; break;
                        }
                    }
                } else selectedPt = -1;
            } else if (action == GLFW_PRESS) {
                if (currentMode == MODE_ADD) {
                    points.push_back({wx, wy});
                    int newIdx = (int)points.size() - 1;
                    if (lastPtIdx != -1) segments.push_back({lastPtIdx, newIdx});
                    lastPtIdx = newIdx;
                } else if (currentMode == MODE_DELETE) {
                    int toDelete = -1;
                    for (int i = 0; i < (int)segments.size(); ++i) {
                        auto p1 = points[segments[i].p1], p2 = points[segments[i].p2];
                        double l2 = std::pow(p1.x-p2.x,2) + std::pow(p1.y-p2.y,2);
                        if (l2 < NEIGHBOR_DIST_SQ) continue;
                        double t = ((wx-p1.x)*(p2.x-p1.x) + (wy-p1.y)*(p2.y-p1.y)) / l2;
                        t = std::max(0.0, std::min(1.0, t));
                        if (std::hypot(p1.x + t*(p2.x-p1.x) - wx, p1.y + t*(p2.y-p1.y) - wy) < SEGMENT_HIT_RADIUS / zoom) {
                            toDelete = i; break;
                        }
                    }
                    if (toDelete != -1) segments.erase(segments.begin() + toDelete);
                }
            }
        }
    }

    void onCursorPos(GLFWwindow* window, double xpos, double ypos) {
        if (isPanning) {
            double dx = (xpos - lastMouseX);
            double dy = (ypos - lastMouseY);
            double halfW = imgW / 2.0 / zoom;
            double halfH = imgH / 2.0 / zoom;
            int w, h; glfwGetWindowSize(window, &w, &h);
            panX -= dx * halfW / (w / 2.0);
            panY += dy * halfH / (h / 2.0);
            lastMouseX = xpos; lastMouseY = ypos;
        } else if (selectedPt != -1) {
            double wx, wy;
            screenToWorld(window, xpos, ypos, wx, wy);
            points[selectedPt] = {wx, wy};
        }
    }

    void onScroll(double yoffset) {
        zoom *= (yoffset > 0) ? 1.1 : 0.9;
    }

    void resetView(GLFWwindow* window) {
        if (imgW <= 0 || imgH <= 0) return;
        int w, h; glfwGetWindowSize(window, &w, &h);
        double scaleX = (double)w / imgW;
        double scaleY = (double)h / imgH;
        zoom = std::min(scaleX, scaleY) * 0.9;
        panX = imgW / 2.0;
        panY = imgH / 2.0;
    }

    void thinning(cv::Mat& img) {
        img /= 255;
        cv::Mat prev = cv::Mat::zeros(img.size(), CV_8UC1);
        cv::Mat diff;
        while (true) {
            cv::Mat marker = cv::Mat::zeros(img.size(), CV_8UC1);
            for (int iter = 0; iter < 2; iter++) {
                for (int y = 1; y < img.rows - 1; y++) {
                    for (int x = 1; x < img.cols - 1; x++) {
                        if (img.at<uchar>(y, x) == 0) continue;
                        uchar p2 = img.at<uchar>(y - 1, x);
                        uchar p3 = img.at<uchar>(y - 1, x + 1);
                        uchar p4 = img.at<uchar>(y, x + 1);
                        uchar p5 = img.at<uchar>(y + 1, x + 1);
                        uchar p6 = img.at<uchar>(y + 1, x);
                        uchar p7 = img.at<uchar>(y + 1, x - 1);
                        uchar p8 = img.at<uchar>(y, x - 1);
                        uchar p9 = img.at<uchar>(y - 1, x - 1);
                        int A = (p2 == 0 && p3 == 1) + (p3 == 0 && p4 == 1) + (p4 == 0 && p5 == 1) + (p5 == 0 && p6 == 1) +
                                (p6 == 0 && p7 == 1) + (p7 == 0 && p8 == 1) + (p8 == 0 && p9 == 1) + (p9 == 0 && p2 == 1);
                        int B = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;
                        int m1 = (iter == 0) ? (p2 * p4 * p6) : (p2 * p4 * p8);
                        int m2 = (iter == 0) ? (p4 * p6 * p8) : (p2 * p6 * p8);
                        if (A == 1 && B >= 2 && B <= 6 && m1 == 0 && m2 == 0) marker.at<uchar>(y, x) = 1;
                    }
                }
                img -= marker;
            }
            cv::absdiff(img, prev, diff);
            if (cv::countNonZero(diff) == 0) break;
            img.copyTo(prev);
        }
        img *= 255;
    }

void extractSkeletonPaths(cv::Mat& skel, std::vector<std::vector<cv::Point>>& paths) {
        cv::Mat visited = cv::Mat::zeros(skel.size(), CV_8UC1);
        
        bool changed = true;
        while (changed) {
            changed = false;
            std::vector<cv::Point> endpoints;
            for (int y = 1; y < skel.rows - 1; y++) {
                for (int x = 1; x < skel.cols - 1; x++) {
                    if (skel.at<uchar>(y, x) == 0) continue;
                    int neighbors = 0;
                    for (int i = -1; i <= 1; i++)
                        for (int j = -1; j <= 1; j++)
                            if ((i != 0 || j != 0) && skel.at<uchar>(y + i, x + j) > 0) neighbors++;
                    if (neighbors == 1) endpoints.push_back(cv::Point(x, y));
                }
            }
            for (auto p : endpoints) {
                int len = 0;
                cv::Point curr = p;
                std::vector<cv::Point> spur;
                while (len < 6) {
                    spur.push_back(curr);
                    cv::Point next(-1, -1);
                    int n_count = 0;
                    for (int i = -1; i <= 1; i++) {
                        for (int j = -1; j <= 1; j++) {
                            if (i == 0 && j == 0) continue;
                            cv::Point n(curr.x + j, curr.y + i);
                            bool in_spur = false;
                            for (auto sp : spur) if (sp == n) in_spur = true;
                            if (n.y >= 0 && n.y < skel.rows && n.x >= 0 && n.x < skel.cols &&
                                !in_spur && skel.at<uchar>(n.y, n.x) > 0) {
                                next = n; n_count++;
                            }
                        }
                    }
                    if (n_count == 1) { curr = next; len++; }
                    else { if (n_count > 1) len = 100; break; }
                }
                if (len < 6) {
                    for (auto sp : spur) skel.at<uchar>(sp.y, sp.x) = 0;
                    changed = true;
                }
            }
        }

        std::vector<cv::Point> nodes;
        for (int y = 0; y < skel.rows; y++) {
            for (int x = 0; x < skel.cols; x++) {
                if (skel.at<uchar>(y, x) == 0) continue;
                int neighbors = 0;
                for (int i = -1; i <= 1; i++)
                    for (int j = -1; j <= 1; j++)
                        if ((i != 0 || j != 0) && y+i >= 0 && y+i < skel.rows && x+j >= 0 && x+j < skel.cols)
                            if (skel.at<uchar>(y + i, x + j) > 0) neighbors++;
                if (neighbors != 2) nodes.push_back(cv::Point(x, y));
            }
        }

        for (const auto& start : nodes) {
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    if (i == 0 && j == 0) continue;
                    cv::Point current(start.x + j, start.y + i);
                    if (current.x < 0 || current.x >= skel.cols || current.y < 0 || current.y >= skel.rows) continue;
                    if (skel.at<uchar>(current.y, current.x) > 0 && !visited.at<uchar>(current.y, current.x)) {
                        std::vector<cv::Point> path;
                        path.push_back(start);
                        cv::Point p = current;
                        while (true) {
                            path.push_back(p);
                            visited.at<uchar>(p.y, p.x) = 255;
                            
                            int deg = 0;
                            cv::Point next(-1, -1);
                            for (int ni = -1; ni <= 1; ni++) {
                                for (int nj = -1; nj <= 1; nj++) {
                                    if (ni == 0 && nj == 0) continue;
                                    cv::Point n(p.x + nj, p.y + ni);
                                    if (n.x >= 0 && n.x < skel.cols && n.y >= 0 && n.y < skel.rows) {
                                        if (skel.at<uchar>(n.y, n.x) > 0) {
                                            deg++;
                                            if (!visited.at<uchar>(n.y, n.x) && n != start) next = n;
                                        }
                                    }
                                }
                            }
                            if (deg != 2 || next.x == -1) {
                                cv::Point end_node = p;
                                for (const auto& node : nodes) {
                                    if (std::abs(node.x - p.x) <= 1 && std::abs(node.y - p.y) <= 1 && node != start) {
                                        end_node = node; break;
                                    }
                                }
                                if (end_node != p) path.back() = end_node;
                                break;
                            }
                            p = next;
                        }
                        if (path.size() > 1) paths.push_back(path);
                    }
                }
            }
        }
        for (int y = 0; y < skel.rows; y++) {
            for (int x = 0; x < skel.cols; x++) {
                if (skel.at<uchar>(y, x) > 0 && !visited.at<uchar>(y, x)) {
                    std::vector<cv::Point> path;
                    cv::Point p(x, y);
                    while (p.x != -1 && !visited.at<uchar>(p.y, p.x)) {
                        visited.at<uchar>(p.y, p.x) = 255;
                        path.push_back(p);
                        cv::Point next(-1, -1);
                        for (int i = -1; i <= 1; i++) {
                            for (int j = -1; j <= 1; j++) {
                                if (i == 0 && j == 0) continue;
                                cv::Point n(p.x + j, p.y + i);
                                if (n.x >= 0 && n.x < skel.cols && n.y >= 0 && n.y < skel.rows)
                                    if (skel.at<uchar>(n.y, n.x) > 0 && !visited.at<uchar>(n.y, n.x)) {
                                        next = n; break;
                                    }
                            }
                            if (next.x != -1) break;
                        }
                        p = next;
                    }
                    if (path.size() > 2) { path.push_back(path.front()); paths.push_back(path); }
                }
            }
        }
    }

    void applyCanny() {
        if (original.empty()) return;
        cv::Mat gray, filtered, edges;
        cv::cvtColor(original, gray, cv::COLOR_BGR2GRAY);
        cv::bilateralFilter(gray, filtered, 9, 75, 75);
        cv::Canny(filtered, edges, cannyThreshold1, cannyThreshold2);
        
        if (useThinning) {
            // No dilation - it rounds corners!
            thinning(edges);
        }
        
        std::vector<std::vector<cv::Point>> rawPaths;
        if (useThinning) {
            extractSkeletonPaths(edges, rawPaths);
        } else {
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(edges, contours, cv::RETR_LIST, cv::CHAIN_APPROX_NONE);
            rawPaths = contours;
        }

        // Merge touching paths for better traverse direction
        std::vector<std::vector<cv::Point>> mergedPaths;
        std::list<std::vector<cv::Point>> unmerged(rawPaths.begin(), rawPaths.end());
        
        while (!unmerged.empty()) {
            std::vector<cv::Point> current = std::move(unmerged.front());
            unmerged.pop_front();
            
            bool found = true;
            while (found) {
                found = false;
                for (auto it = unmerged.begin(); it != unmerged.end(); ++it) {
                    double d1 = cv::norm(current.back() - it->front());
                    double d2 = cv::norm(current.back() - it->back());
                    double d3 = cv::norm(current.front() - it->back());
                    double d4 = cv::norm(current.front() - it->front());
                    
                    if (d1 < 1.5) {
                        current.insert(current.end(), it->begin() + 1, it->end());
                        unmerged.erase(it); found = true; break;
                    } else if (d2 < 1.5) {
                        std::reverse(it->begin(), it->end());
                        current.insert(current.end(), it->begin() + 1, it->end());
                        unmerged.erase(it); found = true; break;
                    } else if (d3 < 1.5) {
                        current.insert(current.begin(), it->begin(), it->end() - 1);
                        unmerged.erase(it); found = true; break;
                    } else if (d4 < 1.5) {
                        std::reverse(it->begin(), it->end());
                        current.insert(current.begin(), it->begin(), it->end() - 1);
                        unmerged.erase(it); found = true; break;
                    }
                }
            }
            mergedPaths.push_back(current);
        }

        // Sub-pixel corner detection for better precision
        std::vector<cv::Point2f> corners;
        cv::goodFeaturesToTrack(gray, corners, 1000, 0.01, 5);
        if (!corners.empty()) {
            cv::TermCriteria criteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 40, 0.001);
            cv::cornerSubPix(gray, corners, cv::Size(5, 5), cv::Size(-1, -1), criteria);
        }

        points.clear(); segments.clear();
        for (auto& path : mergedPaths) {
            if (path.size() < 2) continue;

            bool isClosed = useThinning ? false : (cv::norm(path.front() - path.back()) < CLOSE_POLY_DIST);
            
            // Use raw path points directly for 1-pixel spacing
            std::vector<Point> pathPoints;
            for (auto& ap : path) {
                cv::Point2f bestCorner = cv::Point2f(ap.x, ap.y);
                float minDist = CORNER_SNAP_DIST;
                for (const auto& corner : corners) {
                    float d = cv::norm(cv::Point2f(ap.x, ap.y) - corner);
                    if (d < minDist) { minDist = d; bestCorner = corner; }
                }
                pathPoints.push_back({(double)bestCorner.x, (double)(imgH - bestCorner.y)});
            }

            int startIdx = points.size();
            for (size_t i = 0; i < pathPoints.size(); ++i) {
                points.push_back(pathPoints[i]);
                if (i > 0) segments.push_back({(int)points.size() - 2, (int)points.size() - 1});
            }
            if (isClosed && pathPoints.size() > 2)
                segments.push_back({(int)points.size() - 1, startIdx});
        }
        std::cout << "Applied: t1=" << cannyThreshold1 << " t2=" << cannyThreshold2 << " eps=" << epsilon << " thin=" << useThinning << " pts=" << points.size() << std::endl;
    }



    void savePoly(const std::string& path) {
        std::ofstream f(path);
        f << points.size() << " 2 0 0\n";
        for (int i = 0; i < (int)points.size(); ++i) f << i << " " << points[i].x << " " << points[i].y << " 0\n";
        f << segments.size() << " 0\n";
        for (int i = 0; i < (int)segments.size(); ++i) f << i << " " << segments[i].p1 << " " << segments[i].p2 << " 0\n";
        f << "0\n";
        std::cout << "Saved to " << path << std::endl;
    }
};

ImagePolyGL app;
void mouseBtnWrap(GLFWwindow* w, int b, int a, int m) { app.onMouseButton(w,b,a,m); }
void cursorPosWrap(GLFWwindow* w, double x, double y) { app.onCursorPos(w,x,y); }
void scrollWrap(GLFWwindow* w, double x, double y)    { app.onScroll(y); }
void windowSizeWrap(GLFWwindow* w, int width, int height) {
    app.resetView(w);
}
void keyWrap(GLFWwindow* w, int k, int s, int a, int m) {
    if (a != GLFW_PRESS && a != GLFW_REPEAT) return;
    if (k == GLFW_KEY_Q) glfwSetWindowShouldClose(w, true);
    if (k == GLFW_KEY_C) app.applyCanny();
    if (k == GLFW_KEY_M) app.currentMode = MODE_MOVE;
    if (k == GLFW_KEY_A) app.currentMode = MODE_ADD;
    if (k == GLFW_KEY_D) app.currentMode = MODE_DELETE;
    if (k == GLFW_KEY_F) app.lastPtIdx = -1;
    if (k == GLFW_KEY_S) app.savePoly("output.poly");
    if (k == GLFW_KEY_R) app.resetView(w);
    if (k == GLFW_KEY_I) app.showImage = !app.showImage;
    if (k == GLFW_KEY_G) app.showGrid = !app.showGrid;
    if (k == GLFW_KEY_N) app.showNodeNumbers = !app.showNodeNumbers;
    if (k == GLFW_KEY_EQUAL) { app.gridStep += 5.0; std::cout << "Grid Step: " << app.gridStep << std::endl; }
    if (k == GLFW_KEY_MINUS) { app.gridStep = std::max(1.0, app.gridStep - 5.0); std::cout << "Grid Step: " << app.gridStep << std::endl; }
    if (k == GLFW_KEY_L) { app.onlyLargest = !app.onlyLargest; std::cout << "Only Largest: " << app.onlyLargest << std::endl; }
    if (k == GLFW_KEY_T) { app.useThinning = !app.useThinning; std::cout << "Use Thinning: " << app.useThinning << std::endl; app.applyCanny(); }
    if (k == GLFW_KEY_U) { app.lineWidth += 0.5f; std::cout << "Line Width: " << app.lineWidth << std::endl; }
    if (k == GLFW_KEY_J) { app.lineWidth = std::max(0.5f, app.lineWidth - 0.5f); std::cout << "Line Width: " << app.lineWidth << std::endl; }
    if (k == GLFW_KEY_O) { app.nodeRadius += 0.5f; std::cout << "Node Radius: " << app.nodeRadius << std::endl; }
    if (k == GLFW_KEY_K) { app.nodeRadius = std::max(0.5f, app.nodeRadius - 0.5f); std::cout << "Node Radius: " << app.nodeRadius << std::endl; }
    if (k == GLFW_KEY_LEFT_BRACKET) { app.cannyThreshold1 = std::max(0, app.cannyThreshold1 - 5); app.applyCanny(); }
    if (k == GLFW_KEY_RIGHT_BRACKET) { app.cannyThreshold1 += 5; app.applyCanny(); }
    if (k == GLFW_KEY_SEMICOLON) { app.cannyThreshold2 = std::max(0, app.cannyThreshold2 - 5); app.applyCanny(); }
    if (k == GLFW_KEY_APOSTROPHE) { app.cannyThreshold2 += 5; app.applyCanny(); }
    if (k == GLFW_KEY_COMMA) { app.epsilon = std::max(0.0, app.epsilon - 0.1); app.applyCanny(); }
    if (k == GLFW_KEY_PERIOD) { app.epsilon += 0.1; app.applyCanny(); }
}



int main(int argc, char** argv) {
    std::cerr << "Starting..." << std::endl;
    if (!glfwInit()) { std::cerr << "glfwInit failed" << std::endl; return -1; }
    std::cerr << "GLFW init ok" << std::endl;
    std::string path = (argc > 1) ? argv[1] : "butterfly.jpg";

    // Pre-load to get dimensions
    app.load(path);
    int winW = app.imgW + 50;
    int winH = app.imgH + 50;
    if (winW < 400) winW = 400;
    if (winH < 300) winH = 300;

    std::cerr << "Creating window " << winW << "x" << winH << std::endl;
    GLFWwindow* window = glfwCreateWindow(winW, winH, "ImagePoly GL", NULL, NULL);
    if (!window) { std::cerr << "Window creation failed" << std::endl; return -1; }
    std::cerr << "Window created" << std::endl;
    glfwMakeContextCurrent(window);
    std::cerr << "Context current" << std::endl;
    
    // Load again with context for textures if it was an image
    app.load(path);
    std::cerr << "Resource loaded: " << app.imgW << "x" << app.imgH << " pts: " << app.points.size() << std::endl;

    glfwSetMouseButtonCallback(window, mouseBtnWrap);
    glfwSetCursorPosCallback(window, cursorPosWrap);
    glfwSetScrollCallback(window, scrollWrap);
    glfwSetWindowSizeCallback(window, windowSizeWrap);
    glfwSetKeyCallback(window, keyWrap);

    app.resetView(window);
    std::cerr << "resetView done" << std::endl;

    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    std::cerr << "Entering loop" << std::endl;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        app.draw(window);
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
