#include "levelscenes.h"
#include "globalobjects.h"
#include "helpers.h"

Vector3 LevelFinalScene::getSpawnPos() {
	Vector3 spawnPos = { 30.5f, -38.1f, 12.5f };
	return spawnPos;
}

void LevelFinalScene::InitScene(Engine* engine) {
	PlayerScene::InitScene(engine);

	GameObject* triggerObject = engine->getGameObject("Finish");
	endTrigger = engine->createBoxTrigger(triggerObject->transform.position, { 5.0f,5.0f,10.0f });
	endTrigger->onTriggerEnter = [this](GameObject* other) {
		if (other->tag != controller->tag) return;

		ambientPlayer->playSound(success, 1.0f);
		g_engine->dualsense_playHaptics(success, 1.0f);

		g_engine->addTimer(2.0f, [this]() {
			sceneToLoad = GlobalObjects::credits;
			loadNewScene = true;
		});
	};

	ambientPlayer = engine->createGameObject<GameObject>(Helpers::zeroTransform, nullptr, nullptr, GlobalObjects::characterMaterial, false);
	ambient = engine->createSound("AmbientSFX", "assets/NatureSFX.wav", true, false);
	ambientPlayer->playSound(ambient, 1.0f);

	success = engine->createSound("Success", "assets/success.mp3", false, false);

	engine->dualsense_setLightbarColor(0, 255, 150);
	engine->setClearColor({ 0.122, 0.467, 0.706 });
}

void LevelFinalScene::DestroyScene(Engine* engine) {
	engine->requestDestroyTrigger(endTrigger);
	engine->setClearColor({ 0.0f,0.0f,0.0f });
}

void LevelFinalScene::UICallback(Engine* engine) {
	Vector2 size = { 800, 200 };
	constexpr float verticalOffset = 32.0f;
	Vector2 extents = engine->getExtents();
	Vector2 pos = { extents.x / 2 - size.x / 2, extents.y - size.y - verticalOffset };
	ToolUI::SetNextWindowPos(pos);
	ToolUI::SetNextWindowSize(size);
	ToolUI::Begin("Congratulations!");
	ToolUI::PushFont(UIFonts::largeFont);
	ToolUI::Text("Oh my God! You managed to escape the\nlab! You're out now. And that's the end\nhere, unfortunately... Step on the black platform\nto finish. Or roam around if you\nwant, but you won't find anything interesting here!");
	ToolUI::PopFont();
	ToolUI::End();
}