#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include "TriMeshGenerator.hpp"
#include "MeshOptimizer.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace triangle;

// --- Application State & Logic ---
class MeshApp {
public:
    enum InteractionMode {
        MODE_ADD_POINTS,
        MODE_DRAW_POLYGON
    };

    MeshApp() : m_mesh() {
        m_mesh.addBoundingBox(0.05, 0.05, 0.95, 0.95);
        updateMesh();
    }

    void updateMesh() {
        Options opts;
        opts.voronoi = true;
        opts.quality = m_qualityMode;
        opts.minAngle = m_minAngle;
        opts.maxArea = (m_maxArea > 0) ? m_maxArea : -1.0;
        m_mesh.triangulate(opts);
    }

    void resetView() {
        m_zoom = 1.0;
        m_panX = 0.5;
        m_panY = 0.5;
    }

    void screenToWorld(GLFWwindow* window, double x, double y, double& wx, double& wy) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        double aspect = (double)width / height;
        double nx = (2.0 * x / width) - 1.0;
        double ny = 1.0 - (2.0 * y / height);
        wx = (nx * aspect / m_zoom) + m_panX;
        wy = (ny / m_zoom) + m_panY;
    }

    // --- Callbacks ---
    void onMouseButton(GLFWwindow* window, int button, int action, int mods) {
        if (ImGui::GetIO().WantCaptureMouse) return;

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS) {
                m_isPanning = true;
                m_lastMouseX = xpos;
                m_lastMouseY = ypos;
            } else {
                m_isPanning = false;
            }
            return;
        }

        double wx, wy;
        screenToWorld(window, xpos, ypos, wx, wy);

        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                m_isDragging = true;
                if (m_mode == MODE_DRAW_POLYGON) {
                    int newIdx = m_mesh.numInputPoints();
                    m_mesh.addPoint(wx, wy, 1);
                    if (!m_polyIndices.empty()) {
                        m_mesh.addSegment(m_polyIndices.back(), newIdx, 1);
                    }
                    m_polyIndices.push_back(newIdx);
                    updateMesh();
                    return;
                }

                // Point selection / addition
                m_selectedPoint = -1;
                const auto& pts = m_mesh.inputPoints();
                for (size_t i = 0; i < pts.size(); ++i) {
                    double dx = pts[i].x - wx;
                    double dy = pts[i].y - wy;
                    if (std::sqrt(dx*dx + dy*dy) < 0.02 / m_zoom) {
                        m_selectedPoint = (int)i;
                        break;
                    }
                }
                if (m_selectedPoint == -1) {
                    m_mesh.addPoint(wx, wy);
                    updateMesh();
                }
            } else if (action == GLFW_RELEASE) {
                m_isDragging = false;
                m_selectedPoint = -1;
            }
        }
    }

    void onCursorPos(GLFWwindow* window, double xpos, double ypos) {
        if (ImGui::GetIO().WantCaptureMouse) return;

        if (m_isPanning) {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            double aspect = (double)width / height;
            double dx = (xpos - m_lastMouseX) / width;
            double dy = (ypos - m_lastMouseY) / height;
            m_panX -= dx * 2.0 * aspect / m_zoom;
            m_panY += dy * 2.0 / m_zoom;
            m_lastMouseX = xpos;
            m_lastMouseY = ypos;
            return;
        }

        if (!m_isDragging) return;

        double wx, wy;
        screenToWorld(window, xpos, ypos, wx, wy);

        if (m_mode == MODE_DRAW_POLYGON) {
            if (m_polyIndices.empty()) return;
            const auto& inPts = m_mesh.inputPoints();
            const auto& lastPt = inPts[m_polyIndices.back()];
            double dx = wx - lastPt.x, dy = wy - lastPt.y;
            if (std::sqrt(dx*dx + dy*dy) > 0.02 / m_zoom) {
                int newIdx = m_mesh.numInputPoints();
                m_mesh.addPoint(wx, wy, 1);
                m_mesh.addSegment(m_polyIndices.back(), newIdx, 1);
                m_polyIndices.push_back(newIdx);
                updateMesh();
            }
        } else if (m_selectedPoint != -1) {
            auto& inPts = m_mesh.mutableInputPoints();
            if (m_selectedPoint < (int)inPts.size()) {
                inPts[m_selectedPoint].x = wx;
                inPts[m_selectedPoint].y = wy;
                updateMesh();
            }
        }
    }

    void onScroll(double yoffset) {
        if (ImGui::GetIO().WantCaptureMouse) return;
        m_zoom *= (yoffset > 0) ? 1.1 : 0.9;
    }

    void onKey(int key, int action) {
        if (ImGui::GetIO().WantCaptureKeyboard) return;
        if (action != GLFW_PRESS) return;

        switch (key) {
            case GLFW_KEY_D: m_showDelaunay = !m_showDelaunay; break;
            case GLFW_KEY_V: m_showVoronoi = !m_showVoronoi; break;
            case GLFW_KEY_P: togglePolygonMode(); break;
            case GLFW_KEY_C: clearCanvas(); break;
        }
    }

    // --- UI & Rendering ---
    void drawUI(GLFWwindow* window) {
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Workspace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar)) {
            renderMenuBar(window);
            renderControlPanel();
        }
        ImGui::End();

        renderStatsOverlay();

        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    }

    void renderApp(GLFWwindow* window) {
        glClear(GL_COLOR_BUFFER_BIT);
        setupMatrices(window);
        
        if (m_showDelaunay) drawDelaunay();
        if (m_showVoronoi) drawVoronoi();
        if (m_showPoints) drawPoints();
        drawSegments();

        drawUI(window);
    }

private:
    void clearCanvas() {
        m_mesh.clear();
        m_mesh.addBoundingBox(0.05, 0.05, 0.95, 0.95);
        m_polyIndices.clear();
        m_refineLen = 0.2;
        updateMesh();
    }

    void togglePolygonMode() {
        if (m_mode == MODE_DRAW_POLYGON && m_polyIndices.size() > 2) {
            m_mesh.addSegment(m_polyIndices.back(), m_polyIndices.front(), 1);
            updateMesh();
            m_polyIndices.clear();
        }
        m_mode = (m_mode == MODE_DRAW_POLYGON) ? MODE_ADD_POINTS : MODE_DRAW_POLYGON;
    }

    void setupMatrices(GLFWwindow* window) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        int w, h; glfwGetFramebufferSize(window, &w, &h);
        double aspect = (double)w / h;
        double l = m_panX - (aspect / m_zoom), r = m_panX + (aspect / m_zoom);
        double b = m_panY - (1.0 / m_zoom), t = m_panY + (1.0 / m_zoom);
        glOrtho(l, r, b, t, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }

    void drawDelaunay() {
        const auto& pts = m_mesh.points();
        glColor4f(0.2f, 0.4f, 0.6f, 0.3f);
        glBegin(GL_TRIANGLES);
        for (const auto& t : m_mesh.triangles()) {
            for (int i=0; i<3; ++i) glVertex2f(pts[t.p[i]].x, pts[t.p[i]].y);
        }
        glEnd();
        glColor4f(0.4f, 0.6f, 0.8f, 0.8f);
        glBegin(GL_LINES);
        for (const auto& t : m_mesh.triangles()) {
            for (int i=0; i<3; ++i) {
                glVertex2f(pts[t.p[i]].x, pts[t.p[i]].y);
                glVertex2f(pts[t.p[(i+1)%3]].x, pts[t.p[(i+1)%3]].y);
            }
        }
        glEnd();
    }

    void drawVoronoi() {
        glColor4f(0.8f, 0.2f, 0.2f, 1.0f);
        glBegin(GL_LINES);
        for (const auto& f : m_mesh.voroFaces()) {
            for (size_t i = 0; i < f.vertices.size(); ++i) {
                const auto& p1 = f.vertices[i], &p2 = f.vertices[(i+1)%f.vertices.size()];
                glVertex2f(p1.x, p1.y); glVertex2f(p2.x, p2.y);
            }
        }
        glEnd();
    }

    void drawPoints() {
        const auto& pts = m_mesh.points();
        for (const auto& p : pts) {
            if (p.isFixed || p.marker != 0) glColor3f(1, 0, 0); else glColor3f(1, 1, 1);
            drawCircleInternal(p.x, p.y, 0.005f / m_zoom);
        }
    }

    void drawSegments() {
        const auto& outPts = m_mesh.points();
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glColor3f(1, 0, 0);
        for (const auto& s : m_mesh.segments()) {
            glVertex2f(outPts[s.p[0]].x, outPts[s.p[0]].y);
            glVertex2f(outPts[s.p[1]].x, outPts[s.p[1]].y);
        }
        if (m_mode == MODE_DRAW_POLYGON && !m_polyIndices.empty()) {
            const auto& inPts = m_mesh.inputPoints();
            glColor3f(1, 0.5, 0);
            for (size_t i=0; i<m_polyIndices.size()-1; ++i) {
                const auto& p1 = inPts[m_polyIndices[i]], &p2 = inPts[m_polyIndices[i+1]];
                glVertex2f(p1.x, p1.y); glVertex2f(p2.x, p2.y);
            }
        }
        glEnd();
        glLineWidth(1.0f);
    }

    void drawCircleInternal(float cx, float cy, float r) {
        glBegin(GL_TRIANGLE_FAN);
        for (int i=0; i<12; ++i) {
            float a = 2.0f * 3.14159f * i / 12.0f;
            glVertex2f(cx + r * cosf(a), cy + r * sinf(a));
        }
        glEnd();
    }

    void renderMenuBar(GLFWwindow* window) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Reset Canvas")) clearCanvas();
                if (ImGui::MenuItem("Exit")) glfwSetWindowShouldClose(window, true);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    void renderControlPanel() {
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "MESH CONTROLS");
        if (ImGui::Button("Reset View", ImVec2(-1, 0))) resetView();
        
        if (ImGui::CollapsingHeader("Interactions", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::RadioButton("Point Editor", m_mode == MODE_ADD_POINTS)) m_mode = MODE_ADD_POINTS;
            if (ImGui::RadioButton("Polygon Tool", m_mode == MODE_DRAW_POLYGON)) m_mode = MODE_DRAW_POLYGON;
        }

        if (ImGui::CollapsingHeader("Visualization", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Delaunay", &m_showDelaunay);
            ImGui::Checkbox("Voronoi", &m_showVoronoi);
            ImGui::Checkbox("Points", &m_showPoints);
        }

        if (ImGui::CollapsingHeader("Quality & Optimization")) {
            if (ImGui::Checkbox("Quality Mode", &m_qualityMode)) updateMesh();
            if (m_qualityMode) {
                ImGui::SliderFloat("Angle", &m_minAngle, 0, 35);
                ImGui::SliderFloat("Area", &m_maxArea, 0, 0.01f, "%.5f");
            }
            ImGui::Separator();
            ImGui::SliderInt("Smooth Steps", &m_smoothIters, 1, 20);
            if (ImGui::Button("Smooth Mesh", ImVec2(-1,0))) {
                MeshOptimizer::smooth(m_mesh.mutablePoints(), m_mesh.triangles(), m_smoothIters);
                updateMesh();
            }
        }

        if (ImGui::CollapsingHeader("Refinement")) {
            if (ImGui::Button("Refine Boundaries", ImVec2(-1,0))) {
                m_refineLen *= 0.5; m_mesh.refineSegments(m_refineLen); updateMesh();
            }
            ImGui::TextDisabled("Limit: %.4f", m_refineLen);
        }

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("RESET ALL", ImVec2(-1, 40))) clearCanvas();
        ImGui::PopStyleColor();
    }

    void renderStatsOverlay() {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 220, 10));
        ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs);
        ImGui::Text("V: %d | E: %d | S: %d", (int)m_mesh.points().size(), (int)m_mesh.triangles().size(), (int)m_mesh.segments().size());
        ImGui::End();
    }

    TriangleMesh m_mesh;
    InteractionMode m_mode = MODE_ADD_POINTS;
    bool m_showDelaunay = true, m_showVoronoi = true, m_showPoints = true;
    bool m_qualityMode = false;
    float m_minAngle = 20.0f, m_maxArea = 0.0f;
    int m_smoothIters = 5, m_cvtIters = 10, m_cvtPoints = 50;
    double m_zoom = 1.0, m_panX = 0.5, m_panY = 0.5, m_refineLen = 0.2;
    bool m_isPanning = false, m_isDragging = false;
    double m_lastMouseX = 0, m_lastMouseY = 0;
    int m_selectedPoint = -1;
    std::vector<int> m_polyIndices;
};

// --- Static Dispatchers ---
MeshApp* g_app = nullptr;
void mouseBtnWrap(GLFWwindow* w, int b, int a, int m) { g_app->onMouseButton(w,b,a,m); }
void cursorPosWrap(GLFWwindow* w, double x, double y) { g_app->onCursorPos(w,x,y); }
void scrollWrap(GLFWwindow* w, double x, double y)    { g_app->onScroll(y); }
void keyWrap(GLFWwindow* w, int k, int s, int a, int m) { g_app->onKey(k,a); }

int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(1200, 800, "Mesh Studio", NULL, NULL);
    if (!window) return -1;

    glfwMakeContextCurrent(window);
    g_app = new MeshApp();
    glfwSetMouseButtonCallback(window, mouseBtnWrap);
    glfwSetCursorPosCallback(window, cursorPosWrap);
    glfwSetScrollCallback(window, scrollWrap);
    glfwSetKeyCallback(window, keyWrap);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); io.FontGlobalScale = 1.2f;
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f; style.FrameRounding = 4.0f;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        g_app->renderApp(window);
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL2_Shutdown(); ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    delete g_app;
    glfwTerminate();
    return 0;
}
