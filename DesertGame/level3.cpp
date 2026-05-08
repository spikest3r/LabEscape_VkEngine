#include "levelscenes.h"
#include "globalobjects.h"
#include "helpers.h"
#include <windows.h>
#include <cmath>

void Level3Scene::EarlyInitScene(Engine* engine) {
	// initialize generators
	gen = std::mt19937(rd());
	dist = std::uniform_int_distribution<int>(0, 9);
	simonDist = std::uniform_int_distribution<int>(0, 8);
	distBall = std::uniform_int_distribution<int>(0, 2);
	distGTW = std::uniform_int_distribution<int>(0, gtwWordCount - 1);

	// generate keypad code
	char buf[8];
	int code[4];
	for (int i = 0; i < 4; i++) {
		code[i] = dist(gen);
	}
	sprintf_s(buf, 8, "%i%i%i%i", code[0], code[1], code[2], code[3]);
	OutputDebugStringA(buf);
	OutputDebugStringA("\n");
	randomKeypadCode = buf;
	for (int i = 0; i < 4; i++) {
		codeReveal[i] = '_';
	}
	codeReveal[4] = '\0';
}

void Level3Scene::InitScene(Engine* engine) {
	PlayerScene::InitScene(engine);

	success = engine->createSound("success", "assets/success.mp3", false, false);
	sfxPlayer = engine->createGameObject<GameObject>(Helpers::zeroTransform, nullptr, nullptr, GlobalObjects::characterMaterial, false);

	digitEmpty = engine->getTexture("DigitEmpty");
	for (int i = 0; i < 5; i++) {
		int idx = i + 1;
		char fileName[64];
		sprintf_s(fileName, 64, "assets/digits/%i.png", idx);
		char textureName[16];
		sprintf_s(textureName, 16, "Digit%i", idx);
		Texture* tex = engine->createTexture(textureName, fileName);
		digits[i] = tex;
	}

	// simon says
	Mesh* cubeMesh = engine->getMesh("DigitCube");
	SimonSaysCube* gridMarker = static_cast<SimonSaysCube*>(engine->getGameObject("GridMarker"));
	Vector3 gridOrigin = gridMarker->transform.position;
	Vector3 size = gridMarker->transform.scale;
	constexpr float spacing = .5f;

	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 3; x++) {
			int idx = y * 3 + x;
			if (x == 1 && y == 1) {
				gridMarker->tag = "SimonSaysCube";
				gridMarker->index = idx;
				digitCubes[idx] = gridMarker;
			}
			else {
				int ox = x - 1;
				int oy = y - 1;

				Transform t;
				t.position = gridOrigin;
				t.position.y += ox * spacing;
				t.position.z += oy * spacing;
				t.scale = size;
				t.rotation = { 0,0,0,1 };

				SimonSaysCube* cube = engine->createGameObject<SimonSaysCube>(t, cubeMesh, digitEmpty, GlobalObjects::characterMaterial, false);
				char nameBuf[16];
				sprintf_s(nameBuf, 16, "Cube%i", idx);
				cube->name = nameBuf;
				cube->tag = "SimonSaysCube";
				cube->index = idx;
				digitCubes[idx] = cube;
			}
		}
	}

	GameObject* simonSaysTable = engine->getGameObject("SimonSaysTable");

	Vector3 position = simonSaysTable->transform.position;
	simonSaysTrigger = engine->createBoxTrigger(position, { 4.0f,5.0f,10.0f });
	simonSaysTrigger->onTriggerEnter = [this](GameObject* other) {
		if (simonSaysRunning) return;
		if (simonSaysCompleted) return;

		if (other->tag != controller->tag) return;

		simonSaysRunning = true;

		GeneratePattern();
		ShowPattern();
	};

	// ball and cup
	// 2 -> 1 -> 3
	cups[0] = static_cast<CupObject*>(engine->getGameObject("Cup2"));
	cups[1] = static_cast<CupObject*>(engine->getGameObject("Cup1"));
	cups[2] = static_cast<CupObject*>(engine->getGameObject("Cup3"));
	
	for (int i = 0; i < 3; i++) {
		cups[i]->index = i;
	}

	// get ball
	ball = engine->getGameObject("Ball");

	for (int i = 0; i < 3; i++) {
		cupPosition[i] = cups[i]->transform.position;
	}

	ballScale = ball->transform.scale;

	SetBallState(false);

	position = ball->transform.position;
	ballAndCupTrigger = engine->createBoxTrigger(position, { 3.0f,5.0f,10.0f });
	ballAndCupTrigger->onTriggerEnter = [this](GameObject* other) {
		if (shuffleGameRunning) return;
		if (shuffleGameDone) return;

		if (other->tag != controller->tag) return;

		BeginShuffleGame();
	};

	// duck game
	duckSound = engine->createSound("DuckSound", "assets/duck.mp3", false, false);
	duckTexture = engine->getTexture("DuckTexture");
	activeDuckTexture = engine->createTexture("ActiveDuckTex", "assets/duck_active.png");
	GameObject* duckTable = engine->getGameObject("Table1_2");
	Vector3 duckTablePos = duckTable->transform.position;
	duckTableTrigger = engine->createBoxTrigger(duckTablePos, { 2.0f,1.0f,10.0f });
	duckTableTrigger->onTriggerEnter = [this](GameObject* other) {
		if (other->tag != "DuckObject") return;

		duckCounter++;

		other->playSound(duckSound, 1.0f);
		g_engine->dualsense_playHaptics(duckSound, 1.0f);
		
		other->updateTexture(activeDuckTexture);

		if (duckCounter == 4) {
			RevealCodeDigit(3); // last (4th) digit revealed
		}
	};
	duckTableTrigger->onTriggerExit = [this](GameObject* other) {
		if (other->tag != "DuckObject") return;

		duckCounter--;
		other->updateTexture(duckTexture);
	};
	duckTable->onCollision = [this](GameObject* other, float impulse) {
		if (other->tag != "DuckObject") return;
	};

	// default value initialization
	selectedRight = 0;
	initiatedShowBall = false;
	showedBall = false;
	cupUpWait = 0.0f;
	ballTime = 0.0f;
	cupUpAnimationStage = 0;
	isUp = false;
	awaitingCupRaycast = false;
	currentShuffleIteration = 1;
	shuffleGameDone = false;
	shuffleGameRunning = false;
	cupLerpSpeed = 0.8f;
	shuffleIterations = 3;
	ballIndex = 1;
	cupTime = 0.0f;
	isSwitchingCups = false;

	simonSaysRunning = false;
	simonSaysCompleted = false;
	memset(pattern, 0, 5 * sizeof(int));
	memset(userPattern, 0, 5 * sizeof(int));
	simonSaysInputBlocked = false;
	showingPattern = false;
	timePerBlock = 1.0f;
	patternLimit = 1;
	indexShowing = 0;
	indexTapping = 0;

	gtwActive = false;
	gtwCompleted = false;
	gtwWordIndex = distGTW(gen);
	memset(gtwUserWord, 0, sizeof(gtwUserWord));

	duckCounter = 0;
	isMovingDuck = false;
}


// simon says
void Level3Scene::GeneratePattern() {
	memset(pattern, 0, 5 * sizeof(int));
	memset(userPattern, 0, 5 * sizeof(int));

	for (int i = 0; i < patternLimit; i++) {
		pattern[i] = simonDist(gen);
	}
}

void Level3Scene::ShowPattern() {
	if (showingPattern) return;

	timePerBlock = 1.0f;
	indexShowing = 0;
	simonSaysInputBlocked = true;
	showingPattern = true;
}

void Level3Scene::raycastHandler(RaycastHit hit) {
	if (hit.object->tag == "SimonSaysCube") {
		if (!simonSaysRunning) return;
		if (simonSaysInputBlocked) return;

		hit.object->updateTexture(digits[indexTapping]);
		userPattern[indexTapping] = static_cast<SimonSaysCube*>(hit.object)->index;
		indexTapping++;
		if (indexTapping >= patternLimit) {
			simonSaysInputBlocked = true;

			// Check pattern
			for (int i = 0; i < patternLimit; i++) {
				if (userPattern[i] != pattern[i]) {
					for (auto& cube : digitCubes) {
						cube->updateTexture(digitEmpty);
					}

					ShowPattern();
					indexTapping = 0;
					return;
				}
			}

			// pattern was correct
			g_engine->addTimer(1.0f, [this]() {
				for (auto& cube : digitCubes) {
					cube->updateTexture(digitEmpty);
				}
				if (patternLimit < 5) patternLimit++;
				else {
					simonSaysCompleted = true;
					simonSaysRunning = false;
					OutputDebugStringA("Simon Says SUCCESS!\n");
					RevealCodeDigit(0); // first digit revealed
				}
				indexTapping = 0;
				GeneratePattern();
				ShowPattern();
			});
		}
	}
	else if (hit.object->tag == "CupObject" && awaitingCupRaycast) {
		CupObject* cup = static_cast<CupObject*>(hit.object);
		int cupIndex = cup->index;

		if (cupIndex == ballIndex) {
			selectedRight++;

			if (selectedRight == 10) {
				shuffleGameRunning = false;
				shuffleGameDone = true;
				RevealCodeDigit(1); // second digit revealed
			}

			// TODO: Good SFX
		}
		else {
			// TODO: Bad SFX
		}

		currentShuffleIteration = 0;
		shuffleIterations++;

		showedBall = false;
		initiatedShowBall = false;
		cupUpWait = 0.0f;

		awaitingCupRaycast = false;
	}
	else if (hit.object->tag == "WordNote") {
		if (!gtwCompleted && !gtwActive) {
			gtwActive = true;
			enforcePause = true;
			g_engine->setCursorMode(CursorMode::NORMAL);
			memset(gtwUserWord, 0, sizeof(gtwUserWord));
		}
	}
	else if (hit.object->tag == "DuckObject") {
		if (!isMovingDuck) {
			isMovingDuck = true;
			puppetDuck = hit.object;
			constexpr float maxDistance = 3.0f;
			movingDistance = hit.distance > maxDistance ? hit.distance : maxDistance;
		}
	}
}

inline float lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

inline Vector2 lerpVec2(Vector2 a, Vector2 b, float t) {
	return a + (b - a) * t;
}

float smoothstep(float t) {
	return t * t * (3.0f - 2.0f * t);
}

Vector2 arcMove(const Vector2& start, const Vector2& end, float t, float height)
{
	float u = smoothstep(t);

	Vector2 pos = lerpVec2(start, end, u);

	float arc = std::sin(u * 3.1415926f);

	pos.y -= arc * height;

	return pos;
}

inline Vector2 Vec3ToVec2(Vector3 vec) {
	return { vec.x,vec.y };
}

void Level3Scene::SwitchCups(int indexFrom, int indexTo) {
	if (isSwitchingCups) return;
	if (indexFrom > 2 || indexFrom < 0 || indexTo > 2 || indexTo < 0 || indexFrom == indexTo) return; // illegal condition
	
	cupA = cups[indexFrom];
	cupB = cups[indexTo];
	cupFrom = Vec3ToVec2(cupA->transform.position);
	cupTo = Vec3ToVec2(cupB->transform.position);
	cupTime = 0.0f;
	isSwitchingCups = true;
}

void Level3Scene::SetBallState(bool visible) {
	// update position
	Vector3 ballPos = cups[ballIndex]->transform.position;
	ball->transform.position.x = ballPos.x;

	// set size
	constexpr Vector3 zero = { 0.0f, 0.0f, 0.0f };
	ball->transform.scale = visible ? ballScale : zero;
}

void Level3Scene::UpdateScene(Engine* engine) {
	PlayerScene::UpdateScene(engine);
	float dt = engine->getDeltaTime();

	GamepadState* state = engine->getGamepad();

	if (simonSaysRunning) {
		if (showingPattern) {
			timePerBlock -= dt;
			if (timePerBlock <= 0) {
				timePerBlock = 1.0f;

				if (indexShowing >= patternLimit) {
					showingPattern = false;
					simonSaysInputBlocked = false;

					for (auto& cube : digitCubes) {
						cube->updateTexture(digitEmpty);
					}
				}
				else {
					GameObject* cube = digitCubes[pattern[indexShowing]];
					cube->updateTexture(digits[indexShowing]);

					indexShowing++;
				}
			}
		}
	}

	if (shuffleGameRunning && !awaitingCupRaycast) {
		if (!showedBall) {
			if (!initiatedShowBall) {
				SetBallState(true);
				ShowBall();
				initiatedShowBall = true;
			}
			if (!isUp && initiatedShowBall) {
				SetBallState(false);
				showedBall = true;

				if(cupLerpSpeed < 3.0f) cupLerpSpeed += 0.2f;
			}
		}
		else {
			if (!isSwitchingCups) {
				if (currentShuffleIteration > shuffleIterations) {
					awaitingCupRaycast = true;
					return;
				}

				int slotA = distBall(gen);
				int slotB;
				do {
					slotB = distBall(gen);
				} while (slotA == slotB);

				if (slotA == ballIndex) ballIndex = slotB;
				else if (slotB == ballIndex) ballIndex = slotA;

				SwitchCups(slotA, slotB);
				currentShuffleIteration++;
			}
		}
	}

	if (isSwitchingCups) {
		cupTime += dt * cupLerpSpeed;
		if (cupTime > 1.0f) {
			cupTime = 1.0f;
			isSwitchingCups = false;

			int indexA = -1;
			int indexB = -1;
			for (int i = 0; i < 3; i++) {
				if (cups[i] == cupA) indexA = i;
				if (cups[i] == cupB) indexB = i;
			}
			std::swap(cups[indexA], cups[indexB]);

			cups[indexA]->index = indexA;
			cups[indexB]->index = indexB;
		}

		Vector2 cupPos = arcMove(cupFrom, cupTo, cupTime, -0.4f);
		Vector2 cup2Pos = arcMove(cupTo, cupFrom, cupTime, 0.4f);

		cupA->transform.position.x = cupPos.x;
		cupA->transform.position.y = cupPos.y;
		cupA->updateTransform();

		cupB->transform.position.x = cup2Pos.x;
		cupB->transform.position.y = cup2Pos.y;
		cupB->updateTransform();
	}

	if (isUp) {
		ballTime += dt * 1.5f;

		if (cupUpAnimationStage == 1) {
			cupUpWait += dt;
			if (cupUpWait >= 1.f) {
				cupUpAnimationStage++;
				ballTime = 0.0f;
			}
		}
		else {
			float z = cupUpAnimationStage == 0 ?
				lerp(-1.41f, -1.0f, ballTime) : // move up
				lerp(-1.0f, -1.41f, ballTime); // move down

			for (int i = 0; i < 3; i++) {
				cups[i]->transform.position.z = z;
			}
			if (ballTime >= 1.0f) {
				cupUpAnimationStage++;
				if (cupUpAnimationStage == 3) {
					isUp = false;
				}
				ballTime = 0.0f;
			}
		}
	}

	auto keyState = state->axes[GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.5f;
	auto isRaycasting = keyState || engine->getMouseButton(MouseButton::Left) == PRESS;
	if (!isRaycasting) {
		isMovingDuck = false;
		puppetDuck = nullptr;
	}

	if (isMovingDuck && puppetDuck) {
		Vector3 forward, right;
		g_engine->getCameraVectors(forward, right);
		Vector3 currentPos = g_engine->cameraPosition;

		Vector3 target = currentPos + forward * movingDistance;

		Vector3 objectPos = puppetDuck->transform.position;
		Vector3 dir = target - objectPos;

		Vector3 vel = puppetDuck->getVelocity();

		Vector3 force = dir * 180.0f - vel * 25.0f;

		puppetDuck->applyForce(force);
	}

	if (enforcePause && engine->getKey(KeyCode::Escape) == PRESS) {
		enforcePause = false;
		gtwActive = false;
		g_engine->setCursorMode(CursorMode::DISABLED);
	}
}

void Level3Scene::ShowBall() {
	cupUpAnimationStage = 0;
	ballTime = 0.0f;
	isUp = true;
}

void Level3Scene::ShuffleBallIndex() {
	int newIndex = -1;
	do {
		newIndex = distBall(gen);
	} while (newIndex == ballIndex);
	ballIndex = newIndex;
}

void Level3Scene::BeginShuffleGame() {
	if (shuffleGameDone) return;
	if (shuffleGameRunning) return;

	shuffleGameRunning = true;
}

void Level3Scene::UICallback(Engine* engine) {
	PlayerScene::UICallback(engine);

	Vector2 extents = engine->getExtents();
	extents.y = 0;
	constexpr Vector2 windowSize = { 200,100 };
	constexpr Vector2 offset = { 48, -132 };
	Vector2 pos = extents - windowSize - offset;
	ToolUI::SetNextWindowPos(pos);
	ToolUI::SetNextWindowSize(windowSize);
	ToolUI::Begin("Code");
	char code[16];
	sprintf_s(code, 16, "%c %c %c %c", codeReveal[0], codeReveal[1], codeReveal[2], codeReveal[3]);
	ToolUI::PushFont(UIFonts::largeFont);
	ToolUI::Text(code);
	ToolUI::PopFont();
	ToolUI::End();

	if (shuffleGameRunning) {
		ToolUI::SetNextWindowPos({ 20,20 });
		ToolUI::SetNextWindowSize({ 200,100 });
		ToolUI::Begin("Shuffle Game");
		char buffer[16];
		sprintf_s(buffer, 16, "%i/10", selectedRight);
		ToolUI::PushFont(UIFonts::largeFont);
		ToolUI::Text(buffer);
		ToolUI::PopFont();
		ToolUI::End();
	}
	
	if (gtwActive) {
		ToolUI::SetNextWindowSize({ 800,300 });
		Vector2 extents = engine->getExtents(); // screen size
		ToolUI::SetNextWindowPos({ extents.x / 2 - 800 / 2, extents.y / 2 - 300 / 2 });

		ToolUI::Begin("Note");
		ToolUI::PushFont(UIFonts::largeFont);
		ToolUI::Text("Unscramble the word!");
		ToolUI::Text(gtwWordVariantsScrambled[gtwWordIndex]);
		ToolUI::TextField("", gtwUserWord, 16, true);
		if (ToolUI::Button("Try")) {
			if (!strcmp(gtwUserWord, gtwWordVariants[gtwWordIndex])) {
				gtwCompleted = true;

				RevealCodeDigit(2); // reveal 3rd digit
				
				enforcePause = false;
				gtwActive = false;
				g_engine->setCursorMode(CursorMode::DISABLED);
			}
		}
		ToolUI::SameLine();
		if (ToolUI::Button("Close")) {
			enforcePause = false;
			gtwActive = false;
			g_engine->setCursorMode(CursorMode::DISABLED);
		}
		ToolUI::PopFont();
		ToolUI::End();
	}
}

void Level3Scene::RevealCodeDigit(int index) {
	if (index > 3) return;
	if (codeReveal[index] != '_') return; // already revealed
	codeReveal[index] = randomKeypadCode[index];

	sfxPlayer->playSound(success, 1.0f);
	g_engine->dualsense_playHaptics(success, 1.0f);
}

std::string Level3Scene::getNoteText(std::string name) {
	if (name == "Note1") {
		return "Find all duck statues and place them\non the table to reveal the digit of the code.";
	}
	
	return "Placeholder. Check note name!"; // placeholder to catch invalid notes
}

// one per scene
std::string Level3Scene::getKeypadCode(std::string kpName) {
	return randomKeypadCode;
}

Scene* Level3Scene::getNextScene() {
	return GlobalObjects::levelFinal;
}

void Level3Scene::DestroyScene(Engine* engine) {
	PlayerScene::DestroyScene(engine);

	engine->requestDestroyTrigger(simonSaysTrigger);
	engine->requestDestroyTrigger(ballAndCupTrigger);
	engine->requestDestroyTrigger(duckTableTrigger);
}