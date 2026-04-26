#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <vector>

#include "camera.hpp"
#include "geometry.hpp"
#include "shadows.hpp"
#include "wind.hpp"
#include "shader_utils.hpp"

static constexpr int SMAP = 4096;
static constexpr int DEG = 3;
static constexpr float AMBIENT = 0.7f;
static constexpr int BUNCH_COUNT = 12;
static constexpr int ROCK_COUNT = 6;
static constexpr float SUN_DIST = 2.f;

int main() {
    // window
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* win = glfwCreateWindow(1100, 750, "Test", nullptr, nullptr);
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    // input
    glfwSetMouseButtonCallback(win, onMouse);
    glfwSetCursorPosCallback(win, onCursor);
    glfwSetScrollCallback(win, onScroll);

    // opengl
    glewExperimental = GL_TRUE;
    glewInit();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui::GetIO().FontGlobalScale = 1.2f;

    // shaders
    GLuint gProg = linkProgram("shaders/grass.vert", "shaders/grass.frag");
    GLuint pProg = linkProgram("shaders/plane.vert", "shaders/plane.frag");
    GLuint sProg = linkProgram("shaders/sun.vert",   "shaders/sun.frag");
    GLuint dProg = linkProgram("shaders/depth.vert", "shaders/depth.frag");
    GLuint rProg = linkProgram("shaders/rock.vert",  "shaders/rock.frag");
    auto ul = [](GLuint p, const char* n) { return glGetUniformLocation(p, n); };

    // wind
    WindParams wind;

    // geometry
    auto knots = generateClampedKnots(5, DEG);
    auto bunches = createGrassBunches(BUNCH_COUNT);
    auto rocks = createRocks(ROCK_COUNT, 0.1f);

    GLuint pVBO;
    glGenBuffers(1, &pVBO);
    { auto pv = buildPlane(); upload(pVBO, pv, GL_STATIC_DRAW); }
    GLuint pVAO = makeVAO(pVBO);

    auto sv = buildSphere(0.025f);
    GLuint sVBO;
    glGenBuffers(1, &sVBO);
    upload(sVBO, sv, GL_STATIC_DRAW);
    GLuint sVAO = makeVAO(sVBO, true);

    // shadow map
    GLuint shadowFBO, shadowTex;
    initShadowMap(shadowFBO, shadowTex, SMAP);

    // sun
    float sunAngle = 45.f;
    float sunHeight = 40.f;
    glm::vec3 lightColor(1.f, 0.97f, 0.85f);
    bool shadows = true;

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        // imgui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos({ 10, 10 }, ImGuiCond_Always);
        ImGui::SetNextWindowSize({ 340, 0 }, ImGuiCond_Always);
        ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::Separator(); ImGui::Text("Wind");
        ImGui::SliderFloat("Sway", &wind.swayAmount, 0.f, 0.08f, "%.4f");
        ImGui::SliderFloat("Frequency", &wind.frequency, 0.f, 8.f, "%.2f");

        ImGui::Separator(); ImGui::Text("Sun");
        ImGui::SliderFloat("Sun angle", &sunAngle, 0.f, 360.f, "%.1f");
        ImGui::SliderFloat("Sun height", &sunHeight, -90.f, 90.f, "%.1f");
        ImGui::Checkbox("Shadows", &shadows);

        ImGui::Separator(); ImGui::TextDisabled("Camera");
        ImGui::Text("Yaw:%.1f Pitch:%.1f Dist:%.2f", cam.yaw, cam.pitch, cam.dist);
        ImGui::End();

        // sun position
        float ar = glm::radians(sunAngle);
        float hr = glm::radians(sunHeight);
        glm::vec3 sunPos(
            SUN_DIST * std::cos(hr) * std::cos(ar),
            SUN_DIST * std::sin(hr),
            SUN_DIST * std::cos(hr) * std::sin(ar)
        );
        glm::vec3 lightDir = glm::normalize(sunPos);

        // grass animation
        auto t = static_cast<float>(glfwGetTime());
        std::vector<Vert> gv;
        for (auto& bunch : bunches) {
            gv.clear();
            gv.reserve(bunch.blades.size() * 32 * 6);
            for (auto& b : bunch.blades) {
                updateBlade(b, t, wind);
                buildBlade(b, knots, DEG, gv);
            }
            upload(bunch.vbo, gv);
        }

        // light
        glm::mat4 lProj = glm::ortho(-0.7f, 0.7f, -0.7f, 0.7f, 0.1f, 6.f);
        glm::mat4 lView = glm::lookAt(glm::normalize(sunPos) * 3.f, { 0,0,0 }, { 0,1,0 });
        glm::mat4 lMVP = lProj * lView;

        // shadow pass
        if (shadows) {
            glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
            glViewport(0, 0, SMAP, SMAP);
            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(2, 4);
            glUseProgram(dProg);

            glm::mat4 identity(1.f);
            glUniformMatrix4fv(glGetUniformLocation(dProg, "uLightMVP"), 1, GL_FALSE, glm::value_ptr(lMVP));
            glUniformMatrix4fv(glGetUniformLocation(dProg, "uModel"), 1, GL_FALSE, glm::value_ptr(identity));

            glBindVertexArray(pVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            for (auto& bunch : bunches) {
                glBindVertexArray(bunch.vao);
                glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(bunch.blades.size() * 32 * 6));
            }

            for (auto& rock : rocks) {
                glm::mat4 model = glm::translate(glm::mat4(1.f), rock.pos);
                glUniformMatrix4fv(glGetUniformLocation(dProg, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
                glBindVertexArray(rock.vao);
                glDrawArrays(GL_TRIANGLES, 0, rock.vertCount);
            }

            glDisable(GL_POLYGON_OFFSET_FILL);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // main pass
        int fw, fh;
        glfwGetFramebufferSize(win, &fw, &fh);
        glViewport(0, 0, fw, fh);
        glClearColor(0.47f, 0.67f, 0.95f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj = glm::perspective(glm::radians(45.f),
            static_cast<float>(fw) / static_cast<float>(fh), 0.01f, 10.f);
        glm::mat4 MVP = proj * cam.view();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shadowTex);

        auto setCommon = [&](GLuint prog) {
            glUniformMatrix4fv(ul(prog, "uMVP"), 1, GL_FALSE, glm::value_ptr(MVP));
            glUniformMatrix4fv(ul(prog, "uLightMVP"), 1, GL_FALSE, glm::value_ptr(lMVP));
            glUniform3fv(ul(prog, "uLightDir"), 1, glm::value_ptr(lightDir));
            glUniform3fv(ul(prog, "uLightColor"), 1, glm::value_ptr(lightColor));
            glUniform1i(ul(prog, "uShadowMap"), 0);
            glUniform1i(ul(prog, "uShadowsEnabled"), shadows ? 1 : 0);
        };

        // plane
        glUseProgram(pProg);
        setCommon(pProg);
        glUniform1f(ul(pProg, "uAmbient"), AMBIENT + 0.2f);
        glBindVertexArray(pVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // grass
        glUseProgram(gProg);
        setCommon(gProg);
        glUniform1f(ul(gProg, "uAmbient"), AMBIENT);
        for (auto& bunch : bunches) {
            glBindVertexArray(bunch.vao);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(bunch.blades.size() * 32 * 6));
        }

        // rocks
        glUseProgram(rProg);
        glUniform3fv(ul(rProg, "uLightDir"), 1, glm::value_ptr(lightDir));
        glUniform3fv(ul(rProg, "uLightColor"), 1, glm::value_ptr(lightColor));
        glUniform1f(ul(rProg, "uAmbient"), AMBIENT);
        glUniform1i(ul(rProg, "uShadowMap"), 0);
        glUniform1i(ul(rProg, "uShadowsEnabled"), shadows ? 1 : 0);
        glUniformMatrix4fv(ul(rProg, "uLightMVP"), 1, GL_FALSE, glm::value_ptr(lMVP));
        for (auto& rock : rocks) {
            glm::mat4 model = glm::translate(glm::mat4(1.f), rock.pos);
            glm::mat4 rockMVP = MVP * model;
            glUniformMatrix4fv(ul(rProg, "uMVP"), 1, GL_FALSE, glm::value_ptr(rockMVP));
            glUniformMatrix4fv(ul(rProg, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(rock.vao);
            glDrawArrays(GL_TRIANGLES, 0, rock.vertCount);
        }

        // sun
        glm::mat4 sMVP = MVP * glm::translate(glm::mat4(1.f), sunPos);
        glUseProgram(sProg);
        glUniformMatrix4fv(ul(sProg, "uMVP"), 1, GL_FALSE, glm::value_ptr(sMVP));
        glUniform3fv(ul(sProg, "uSunColor"), 1, glm::value_ptr(lightColor));
        glBindVertexArray(sVAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(sv.size()));
        glBindVertexArray(0);

        // imgui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);
    }

    // cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glDeleteFramebuffers(1, &shadowFBO);
    glDeleteTextures(1, &shadowTex);
    for (auto& bunch : bunches) {
        glDeleteVertexArrays(1, &bunch.vao);
        glDeleteBuffers(1, &bunch.vbo);
    }
    for (auto& rock : rocks) {
        glDeleteVertexArrays(1, &rock.vao);
        glDeleteBuffers(1, &rock.vbo);
    }
    for (auto vao : { pVAO, sVAO }) glDeleteVertexArrays(1, &vao);
    for (auto vbo : { pVBO, sVBO }) glDeleteBuffers(1, &vbo);
    for (auto p : { gProg, pProg, sProg, dProg, rProg }) glDeleteProgram(p);
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}