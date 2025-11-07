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
#include <thread>
#include <mutex>
#include <atomic>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "PhysicsWorld.h"
#include "Perlin.h"
#include "VoxelWorld.h"
#include "VoxelCharacterController.h"
#include "VoxelRaycast.h"
#include "BlockOutline.h"
#include "TerrainGenerator.h"
#include "CameraPathRecorder.h"

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

// THREAD-SICHERE VOXELWORLD-VERWALTUNG
std::mutex voxelWorldMutex;
std::atomic<VoxelWorld*> voxelWorld{nullptr};

VoxelCharacterController* characterController;

// Voxel lighting settings
glm::vec3 sunDirection = glm::normalize(glm::vec3(0.5f, -1.0f, 0.3f)); // Sonne scheint schräg von oben
glm::vec3 sunColor = glm::vec3(1.0f, 0.95f, 0.8f);  // Warmes Sonnenlicht
glm::vec3 ambientColor = glm::vec3(0.3f, 0.35f, 0.4f); // Bläuliches Umgebungslicht

// Block targeting
RaycastHit currentTargetBlock;
bool hasTargetBlock = false;

// Block outline renderer
BlockOutline* blockOutline = nullptr;

// Camera Path Recorder
CameraPathRecorder* cameraPathRecorder = nullptr;

// Terrain generation - THREAD-SICHER
TerrainGenerator* terrainGenerator = nullptr;
std::mutex terrainProgressMutex;
std::atomic<float> terrainGenerationProgress{0.0f};
std::string terrainGenerationMessage = "";
std::atomic<bool> terrainGenerationInProgress{false};
std::atomic<bool> terrainGenerated{false};

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
	// glEnable(GL_CULL_FACE);
	// glCullFace(GL_BACK); // Voxel-System verwendet Back-Face Culling

	// build and compile shaders
	// -------------------------
	Shader voxelShader("shaders/voxel.vert", "shaders/voxel.frag");
	Shader physicsShader("shaders/point_shadow.vert", "shaders/point_shadow.frag");
	Shader simpleDepthShader("shaders/ps_depth.vert", "shaders/ps_depth.frag", "shaders/ps_depth.geom");

	// load textures
	// -------------
	unsigned int woodTexture = loadTexture("resources/textures/wood.png");
	unsigned int voxelAtlasTexture = loadTexture("resources/textures/atlas.png");

	// configure depth map FBO (für Physics-Objekte, falls noch verwendet)
	// -----------------------
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);
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
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader configuration
	// --------------------
	physicsShader.use();
	physicsShader.setInt("diffuseTexture", 0);
	physicsShader.setInt("depthMap", 1);

	world = new PhysicsWorld();
	world->Init();

	// Initialisiere Voxel-Welt
	voxelWorld.store(new VoxelWorld(), std::memory_order_release);
	
	// Initialisiere Terrain Generator
	terrainGenerator = new TerrainGenerator();
	
	// NEUES TERRAIN-SYSTEM: Größeres Terrain mit Fortschrittsanzeige
	std::cout << "Starte Terrain-Generierung..." << std::endl;
	terrainGenerationInProgress.store(true, std::memory_order_release);
	
	TerrainConfig config;
	config.sizeX = 256;
	config.sizeZ = 256;
	config.scale = 0.03f;
	config.heightMultiplier = 30.0f;
	config.minHeight = -20;
	config.generateCaves = true;
	config.seed = 12345;
	config.octaves = 4;
	config.persistence = 0.5f;
	config.lacunarity = 2.0f;
	config.continentalnessScale = 0.01f;
	config.erosionScale = 0.03f;
	config.mountainScale = 0.02f;
	config.mountainThreshold = 0.6f;
	config.mountainHeightMultiplier = 2.5f;
	config.caveScale = 0.05f;
	config.caveThreshold = 0.55f;
	config.caveMinDepth = 5;
	config.generateBeaches = true;
	config.waterLevel = 0;
	config.numThreads = std::thread::hardware_concurrency();
	
	// Progress-Callback für UI-Updates - THREAD-SICHER
	auto progressCallback = [](float progress, const std::string& message) {
		terrainGenerationProgress.store(progress, std::memory_order_release);
		{
			std::lock_guard<std::mutex> lock(terrainProgressMutex);
			terrainGenerationMessage = message;
		}
		std::cout << "Terrain: " << (int)(progress * 100) << "% - " << message << std::endl;
	};
	
	// Generiere Terrain PARALLEL (viel schneller!)
	VoxelWorld* initialWorld = voxelWorld.load(std::memory_order_acquire);
	terrainGenerator->generateTerrainParallel(initialWorld, config, progressCallback);
	
	terrainGenerationInProgress.store(false, std::memory_order_release);
	terrainGenerated.store(true, std::memory_order_release);
	std::cout << "Terrain-Generierung abgeschlossen!" << std::endl;
	
	/* ALTE METHODE - Auskommentiert
	// Erstelle Voxel-Terrain
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
	*/

	// lighting info (für Physics-Objekte)
	glm::vec3 lightPos(10.0f, 10.0f, 10.0f);
	float near_plane = 1.0f;
	float far_plane = 25.0f;

	// Initialisiere Voxel Character Controller
	VoxelWorld* worldForController = voxelWorld.load(std::memory_order_acquire);
	characterController = new VoxelCharacterController(worldForController, window);
	
	// Initialisiere Block Outline Renderer
	blockOutline = new BlockOutline();
	blockOutline->init("shaders/outline.vert", "shaders/outline.frag");

	// Initialisiere Camera Path Recorder
	cameraPathRecorder = new CameraPathRecorder();
	cameraPathRecorder->setRecordingRate(30.0f);  // 30 Keyframes pro Sekunde
	cameraPathRecorder->setLooping(false);

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

		// Update Character Controller nur wenn NICHT im Playback
		if (!cameraPathRecorder || !cameraPathRecorder->isPlaying()) {
			characterController->update(deltaTime);
		}
		
		// Update Camera Path Recorder
		if (cameraPathRecorder) {
			if (cameraPathRecorder->isRecording() && characterController->isFreeFlyMode()) {
				// Zeichne Kameraposition auf im Free Fly Modus
				glm::vec3 camPos = characterController->getPosition() + glm::vec3(0.0f, 1.6f, 0.0f);
				cameraPathRecorder->updateRecording(camPos, characterController->getFront(), characterController->getUp(), deltaTime);
			}
			
			if (cameraPathRecorder->isPlaying()) {
				// Spiele aufgezeichneten Pfad ab
				glm::vec3 playbackPos, playbackFront, playbackUp;
				if (cameraPathRecorder->updatePlayback(playbackPos, playbackFront, playbackUp, deltaTime)) {
					// Überschreibe Kamera-Position mit Playback-Daten
					camera.Position = playbackPos;
					camera.Front = playbackFront;
					camera.Up = playbackUp;
				}
			}
		}

		// Update target block (für Visualisierung) - THREAD-SICHER
		VoxelWorld* worldForRaycast = voxelWorld.load(std::memory_order_acquire);
		if (worldForRaycast && (!cameraPathRecorder || !cameraPathRecorder->isPlaying())) {
			// Nur Target Block berechnen wenn nicht im Playback
			glm::vec3 rayOrigin = characterController->getPosition() + glm::vec3(0.0f, 1.6f, 0.0f);
			glm::vec3 rayDirection = characterController->getFront();
			currentTargetBlock = VoxelRaycast::raycast(rayOrigin, rayDirection, 5.0f, worldForRaycast);
			hasTargetBlock = currentTargetBlock.hit;
		}

		// render
		// ----__
		glClearColor(0.5f, 0.5f, 0.8f, 1.0f); // Sky color
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Überschreibe Kamera nur wenn NICHT im Playback-Modus
		if (!cameraPathRecorder || !cameraPathRecorder->isPlaying()) {
			// Update Kamera basierend auf Character Controller
			camera.Position = characterController->getPosition() + glm::vec3(0.0f, 1.6f, 0.0f);
			camera.Front = characterController->getFront();
			camera.Up = characterController->getUp();
		}
		camera.Zoom = 45.0f;
		
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 500.0f);
		glm::mat4 view = camera.GetViewMatrix();

		// === Render Voxel-Welt mit voxel-spezifischem Shader ===
		VoxelWorld* worldForRendering = voxelWorld.load(std::memory_order_acquire);
		if (worldForRendering) {
			voxelShader.use();
			voxelShader.setMat4("projection", projection);
			voxelShader.setMat4("view", view);
			voxelShader.setMat4("model", glm::mat4(1.0f));
			
			// Voxel Lighting Uniforms
			voxelShader.setVec3("sunDirection", sunDirection);
			voxelShader.setVec3("sunColor", sunColor);
			voxelShader.setVec3("ambientColor", ambientColor);
			voxelShader.setVec3("viewPos", camera.Position);
			
			// Bind Voxel Atlas Texture
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, voxelAtlasTexture);
			voxelShader.setInt("diffuseTexture", 0);
			
			worldForRendering->render();
		}
		
		// === Render Block Outlines ===
		if (hasTargetBlock && blockOutline) {
			blockOutline->renderDualOutline(
				currentTargetBlock.blockPos,   // Rot: Block zum Löschen
				currentTargetBlock.placePos,   // Grün: Position für neuen Block
				projection,
				view
			);
		}

		// === Optional: Render Physics-Objekte mit Schatten (falls noch benötigt) ===
		// Shadow-Rendering auskommentiert, da nicht mehr für Voxel benötigt
		/*
		glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
		std::vector<glm::mat4> shadowTransforms;
		shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
		// ... weitere shadow transforms ...

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

		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		physicsShader.use();
		physicsShader.setMat4("projection", projection);
		physicsShader.setMat4("view", view);
		physicsShader.setVec3("lightPos", lightPos);
		physicsShader.setVec3("viewPos", camera.Position);
		physicsShader.setInt("shadows", true);
		physicsShader.setFloat("far_plane", far_plane);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, woodTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
		renderScene(physicsShader);
		*/

		ImGui::Begin("Voxel Lighting Settings");

		if (ImGui::DragFloat3("Sun Direction", (float*)&sunDirection, 0.01f)) {
			sunDirection = glm::normalize(sunDirection);
		}

		ImGui::ColorEdit3("Sun Color", (float*)&sunColor);
		ImGui::ColorEdit3("Ambient Color", (float*)&ambientColor);
		
		ImGui::Separator();
		ImGui::Text("=== Terrain Generation ===");
		
		// Terrain Config UI
		static TerrainConfig uiConfig;
		static bool configInitialized = false;
		
		if (!configInitialized) {
			// Initialisiere mit Standard-Werten
			uiConfig.sizeX = 512;
			uiConfig.sizeZ = 512;
		 uiConfig.scale = 0.03f;
		 uiConfig.heightMultiplier = 30.0f;
		 uiConfig.minHeight = -20;
		 uiConfig.generateCaves = true;
		 uiConfig.seed = 12345;
		 uiConfig.octaves = 4;
		 uiConfig.persistence = 0.5f;
		 uiConfig.lacunarity = 2.0f;
		 uiConfig.continentalnessScale = 0.01f;
		 uiConfig.erosionScale = 0.03f;
		 uiConfig.mountainScale = 0.02f;
		 uiConfig.mountainThreshold = 0.6f;
		 uiConfig.mountainHeightMultiplier = 2.5f;
		 uiConfig.caveScale = 0.05f;
		 uiConfig.caveThreshold = 0.55f;
		 uiConfig.caveMinDepth = 5;
		 uiConfig.generateBeaches = true;
		 uiConfig.waterLevel = 0;
		 uiConfig.numThreads = std::thread::hardware_concurrency();
		 configInitialized = true;
		}
		
		if (terrainGenerationInProgress.load(std::memory_order_acquire)) {
			float progress = terrainGenerationProgress.load(std::memory_order_acquire);
			ImGui::ProgressBar(progress, ImVec2(-1, 0));
			
			std::string message;
			{
				std::lock_guard<std::mutex> lock(terrainProgressMutex);
				message = terrainGenerationMessage;
			}
			ImGui::Text("%s", message.c_str());
		} else {
			// Terrain-Parameter Collapsing Headers für bessere Übersicht
			
			if (ImGui::CollapsingHeader("Basis-Parameter", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::SliderInt("Größe X", &uiConfig.sizeX, 32, 512);
				ImGui::SliderInt("Größe Z", &uiConfig.sizeZ, 32, 512);
				ImGui::SliderFloat("Scale", &uiConfig.scale, 0.001f, 0.1f, "%.4f");
				ImGui::SliderFloat("Höhen-Multiplikator", &uiConfig.heightMultiplier, 5.0f, 100.0f);
				ImGui::SliderInt("Min-Höhe", &uiConfig.minHeight, -50, 0);
				ImGui::InputInt("Seed", &uiConfig.seed);
				if (ImGui::Button("Zufälliger Seed")) {
					uiConfig.seed = static_cast<int>(time(nullptr));
				}
			}
			
			if (ImGui::CollapsingHeader("Noise-Parameter")) {
				ImGui::SliderInt("Oktaven", &uiConfig.octaves, 1, 8);
				ImGui::SliderFloat("Persistence", &uiConfig.persistence, 0.1f, 1.0f);
				ImGui::SliderFloat("Lacunarity", &uiConfig.lacunarity, 1.5f, 4.0f);
			}
			
			if (ImGui::CollapsingHeader("Terrain-Features")) {
				ImGui::SliderFloat("Kontinental-Scale", &uiConfig.continentalnessScale, 0.001f, 0.05f, "%.4f");
				ImGui::SliderFloat("Erosion-Scale", &uiConfig.erosionScale, 0.01f, 0.1f, "%.4f");
				ImGui::Text("Berge:");
				ImGui::SliderFloat("Berg-Scale", &uiConfig.mountainScale, 0.005f, 0.05f, "%.4f");
				ImGui::SliderFloat("Berg-Schwellwert", &uiConfig.mountainThreshold, 0.3f, 0.9f);
				ImGui::SliderFloat("Berg-Höhe", &uiConfig.mountainHeightMultiplier, 1.0f, 5.0f);
			}
			
			if (ImGui::CollapsingHeader("Höhlen-Parameter")) {
				ImGui::Checkbox("Höhlen generieren", &uiConfig.generateCaves);
				if (uiConfig.generateCaves) {
					ImGui::SliderFloat("Höhlen-Scale", &uiConfig.caveScale, 0.01f, 0.1f, "%.4f");
					ImGui::SliderFloat("Höhlen-Schwellwert", &uiConfig.caveThreshold, 0.4f, 0.7f);
					ImGui::SliderInt("Min-Tiefe", &uiConfig.caveMinDepth, 1, 20);
				}
			}
			
			if (ImGui::CollapsingHeader("Erweiterte Features")) {
				ImGui::Checkbox("Strände generieren", &uiConfig.generateBeaches);
				ImGui::SliderInt("Wasser-Level", &uiConfig.waterLevel, -10, 10);
				ImGui::SliderInt("Anzahl Threads", &uiConfig.numThreads, 1, 32);
			}
			
			ImGui::Separator();
			
			// Preset-Buttons
			ImGui::Text("Presets:");
			if (ImGui::Button("Flaches Land")) {
				uiConfig.heightMultiplier = 10.0f;
				uiConfig.mountainThreshold = 0.9f;
			 uiConfig.erosionScale = 0.05f;
			}
			ImGui::SameLine();
			if (ImGui::Button("Hügelland")) {
			 uiConfig.heightMultiplier = 25.0f;
			 uiConfig.mountainThreshold = 0.7f;
			 uiConfig.mountainHeightMultiplier = 1.5f;
			}
			ImGui::SameLine();
			if (ImGui::Button("Gebirge")) {
			 uiConfig.heightMultiplier = 40.0f;
			 uiConfig.mountainThreshold = 0.5f;
			 uiConfig.mountainHeightMultiplier = 3.5f;
			}
			if (ImGui::Button("Extreme Berge")) {
			 uiConfig.heightMultiplier = 60.0f;
			 uiConfig.mountainThreshold = 0.4f;
			 uiConfig.mountainHeightMultiplier = 5.0f;
			 uiConfig.octaves = 6;
			}
			ImGui::SameLine();
			if (ImGui::Button("Inselwelt")) {
			 uiConfig.continentalnessScale = 0.005f;
			 uiConfig.heightMultiplier = 20.0f;
			 uiConfig.waterLevel = 5;
			 uiConfig.generateBeaches = true;
			}
			
			ImGui::Separator();
			
			// Generate Button
			if (ImGui::Button("Terrain Generieren", ImVec2(-1, 40))) {
				if (!terrainGenerationInProgress.load(std::memory_order_acquire)) {
					std::cout << "Starte Terrain-Regenerierung..." << std::endl;
					terrainGenerationInProgress.store(true, std::memory_order_release);
					
					// Kopiere Config für Thread
					TerrainConfig threadConfig = uiConfig;
					
					// Terrain in separatem Thread generieren
					std::thread([threadConfig]() {
						// Erstelle NEUE Welt (nicht die alte löschen!)
						VoxelWorld* newWorld = new VoxelWorld();
						
						auto progressCallback = [](float progress, const std::string& message) {
							terrainGenerationProgress.store(progress, std::memory_order_release);
							{
								std::lock_guard<std::mutex> lock(terrainProgressMutex);
								terrainGenerationMessage = message;
							}
						};
						
						// Generiere Terrain in der NEUEN Welt
						terrainGenerator->generateTerrainParallel(newWorld, threadConfig, progressCallback);
						
						// ATOMARER Austausch der Welten - SICHER!
						VoxelWorld* oldWorld = voxelWorld.exchange(newWorld, std::memory_order_acq_rel);
						
						// Warte ein bisschen, damit Render-Thread umschaltet
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
						
						// Alte Welt löschen
						if (oldWorld) {
							delete oldWorld;
						}
						
						terrainGenerationInProgress.store(false, std::memory_order_release);
						terrainGenerated.store(true, std::memory_order_release);
						std::cout << "Terrain-Regenerierung abgeschlossen!" << std::endl;
					}).detach();
				}
			}
			
			if (terrainGenerated.load(std::memory_order_acquire)) {
				ImGui::Text("Aktueller Seed: %d", terrainGenerator->getCurrentSeed());
			}
		}

		ImGui::End();

		// === Camera Path Recorder UI ===
		ImGui::Begin("Camera Path Recorder");
		
		if (cameraPathRecorder) {
			// Status anzeigen
			ImGui::Text("Status: %s", 
				cameraPathRecorder->isRecording() ? "Recording" :
				cameraPathRecorder->isPlaying() ? "Playing" : "Idle");
			
			ImGui::Separator();
			
			// Recording Controls
			ImGui::Text("=== Recording ===");
			
			if (cameraPathRecorder->isRecording()) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "REC");
				ImGui::SameLine();
				ImGui::Text("Time: %.2f s", cameraPathRecorder->getRecordingDuration());
				
				if (ImGui::Button("Stop Recording", ImVec2(-1, 30))) {
					cameraPathRecorder->stopRecording();
				}
			} else {
				bool canRecord = characterController && characterController->isFreeFlyMode();
				
				if (!canRecord) {
					ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), 
						"Enable Free Fly Mode (F) to record!");
				}
				
				ImGui::BeginDisabled(!canRecord || cameraPathRecorder->isPlaying());
				if (ImGui::Button("Start Recording", ImVec2(-1, 30))) {
					cameraPathRecorder->startRecording();
				}
				ImGui::EndDisabled();
			}
			
			// Recording Settings
			static float recordingRate = 30.0f;
			if (ImGui::SliderFloat("Recording Rate (fps)", &recordingRate, 5.0f, 120.0f)) {
				cameraPathRecorder->setRecordingRate(recordingRate);
			}
			
			ImGui::Separator();
			
			// Playback Controls
			ImGui::Text("=== Playback ===");
			
			ImGui::Text("Keyframes: %d", cameraPathRecorder->getKeyframeCount());
			ImGui::Text("Duration: %.2f seconds", cameraPathRecorder->getRecordingDuration());
			
			if (cameraPathRecorder->isPlaying()) {
				ImGui::ProgressBar(cameraPathRecorder->getPlaybackProgress(), ImVec2(-1, 0));
				ImGui::Text("Time: %.2f / %.2f s", 
					cameraPathRecorder->getPlaybackTime(), 
					cameraPathRecorder->getRecordingDuration());
				
				if (ImGui::Button("Stop Playback", ImVec2(-1, 30))) {
					cameraPathRecorder->stopPlayback();
				}
			} else {
				bool hasRecording = cameraPathRecorder->getKeyframeCount() > 0;
				
				ImGui::BeginDisabled(!hasRecording || cameraPathRecorder->isRecording());
				if (ImGui::Button("Play", ImVec2(-1, 30))) {
					cameraPathRecorder->startPlayback();
				}
				ImGui::EndDisabled();
			}
			
			// Playback Settings
			static float playbackSpeed = 1.0f;
			if (ImGui::SliderFloat("Playback Speed", &playbackSpeed, 0.1f, 5.0f, "%.2fx")) {
				cameraPathRecorder->setPlaybackSpeed(playbackSpeed);
			}
			
			static bool looping = false;
			if (ImGui::Checkbox("Loop Playback", &looping)) {
				cameraPathRecorder->setLooping(looping);
			}
			
			ImGui::Separator();
			
			// File Operations
			ImGui::Text("=== File Operations ===");
			
			static char pathFilename[256] = "camera_path.bin";
			ImGui::InputText("Filename", pathFilename, sizeof(pathFilename));
			
			if (ImGui::Button("Save Path", ImVec2(-1, 0))) {
				cameraPathRecorder->savePath(pathFilename);
			}
			
			if (ImGui::Button("Load Path", ImVec2(-1, 0))) {
				cameraPathRecorder->loadPath(pathFilename);
			}
			
			ImGui::BeginDisabled(cameraPathRecorder->isRecording() || cameraPathRecorder->isPlaying());
			if (ImGui::Button("Clear Recording", ImVec2(-1, 0))) {
				cameraPathRecorder->clearRecording();
			}
			ImGui::EndDisabled();
			
			ImGui::Separator();
			
			// Info
			if (ImGui::Button("Print Info to Console", ImVec2(-1, 0))) {
			 cameraPathRecorder->printInfo();
			}
		}
		
		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Cleanup
	if (terrainGenerator) {
		delete terrainGenerator;
		terrainGenerator = nullptr;
	}
	
	if (cameraPathRecorder) {
		delete cameraPathRecorder;
		cameraPathRecorder = nullptr;
	}
	
	if (blockOutline) {
		delete blockOutline;
		blockOutline = nullptr;
	}
	
	if (characterController) {
		delete characterController;
		characterController = nullptr;
	}
	
	VoxelWorld* worldForCleanup = voxelWorld.exchange(nullptr, std::memory_order_acq_rel);
	if (worldForCleanup) {
		delete worldForCleanup;
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
    // Toggle Free Fly Mode mit F-Taste
    static bool fKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fKeyPressed) {
    fKeyPressed = true;
        if (characterController) {
   characterController->toggleFreeFlyMode();
    
          // Cursor-Modus anpassen
   if (characterController->isFreeFlyMode()) {
  // Free Fly aktiviert - Cursor sichtbar für UI
              glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
         } else {
  // Normal Mode - Cursor versteckt
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
          }
        }
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
        fKeyPressed = false;
    }
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
        // Im Free Fly Modus nur bei gedrückter rechter Maustaste
if (characterController->isFreeFlyMode()) {
   if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
    characterController->onMouseMove(dx, dy);
      }
} else {
  // Normal Mode - immer aktiv
      characterController->onMouseMove(dx, dy);
  }
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

	// Linksklick: Block platzieren
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		VoxelWorld* worldForPlacing = voxelWorld.load(std::memory_order_acquire);
		if (worldForPlacing && characterController && hasTargetBlock) {
			// Verwende den bereits berechneten Target-Block
			worldForPlacing->setBlock(
				currentTargetBlock.placePos.x, 
				currentTargetBlock.placePos.y, 
				currentTargetBlock.placePos.z, 
				BlockType::Stone
			);
			
			std::cout << "Block platziert bei: (" << currentTargetBlock.placePos.x << ", " 
			 << currentTargetBlock.placePos.y << ", " << currentTargetBlock.placePos.z << ")" << std::endl;
		}
	}
	
	// Mittelklick: Block entfernen
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
		VoxelWorld* worldForRemoving = voxelWorld.load(std::memory_order_acquire);
		if (worldForRemoving && characterController && hasTargetBlock) {
			// Verwende den bereits berechneten Target-Block
			worldForRemoving->setBlock(
				currentTargetBlock.blockPos.x, 
				currentTargetBlock.blockPos.y, 
				currentTargetBlock.blockPos.z, 
				BlockType::Air
			);
			
			std::cout << "Block entfernt bei: (" << currentTargetBlock.blockPos.x << ", " 
			  << currentTargetBlock.blockPos.y << ", " << currentTargetBlock.blockPos.z << ")" << std::endl;
		}
	}
}

