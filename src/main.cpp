/*
-- conan install . --output-folder=build --build=missing
*/

#include <iostream>
#include <string>
#include <functional>
#include <vector>

#include <epoxy/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "file_manager/file_manager.h"
#include "shader/shader.h"
#include "camera/camera.h"
#include "cube/cube.h"
#include "scene/scene.h"
#include "scene/scene_manager.h"
#include "plane/plane.h"
#include "pointlight/pointlight.h"
#include "mesh/mesh.h"
#include "texture/texture.h"
#include "raycast/raycast.h"
#include "objmanager/objmanager.h"
#include "engine_ui/engine_ui.h"

#include "includes/imgui/imgui.h"
#include "includes/imgui/imgui_impl_glfw.h"
#include "includes/imgui/imgui_impl_opengl3.h"
#include "includes/imgui/imgui_internal.h"

//Game logic includes
#include "game/submarine/submarine.h"

void errorCallback(int error, const char *description) {
    std::cerr << "GLFW Error: " << error << ": " << description << std::endl;
}


//Manage Inputs
const float lookspeed = 1.5f;
static float baseMoveSpeed = 1.5f; // set by scroll wheel
static float movespeed = 1.5f;     // actual used for movement (may be sprinted)
const float movespeed_min = 0.1f;
const float movespeed_max = 20.0f;
const float sprintMultiplier = 1.75f;

// Mouse look state
static double lastMouseX = 0.0, lastMouseY = 0.0;
static bool firstMouse = true;
static bool mouseCaptured = false;

static float dt = 1.0f;

static int screenWidth, screenHeight;

void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
    if (!mouseCaptured) return;
    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
        return;
    }
    //std::cout << "Mouse moved to: " << xpos << ", " << ypos << " last: " << lastMouseX << ", " << lastMouseY << std::endl;
    double xoffset = xpos - lastMouseX;
    double yoffset = lastMouseY - ypos;
    lastMouseX = xpos;
    lastMouseY = ypos;

    // Get camera and rotate
    Scene* scene = SceneManager::getActiveScene();
    if (scene) {
        Camera* cam = scene->getActiveCamera();
        if (cam) {
            float sensitivity = 1.0f * lookspeed * dt;
            cam->rotateViewDirection({ (float)yoffset * sensitivity, -(float)xoffset * sensitivity });
        }
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (yoffset > 0) {
        baseMoveSpeed *= 1.1f;
    } else if (yoffset < 0) {
        baseMoveSpeed /= 1.1f;
    }
    if (baseMoveSpeed < movespeed_min) baseMoveSpeed = movespeed_min;
    if (baseMoveSpeed > movespeed_max) baseMoveSpeed = movespeed_max;
    //std::cout << "Move speed: " << movespeed << std::endl;
}

//input keys
struct InputAction {
    int key;
    std::function<void()> action;
};



int main() {

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    std::cout << "GLFW Successfully Initialized." << std::endl;

    //set glfw error callback
    glfwSetErrorCallback(errorCallback);

    //set opengl version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#ifdef __linux__
    glfwWindowHint(GLFW_CONTEXT_DEBUG, GLFW_TRUE);
#endif

    float time = (float)glfwGetTime();

    //create window - maximized borderless fullscreen
    GLFWwindow *window = glfwCreateWindow(1920, 1080, "Game", glfwGetPrimaryMonitor(), nullptr);
    if (!window) {
        // Fallback to maximized windowed if fullscreen fails
        window = glfwCreateWindow(1920, 1080, "Game", nullptr, nullptr);
        if (window) {
            glfwMaximizeWindow(window);
        }
    }

    std::cout << "Window created: " << (window ? "yes" : "no") << std::endl;

    //set scroll callback 
    if (window) glfwSetScrollCallback(window, scrollCallback);
    // set mouse move callback
    if (window) glfwSetCursorPosCallback(window, mouseMoveCallback);
    
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    //make opengl context current
    std::cout << "Making context current..." << std::endl;
    glfwMakeContextCurrent(window);
    std::cout << "Context made current." << std::endl;
    glfwSwapInterval(0);
    std::cout << "Swap interval set." << std::endl;

    // Epoxy handles OpenGL function loading automatically
    std::cout << "OpenGL initialized: " << glGetString(GL_VERSION) << std::endl;

    // enable depth testing
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glEnable(GL_DEPTH_TEST);

    // Enable backface culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); // Default for most OBJ files

    // Shadow map setup
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    GLuint depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    // Create depth texture
    GLuint depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);


    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    // Framebuffer completeness check
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: Shadow framebuffer is not complete!" << std::endl;
    } else {
        std::cout << "Shadow framebuffer is complete." << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    //imgui initializing
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); 
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; 
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::StyleColorsDark();
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Get screen dimensions
    
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);

    // Initialize shader
    Shader simpleShader;
    simpleShader.init(
    FileManager::read("src/shaders/simple.vs"),
    FileManager::read("src/shaders/simple.fs")
    );

    Shader shadowShader;
    shadowShader.init(
        FileManager::read("src/shaders/shadow_depth.vs"),
        FileManager::read("src/shaders/shadow_depth.fs")
    );

    // Scene Setup
    Scene* scene = SceneManager::createScene("scene", true);
    scene->loadFromFile("scene.json", screenWidth, screenHeight);
    ShowToast("Scene loaded!");
    // Camera setup
    Camera *camera = scene->getActiveCamera();
    //Camera *camera = new Camera(screenWidth, screenHeight, {0.f,-0.5f,7.f}, {0.f,0.f,-1.f});
    //scene->add(camera);

    /*
    ParentObject<PointLight>* parentLight = new ParentObject<PointLight>(glm::vec3(0,5,0), glm::vec3(1,0,0), 1.0f, true);
    Mesh *childMesh = new Mesh("../assets/CeilingLight/ceiling_light.obj", Material("../assets/CeilingLight/ceiling_light.png"), glm::vec3(0,5,0), glm::vec3(0.5f,0.5f,0.5f));
    scene->add(childMesh);
    parentLight->addChild(childMesh);
    scene->add(parentLight);
    */

    SpotLight* spotLight = new SpotLight(glm::vec3(0,2,0), glm::vec3(0,-1,0), glm::vec3(1,1,1), 3.0f, 22.5f, true);
    scene->add(spotLight);

    // Scene Editing Mode Inits
    bool sceneEditingMode = false;
    bool lastKey1State = false;
    bool lastKey2State = false;
    bool lastKeyEState = false;
    bool lastKeySPressed = false;
    SceneObject* selectedMesh = nullptr;
    SceneObject* interactTarget = nullptr;
    std::string interactionPrompt;
    const float interactionRange = 3.0f;

    // Center mouse before changing capture modes
    mouseCaptured = true;   
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Submarine Game Logic
    Submarine submarine(scene);

    //Main Loop

    while (!glfwWindowShouldClose(window)) {

        // Sprint logic: hold Shift to sprint (multiplies baseMoveSpeed)
        bool shiftPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        if (shiftPressed) {
            movespeed = baseMoveSpeed * sprintMultiplier;
        } else {
            movespeed = baseMoveSpeed;
        }

        //time elapsed since program launch
        float last_time = time;
        time = (float)glfwGetTime();
        dt = time - last_time;

        //background color
        float red = 0;
        float green = 0;
        float blue = 0;

        //Set Input Actions
        std::vector<InputAction> inputActions = {
            {GLFW_KEY_I, [&scene]() { scene->getActiveCamera()->rotateViewDirection({1.5f*dt*lookspeed,0}); }},
            {GLFW_KEY_K, [&scene]() { scene->getActiveCamera()->rotateViewDirection({-1.5f*dt*lookspeed,0}); }},
            {GLFW_KEY_J, [&scene]() { scene->getActiveCamera()->rotateViewDirection({0,1.5f*dt*lookspeed}); }},
            {GLFW_KEY_L, [&scene]() { scene->getActiveCamera()->rotateViewDirection({0,-1.5f*dt*lookspeed}); }},
            {GLFW_KEY_W, [&scene]() { scene->getActiveCamera()->move2D({2*dt*movespeed,0}); }},
            {GLFW_KEY_S, [&scene]() { scene->getActiveCamera()->move2D({-2*dt*movespeed,0}); }},
            {GLFW_KEY_A, [&scene]() { scene->getActiveCamera()->move2D({0,-2*dt*movespeed}); }},
            {GLFW_KEY_D, [&scene]() { scene->getActiveCamera()->move2D({0,2*dt*movespeed}); }},
            // Up/down movement: O = up, U = down
            {GLFW_KEY_O, [&scene]() { scene->getActiveCamera()->moveVertical(2*dt*movespeed); }}, // move up
            {GLFW_KEY_U, [&scene]() { scene->getActiveCamera()->moveVertical(-2*dt*movespeed); }} // move down
        };

        //Manage Inputs
        for (const auto& ia : inputActions) {
            if (glfwGetKey(window, ia.key) == GLFW_PRESS) {
                ia.action();
            }
        }


        //Scene editing toggle 
        bool altPressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
        bool key1Pressed = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
        if (altPressed && key1Pressed && !lastKey1State) {
            sceneEditingMode = !sceneEditingMode;
            if (sceneEditingMode) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                mouseCaptured = false;
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                mouseCaptured = true;
            }
            firstMouse = true;
        }
        lastKey1State = altPressed && key1Pressed;
        bool key2Pressed = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;
        if (altPressed && key2Pressed && !lastKey2State) {
            if (scene->getActiveCamera() == camera) {
                scene->setActiveCamera(static_cast<Camera*>(scene->getFirstObjectWithTag("topdown_camera")));
            } else {
                scene->setActiveCamera(camera);
            }
        }
        lastKey2State = altPressed && key2Pressed;
        //alt + s to save scene 
        bool keySPressed = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;

        if (altPressed && keySPressed && !lastKeySPressed) {
            scene->saveToFile(scene->getName() + ".json");
            std::cout << "Scene saved to " << scene->getName() + ".json" << std::endl;
        }
        lastKeySPressed = altPressed && keySPressed;

        // Keep GLFW cursor mode aligned with edit/game mode.
        int desiredCursorMode = sceneEditingMode ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
        int currentCursorMode = glfwGetInputMode(window, GLFW_CURSOR);
        if (currentCursorMode != desiredCursorMode) {
            glfwSetInputMode(window, GLFW_CURSOR, desiredCursorMode);
            firstMouse = true;
        }
        mouseCaptured = desiredCursorMode == GLFW_CURSOR_DISABLED;


        // GAME LOOP
        if (sceneEditingMode) {
            if (ImGui::GetIO().WantCaptureMouse) {
                // ImGui is handling the mouse input
            } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                double mouseX, mouseY;
                glfwGetCursorPos(window, &mouseX, &mouseY);

                glm::vec3 rayDir = Raycast::getRayFromScreen((float)mouseX, (float)mouseY, screenWidth, screenHeight, camera);
                glm::vec3 rayOrigin = camera->getPosition();

                // Find closest mesh hit by ray
                float closestDist = std::numeric_limits<float>::max();
                SceneObject* hitMesh = nullptr;
                for (auto mesh : scene->getMeshes()) { 
                    float hitDist;
                    if (mesh->intersectRay(rayOrigin, rayDir, hitDist)) { 
                        if (hitDist < closestDist) {
                            closestDist = hitDist;
                            hitMesh = mesh;
                        }
                    }
                }
                if (hitMesh && hitMesh != selectedMesh) {
                    selectedMesh = hitMesh;
                    scene->getGizmo()->setPosition(selectedMesh->getPosition());
                }
            }
        }

        auto getInteractionPromptForObject = [](SceneObject* object) -> std::string {
            if (!object) {
                return "";
            }

            const std::string tag = object->getTag();
            if (tag == "computer") {
                return "[E] Use Computer";
            }
            if (tag == "locker") {
                return "[E] Open Locker";
            }

            return "";
        };

        interactionPrompt.clear();
        interactTarget = nullptr;

        if (!sceneEditingMode) {
            Camera* activeCamera = scene->getActiveCamera();
            if (activeCamera) {
                const float screenCenterX = screenWidth * 0.5f;
                const float screenCenterY = screenHeight * 0.5f;
                glm::vec3 rayDir = Raycast::getRayFromScreen(screenCenterX, screenCenterY, screenWidth, screenHeight, activeCamera);
                glm::vec3 rayOrigin = activeCamera->getPosition();

                float closestDist = std::numeric_limits<float>::max();
                SceneObject* closestInteractable = nullptr;

                for (auto mesh : scene->getMeshes()) {
                    float hitDist;
                    if (!mesh->intersectRay(rayOrigin, rayDir, hitDist)) {
                        continue;
                    }

                    const std::string prompt = getInteractionPromptForObject(mesh);
                    if (prompt.empty() || hitDist > interactionRange || hitDist >= closestDist) {
                        continue;
                    }

                    closestDist = hitDist;
                    closestInteractable = mesh;
                    interactionPrompt = prompt;
                }

                interactTarget = closestInteractable;
            }
        }

        bool keyEPressed = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
        if (!sceneEditingMode && interactTarget && keyEPressed && !lastKeyEState) {
            const std::string tag = interactTarget->getTag();
            if (tag == "computer") {
                ShowToast("Computer interaction not implemented yet.");
            } else if (tag == "locker") {
                ShowToast("Locker interaction not implemented yet.");
            }
        }
        lastKeyEState = keyEPressed;

        submarine.update(dt);

        // Rendering
        {
            int fbWidth, fbHeight;
            glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
            glfwGetWindowSize(window, &screenWidth, &screenHeight);
            
            Scene* activeScene = SceneManager::getActiveScene();
            Camera* activeCamera = scene->getActiveCamera();
            if (activeCamera) {
                activeCamera->updateProjection(fbWidth, fbHeight);
            }
            
            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
            glClear(GL_DEPTH_BUFFER_BIT);
            shadowShader.use();
            activeScene->drawShadowMap(shadowShader);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glViewport(0, 0, fbWidth, fbHeight);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            simpleShader.use();
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, depthMap);
            simpleShader.setInt("shadowMap", 4);
            activeScene->draw(simpleShader);
            
            
            // Unbind the VAO
            glBindVertexArray(0);
        }

        // UI Rendering
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)screenWidth, (float)screenHeight);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        ShowEngineUI(scene, screenWidth, screenHeight, selectedMesh, sceneEditingMode, camera);
        RenderToast(screenWidth);
        if (!sceneEditingMode) {
            RenderInteractionPrompt(screenWidth, screenHeight, interactionPrompt);
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        //swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    //cleanup GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;

}
