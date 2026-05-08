#include "levelscenes.h"
#include "globalobjects.h"
#include "windows.h"

constexpr float borderSpeed = 3.0f;

std::string Level2Scene::getNoteText(std::string name) {
	if (name == "Note1") {
		std::string riddle =
			"CONFIDENTIAL\n"
			"I begin with the number of planets in our\n"
			"solar system. Then comes the atomic number of\n"
			"fluorine. I follow with the number of letters\n"
			"in the month that starts summer. After that,\n"
			"the position of the first vowel in the\n"
			"alphabet.\n"
			"The answer is the pin to exit gate.";
		return riddle;
	}
	else if (name == "Note3") {
		return "1";
	}
	else if (name == "Note4") {
		return "9";
	}
	else if (name == "Note5") {
		return "5";
	}
	else if (name == "Note6") {
		std::string hint = "Keypad to turn off cameras\n"
			"Last digit is 0\n"
			"And all digits are unique\n"
			"No need to thank me";
		return hint;
	}
	else if (name == "Note7") {
		return "Look carefully where the note with code is. (Especially near the beginning of the level)";
	}
	else if (name == "Note8") {
		return "Crouch and move slowly.\nDon't move if camera sees you (red outline).\nIf you're lucky, you'll move on and\nfind keypad (not gate one).\nWhen you enter big room turn right\nimmediately and you will see it on the wall.\nCode is there too.\nGood luck, I guess.";
	}

	return "Placeholder. Check note name!"; // placeholder to catch invalid notes
}

std::string Level2Scene::getKeypadCode(std::string kpName) {
	if (kpName == "Keypad1") {
		return "8941";
	}
	else if (kpName == "Keypad2") {
		return "9510";
	}
	else {
		throw std::exception("bad keypad!");
	}
}

Scene* Level2Scene::getNextScene() {
	// todo: next level switch
	return GlobalObjects::level3;
}

inline float length(Vector3& v) {
	return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline float lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

void Level2Scene::CheckTracking() {
	if (!trackerActive) return;
	if (!trackingPlayer) return;
	if (lost) return;

	if (!isCrouching()) {
		lost = true;
		g_engine->setCursorMode(CursorMode::NORMAL);
		trackingPlayer = false;
		ambientPlayer->playSound(gameover, 1.0f);
		if (g_engine->isDualSenseAttached()) {
			g_engine->dualsense_playHaptics(gameover, 1.0f);
			g_engine->dualsense_setLightbarColor(255, 155, 45);
		}
		return;
	}

	float dt = g_engine->getDeltaTime();

	if (stop) stopTime -= dt;
	else stopTimeout -= dt;

	if (stop) {
		// account for reaction time 
		const float reactionTime = hasController ? 0.5f : 0.7f;
		if (stopTime < stopTime_value - reactionTime) {
			Vector3 vel = controller->getVelocity();
			float l = length(vel);

			if (l > 0.02f) {
				g_engine->setCursorMode(CursorMode::NORMAL);
				lost = true;
			}

			if (!hasController) {
				crosshair->size = { 128.0f, 128.0f };
			}
		}
		else {
			if(!hasController) crosshair->size = { 0.0f,0.0f };
		}

		if (stopTime <= 0) {
			float value = dist(gen);
			stopTimeout = value;
			stop = false;

			g_engine->dualsense_setLightbarColor(idleColor.R, idleColor.G, idleColor.B);
		}

		if (borderOffset > 0) {
			borderOffset = lerp(borderOffset, 0.f, dt * borderSpeed);
		}
	}
	else {
		if (stopTimeout <= 0) {
			float value = dist(gen);
			stopTime = value;
			stopTime_value = value;
			stop = true;

			if (g_engine->isDualSenseAttached()) {
				g_engine->dualsense_playHaptics(rumble, 1.0f);
				g_engine->dualsense_setLightbarColor(255, 0, 150);
			}
		}

		if (borderOffset < 512) {
			borderOffset = lerp(borderOffset, 512.f, dt * borderSpeed);
		}
	}
}

void Level2Scene::UpdateScene(Engine* engine) {
	if (reload) {
		sceneToLoad = GlobalObjects::level2;
		loadNewScene = true;
	}

	if(!lost) PlayerScene::UpdateScene(engine);

	Vector2 extent = engine->getExtents();
	Vector2 borderSize = { extent.x + borderOffset, extent.y + borderOffset };
	Vector2 borderPos = { extent.x / 2, extent.y / 2 }; // Center of screen
	border->position = borderPos;
	border->size = borderSize;

	CheckTracking();
}

void Level2Scene::InitScene(Engine* engine) {
	PlayerScene::InitScene(engine);

	gen = std::mt19937(rd());
	dist = std::uniform_real_distribution<float>(3.f, 7.f); // 3 - 7

	beginTrigger = engine->createBoxTrigger({ -13.5f,1.2f,-0.2 }, { 10.f,1.f,10.f });
	beginTrigger->onTriggerEnter = [this](GameObject* other) {
		trackingPlayer = true;
	};

	gameover = engine->createSound("gameover", "assets/gameover.mp3", false, false);
	rumble = engine->createSound("rumbleBuzz", "assets/buzz.mp3", false, false);

	borderTex = engine->createTexture("borderTex", "assets/border.png");
	Vector2 extent = engine->getExtents();
	Vector2 borderSize = { extent.x + borderOffset, extent.y + borderOffset };
	Vector2 borderPos = { extent.x / 2, extent.y / 2 }; // Center of screen
	border = engine->createUIElement(borderTex, borderPos, borderSize);

	reload = false;
	lost = false;
	stop = false;
	trackingPlayer = false;
	trackerActive = true;
	borderOffset = 512.0f;

	hasController = engine->isDualSenseAttached();

	Keypad* kp = static_cast<Keypad*>(engine->getGameObject("Keypad2"));
	kp->OnSuccess = [this]() {
		trackerActive = false;
		stopTime = 0.0f;
		borderOffset = 512.0f;
		crosshair->size = { 128.0f, 128.0f }; // edge case handled
	};
}

void Level2Scene::DestroyScene(Engine* engine) {
	PlayerScene::DestroyScene(engine);

	engine->requestDestroyTrigger(beginTrigger);
}

void Level2Scene::UICallback(Engine* engine) {
	PlayerScene::UICallback(engine);

	if (lost) {
		Vector2 extent = engine->getExtents();
		ToolUI::SetNextWindowSize({ 300, 150 });
		ToolUI::SetNextWindowPos({ extent.x / 2 - (300/2), extent.y / 2 - (150 / 2)});
		ToolUI::Begin("You lost!");
		ToolUI::PushFont(UIFonts::largeFont);
		ToolUI::Text("Game over!");
		if (ToolUI::Button("Restart level")) {
			reload = true;
		}
		if (ToolUI::Button("Return to menu")) {
			sceneToLoad = GlobalObjects::intro;
			loadNewScene = true;
		}
		ToolUI::PopFont();
		ToolUI::End();
	}
}