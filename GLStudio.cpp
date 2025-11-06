#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader.h>
#include <camera.h>
#include <model.h>

#include <iostream>
#include <filesystem>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "PhysicsWorld.h"
#include "Perlin.h"
#include "VoxelWorld.h"
#include "VoxelCharacterController.h"


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
void renderScene(const Shader& shader);
void renderCube();

// settings
const unsigned int SCR_WIDTH = 3440;
const unsigned int SCR_HEIGHT = 1440;
bool shadows = true;
bool shadowsKeyPressed = false;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool navigate_mouse = false;

PhysicsWorld* world;
Model* model;

VoxelWorld* voxelWorld;
VoxelCharacterController* characterController;

void createTerrainLandscape(PhysicsWorld* world, int size, float scale, float heightMultiplier) {
	Perlin perlin;

	for (int x = -size; x < size; x++) {
		for (int z = -size; z < size; z++) {
			float y = perlin.noise3D((float)x * scale, 0.0f, (float)z * scale) * heightMultiplier;
			world->CreateStaticBoxBody((float)x, y - 2.5f, (float)z, 1.0f);
		}
	}
}

void createVoxelTerrain(VoxelWorld* voxelWorld, int size, float scale, float heightMultiplier) {
	Perlin perlin;

	for (int x = -size; x < size; x++) {
		for (int z = -size; z < size; z++) {
			float height = perlin.noise3D((float)x * scale, 0.0f, (float)z * scale) * heightMultiplier;
			int maxY = static_cast<int>(height);
			
			// Generiere Terrain-Schichten
			for (int y = -5; y <= maxY; y++) {
				BlockType blockType;
				
				if (y == maxY && maxY > 0) {
					blockType = BlockType::Grass; // Oberste Schicht: Gras
				} else if (y > maxY - 3 && y < maxY) {
					blockType = BlockType::Dirt; // Erd-Schichten
				} else {
					blockType = BlockType::Stone; // Tiefe Schichten: Stein
				}
				
				voxelWorld->setBlock(x, y, z, blockType);
			}
		}
	}
}

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);


	// Setup Dear ImGui style
	ImGui::StyleColorsClassic();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	bool show_demo_window = true;

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	// build and compile shaders
	// -------------------------
	Shader shader("shaders/point_shadow.vert", "shaders/point_shadow.frag");
	Shader simpleDepthShader("shaders/ps_depth.vert", "shaders/ps_depth.frag", "shaders/ps_depth.geom");

	// load textures
	// -------------
	unsigned int woodTexture = loadTexture("resources/textures/wood.png");

	// configure depth map FBO
	// -----------------------
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);
	// create depth cubemap texture
	unsigned int depthCubemap;
	glGenTextures(1, &depthCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
	for (unsigned int i = 0; i < 6; ++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// shader configuration
	// --------------------
	shader.use();
	shader.setInt("diffuseTexture", 0);
	shader.setInt("depthMap", 1);

	world = new PhysicsWorld();
	world->Init();

	//world->CreateBoxBody(1, 1, 1, 1, 0, 0, 0, 0);
	//world->CreateBoxBody(2, 2, 1, 1, 0, 0, 0, 0);

	// create a perlin noise landscape using boxes and perlin noise 3D

	Perlin perlin;

	int size = 30;

	// createTerrainLandscape(world, size, 0.1f, 5.0f);
	
	// Initialisiere Voxel-Welt
	voxelWorld = new VoxelWorld();
	
	// Erstelle Voxel-Terrain (Alternative zur Physics-Welt)
	createVoxelTerrain(voxelWorld, 16, 0.1f, 8.0f);
	
	// Beispiel: Erstelle eine kleine Test-Struktur
	for (int x = 0; x < 8; x++) {
		for (int z = 0; z < 8; z++) {
			voxelWorld->setBlock(x, 0, z, BlockType::Grass);
			voxelWorld->setBlock(x, -1, z, BlockType::Dirt);
			voxelWorld->setBlock(x, -2, z, BlockType::Stone);
		}
	}
	
	// Aktualisiere alle Chunk-Meshes
	voxelWorld->updateAllChunks();


	// lighting info
	// -------------
	glm::vec3 lightPos(10.0f, 10.0f, 10.0f);

	float near_plane = 1.0f;
	float far_plane = 25.0f;

	Camera camera;
	// Initialisiere Voxel Character Controller
	characterController = new VoxelCharacterController(voxelWorld, window);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Update Character Controller
		characterController->update(deltaTime);
		
		// move light position over time
		// lightPos.z = static_cast<float>(sin(glfwGetTime() * 0.5) * 3.0);

		// render
		// ------
		glClearColor(0.5f, 0.5f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 0. create depth cubemap transformation matrices
		// -----------------------------------------------
		glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
		std::vector<glm::mat4> shadowTransforms;
		shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
		shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
		shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
		shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
		shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
		shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

		// 1. render scene to depth cubemap
		// --------------------------------
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		simpleDepthShader.use();
		for (unsigned int i = 0; i < 6; ++i)
			simpleDepthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
		simpleDepthShader.setFloat("far_plane", far_plane);
		simpleDepthShader.setVec3("lightPos", lightPos);
		renderScene(simpleDepthShader);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 2. render scene as normal 
		// -------------------------
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Update Kamera basierend auf Character Controller (ZUERST!)
		camera.Position = characterController->getPosition() + glm::vec3(0.0f, 1.6f, 0.0f); // Augenhöhe
		camera.Front = characterController->getFront();
		camera.Up = characterController->getUp();
		camera.Zoom = 45.0f;
		
		shader.use();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		shader.setMat4("projection", projection);
		shader.setMat4("view", view);
		// set lighting uniforms
		shader.setVec3("lightPos", lightPos);
		shader.setVec3("viewPos", camera.Position);
		shader.setInt("shadows", shadows); // enable/disable shadows by pressing 'SPACE'
		shader.setFloat("far_plane", far_plane);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, woodTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
		renderScene(shader);

		// Render Voxel-Welt (mit Identitäts-Model-Matrix)
		if (voxelWorld) {
			shader.use();
			glm::mat4 model = glm::mat4(1.0f); // Identitätsmatrix
			shader.setMat4("model", model);
			voxelWorld->render();
		}

		ImGui::Begin("Settings");


		if (ImGui::DragFloat3("Light position", (float*)&lightPos)) {

		}

		if (ImGui::DragFloat("Near plane", &near_plane)) {

		}

		if (ImGui::DragFloat("Far plane", &far_plane)) {

		}

		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();

	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Cleanup
	if (characterController) {
		delete characterController;
		characterController = nullptr;
	}
	
	if (voxelWorld) {
		delete voxelWorld;
		voxelWorld = nullptr;
	}
	
	if (world) {
		delete world;
		world = nullptr;
	}

	glfwTerminate();
	return 0;
}

// renders the 3D scene
// --------------------
void renderScene(const Shader& shader)
{
	// room cube
	
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0, -5, 0));
	model = glm::scale(model, glm::vec3(10,0.01f,10));

	shader.setMat4("model", model);
	glDisable(GL_CULL_FACE); // note that we disable culling here since we render 'inside' the cube instead of the usual 'outside' which throws off the normal culling methods.
	// shader.setInt("reverse_normals", 1); // A small little hack to invert normals when drawing cube from the inside so lighting still works.
	renderCube();
	// shader.setInt("reverse_normals", 0); // and of course disable it
	glEnable(GL_CULL_FACE);
	// cubes

	for (int j = world->GetDynamics()->getNumCollisionObjects() - 1; j >= 0; j--)
	{
		btCollisionObject* obj = world->GetDynamics()->getCollisionObjectArray()[j];

		if (obj->getCollisionShape()->getShapeType() != BOX_SHAPE_PROXYTYPE)
		{
			continue;
		}

		btRigidBody* body = btRigidBody::upcast(obj);
		btTransform trans;
		if (body && body->getMotionState())
		{
			body->getMotionState()->getWorldTransform(trans);
		}
		else
		{
			trans = obj->getWorldTransform();

		}
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(float(trans.getOrigin().getX()), float(trans.getOrigin().getY()), float(trans.getOrigin().getZ())));


		float roll;
		float pitch;
		float yaw;
		trans.getRotation().getEulerZYX(roll, pitch, yaw);


		model = glm::rotate(model, yaw, glm::vec3(1, 0, 0));
		model = glm::rotate(model, pitch, glm::vec3(0, 1, 0));
		model = glm::rotate(model, roll, glm::vec3(0, 0, 1));
		model = glm::scale(model, glm::vec3(0.5f));
		shader.setMat4("model", model);
		renderCube();
		// printf("world pos object %d = %f,%f,%f\n", j, float(trans.getOrigin().getX()), float(trans.getOrigin().getY()), float(trans.getOrigin().getZ()));
	}



}

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			 // bottom face
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			  1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 // top face
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			  1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			  1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);
	static double lastX = xpos, lastY = ypos;
	double dx = xpos - lastX;
	double dy = ypos - lastY;
	lastX = xpos; 
	lastY = ypos;
	
	if (characterController) {
		characterController->onMouseMove(dx, dy);
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{

}

unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
		format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		navigate_mouse = true;
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
		navigate_mouse = false;

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		// Platziere Block in Voxel-Welt
		if (voxelWorld && characterController) {
			glm::vec3 pos = characterController->getPosition();
			glm::vec3 front = characterController->getFront();
			
			glm::vec3 blockPos = pos + front * 3.0f;
			voxelWorld->setBlock(
				static_cast<int>(std::round(blockPos.x)),
				static_cast<int>(std::round(blockPos.y)),
				static_cast<int>(std::round(blockPos.z)),
				BlockType::Stone
			);
		}
	}
	
	// Mittelklick: Block entfernen
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
		if (voxelWorld && characterController) {
			glm::vec3 pos = characterController->getPosition();
			glm::vec3 front = characterController->getFront();
			
			glm::vec3 blockPos = pos + front * 3.0f;
			voxelWorld->setBlock(
				static_cast<int>(std::round(blockPos.x)),
				static_cast<int>(std::round(blockPos.y)),
				static_cast<int>(std::round(blockPos.z)),
				BlockType::Air
			);
		}
	}
}

