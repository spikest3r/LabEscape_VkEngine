#include <engine.h>
#include <iostream>
#include <scene.h>
#include "globalobjects.h"
#include "levelscenes.h"
#include <Windows.h> // TODO: ifdef here
#include "helpers.h"

PhysicsMaterial* GlobalObjects::characterMaterial;
Level1Scene* GlobalObjects::level1;
Level2Scene* GlobalObjects::level2;
Level3Scene* GlobalObjects::level3;
LevelFinalScene* GlobalObjects::levelFinal;
IntroScene* GlobalObjects::intro;
CreditsScene* GlobalObjects::credits;
UIFont UIFonts::defaultFont;
UIFont UIFonts::largeFont;
const Transform Helpers::zeroTransform = { {0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,1.0f},{0.0f,0.0f,0.0f} };

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	Engine* engine = Engine::Create();
	engine->init(800, 600, "Lab Escape");

	// Initialize globals
	GlobalObjects::characterMaterial = engine->createPhysicsMaterial(0.0f, 0.0f, 0.0f);
	ToolUI::AddFontFromFileTTF(UIFonts::defaultFont, "C:\\Windows\\Fonts\\consola.ttf", 14.0f);
	ToolUI::AddFontFromFileTTF(UIFonts::largeFont, "C:\\Windows\\Fonts\\consola.ttf", 28.0f);

	// Load scenes
	bool valid;
	GlobalObjects::level1 = engine->createScene<Level1Scene>("assets/level1.scene", &valid);
	GlobalObjects::level2 = engine->createScene<Level2Scene>("assets/level2.scene", &valid);
	GlobalObjects::level3 = engine->createScene<Level3Scene>("assets/level3.scene", &valid);
	GlobalObjects::levelFinal = engine->createScene<LevelFinalScene>("assets/level4.scene", &valid);
	GlobalObjects::intro = engine->createScene<IntroScene>();
	GlobalObjects::credits = engine->createScene<CreditsScene>();

	if (!valid) {
		// TODO: Bad scene error handler
		return -1;
	}

	engine->setCursorMode(CursorMode::DISABLED);

	engine->loadScene(GlobalObjects::intro);

	while (engine->running()) {
		engine->updateScene();
		engine->update();

		engine->render();

		if (engine->isLastFrame()) {
			auto scene = static_cast<BaseScene*>(engine->getActiveScene());
			if (scene && scene->loadNewScene) {
				scene->loadNewScene = false;
				engine->loadScene(scene->sceneToLoad);
			}
		}
	}

	engine->cleanup();
	Engine::Destroy(engine);
}