#include "basescene.h"
#include "globalobjects.h"
#include <iostream>
#include "helpers.h"
#include "noteobject.h"
#include "keypad.h"
#include <engine_tool_ui.h>
#include <engine.h>
#include <Windows.h>
#include <algorithm>
#include <chrono>
#include "level3objects.h"

constexpr float runFactor = 1.5f;
constexpr float crouchFactor = 4.0f;

Vector3 PlayerScene::getSpawnPos() {
	Vector3 spawnPos = { -13.5f, -4.2f, 0.1f };
	return spawnPos;
}

void PlayerScene::InitScene(Engine* engine) {
	Vector3 spawnPos = getSpawnPos();
	controller = engine->createCharacterController(0.4f, .8f, spawnPos, GlobalObjects::characterMaterial, true);

	step1 = engine->createSound("step1", "assets/step1.wav", false, false);
	step2 = engine->createSound("step2", "assets/step2.wav", false, false);
	
	LeftDoor = engine->getGameObject("DoorLeft");
	RightDoor = engine->getGameObject("DoorRight");

	sfx_keypadClick = engine->createSound("sfx_keypadClick", "assets/keypad/sfx_keypadClick.wav", false, false);
	sfx_keypadGranted = engine->createSound("sfx_keypadGranted", "assets/keypad/sfx_keypadGranted.wav", false, false);
	sfx_keypadDenied = engine->createSound("sfx_keypadDenied", "assets/keypad/sfx_keypadDenied.wav", false, false);
	sfx_keypadHapticClick = engine->createSound("sfx_keypadHapticClick", "assets/keypad/haptic_click.wav", false, false);

	noteSfx = engine->createSound("noteSfx", "assets/note.mp3", false, false);

	if (LeftDoor && RightDoor) {
		ambientSfx = engine->createSound("ambient", "assets/Sfx.wav", true, false);
		ambientPlayer = engine->createGameObject<GameObject>(Helpers::zeroTransform, nullptr, nullptr, GlobalObjects::characterMaterial, false);
		ambientPlayer->playSound(ambientSfx, 0.5f);
	}

	engine->SetUICallback([this](Engine* engine)
		{
			this->UICallback(engine);
		});

	engine->setLightPosition({ 0.3f,5.0f,0.0f });

	if (LeftDoor && RightDoor) {

		LeftDoor->setPhysicsType(PhysicsType::Kinematic);
		RightDoor->setPhysicsType(PhysicsType::Kinematic);

		LeftDoor->updateTransform();
		RightDoor->updateTransform();

		GameObject* exit = engine->getGameObject("Exit1");
		Vector3 pos = exit->transform.position;
		pos.z -= pos.z;
		exitTrigger = engine->createBoxTrigger(pos, Vector3(2, 5, 10));
		exitTrigger->onTriggerEnter = [this](GameObject* other) {
			PlayDoorAnim();
			LaunchTransition();
			OutputDebugStringA("Playing anim\n");
			};
	}

	engine->dualsense_setLightbarColor(idleColor.R, idleColor.G, idleColor.B);

	crosshairTex = engine->createTexture("Crosshair_03", "assets/Crosshair_03.png"); // 512x512
	crosshair = engine->createUIElement(crosshairTex, { 0,0 }, { 128,128 });

	engine->setCursorMode(CursorMode::DISABLED);

	g_engine = engine;
}

void PlayerScene::UICallback(Engine* engine) {
	if (pauseMenu) {
		ToolUI::SetNextWindowPos({ 30,30 });
		ToolUI::SetNextWindowSize({ 200,200 });
		ToolUI::Begin("Pause menu");
		ToolUI::PushFont(UIFonts::largeFont);
		if (ToolUI::Button("Continue")) {
			pauseMenu = false;
			engine->setCursorMode(CursorMode::DISABLED);
		}
		if (ToolUI::Button("Restart")) {
			pauseMenu = false;
			sceneToLoad = engine->getActiveScene();
			loadNewScene = true;
		}
		if (ToolUI::Button("Exit")) {
			pauseMenu = false;
			sceneToLoad = GlobalObjects::intro;
			loadNewScene = true;
		}
		ToolUI::PopFont();
		ToolUI::End();
		return;
	}

	if (openNote && noteToShow) {
		ToolUI::SetNextWindowSize({ 800,300 });
		Vector2 extents = engine->getExtents(); // screen size
		ToolUI::SetNextWindowPos({ extents.x / 2 - 800 / 2, extents.y / 2 - 300 / 2 });

		ToolUI::Begin("Note");
		ToolUI::PushFont(UIFonts::largeFont);
		ToolUI::Text(noteToShow->text.c_str());
		ToolUI::PopFont();
		ToolUI::End();
	}

	if (openKeypad && keypadToShow) {
		ToolUI::SetNextWindowSize({ 200,320 });
		Vector2 extents = engine->getExtents(); // screen size
		ToolUI::SetNextWindowPos({ extents.x / 2 - 200 / 2, extents.y / 2 - 320 / 2 });

		constexpr float btnWidth = 50;
		constexpr float btnHeight = 50;

		ToolUI::Begin("Keypad");
		ToolUI::PushFont(UIFonts::largeFont);
		char code_buf[16];
		sprintf_s(code_buf, 16, "Code: %s", codeInputBuffer);
		ToolUI::Text(code_buf);
		for (int y = 0; y < 3; y++) {
			for (int x = 0; x < 3; x++) {
				int idx = y * 3 + x;
				idx++;
				char buf[4];
				sprintf_s(buf, 4, "%i", idx);
				if (ToolUI::Button(buf, { btnWidth, btnHeight })) {
					handleKeypadNumKey(idx);
				}
				if (x + 1 != 3) ToolUI::SameLine();
			}
		}
		if (ToolUI::Button("0", { btnWidth, btnHeight })) {
			handleKeypadNumKey(0);
		}
		ToolUI::SameLine();
		if (ToolUI::Button("Enter", { btnWidth * 2 + 8, btnHeight })) {
			if (std::string(codeInputBuffer) == keypadToShow->code) {
				// trigger success
				keypadToShow->playSound(sfx_keypadGranted, 1.0f);
				if (engine->isDualSenseAttached()) {
					engine->dualsense_playHaptics(sfx_keypadGranted, 1.0f);
				}
				keypadToShow->used = true;
				pushDSColor({ 0,255,0 }, 1.0f);
				if (keypadToShow->OnSuccess) {
					keypadToShow->OnSuccess();
				}
			}
			else {
				// trigger error
				keypadToShow->playSound(sfx_keypadDenied, 1.0f);
				if (engine->isDualSenseAttached()) engine->dualsense_playHaptics(sfx_keypadDenied, 1.0f);
				pushDSColor({ 255,0,0 }, 1.0f);
			}
			closePopup();
		}
		ToolUI::PopFont();
		ToolUI::End();
	}

	// TODO: Make toggleable
	return;

	ToolUI::Begin("Engine Monitor");

	auto vramData = engine->getVRAMStats();
	for (const auto& heap : vramData) {
		char buffer1[128];
		sprintf_s(buffer1, 128, "VRAM Heap %u", heap.heapIndex);
		ToolUI::Text(buffer1);
		ToolUI::ProgressBar(heap.usageMB / heap.budgetMB, Vector2(0.0f, 0.0f));
		ToolUI::SameLine();
		char buffer2[128];
		sprintf_s(buffer2, 128, "%.1f / %.1f MB", heap.usageMB, heap.budgetMB);
		ToolUI::Text(buffer2);
	}

	ToolUI::End();
}

void PlayerScene::closePopup() {
	closePopupFlag = true;
}

void PlayerScene::handleKeypadNumKey(int idx) {
	if (!keypadToShow) return;
	keypadToShow->playSound(sfx_keypadClick, 1.0f);
	if (g_engine->isDualSenseAttached()) g_engine->dualsense_playHaptics(sfx_keypadHapticClick, 1.0f);
	if (strlen(codeInputBuffer) < keypadToShow->length) {
		char temp[16];
		sprintf_s(temp, 16, "%s%i", codeInputBuffer, idx);
		memcpy(codeInputBuffer, temp, sizeof(temp));
	}
}

void PlayerScene::DestroyScene(Engine* engine) {
	doorAnimRunning = false;

	engine->dualsense_setLightbarColor(0, 0, 0);

	// sounds, textures, meshes and gameobjets are cleaned up automatically
	engine->requestDestroyCharacterController(controller);
	engine->requestDestroyTrigger(exitTrigger);
}

void PlayerScene::checkKeyboard(Engine* engine) {
	float dt = engine->getDeltaTime();

	Vector3 pos = controller->getPosition();

	// Mouse Look
	Vector2 mouse = engine->getMousePos();
	if (firstMouse) {
		lastX = mouse.x; lastY = mouse.y;
		if (lastX != 0.0f || lastY != 0.0f) firstMouse = false;
	}

	if (!firstMouse) {
		float dx = lastX - mouse.x;
		float dy = lastY - mouse.y;
		lastX = mouse.x; lastY = mouse.y;

		yaw += dx * mouseSensitivity;
		pitch += dy * mouseSensitivity;
		if (pitch > 89.0f) pitch = 89.0f;
		if (pitch < -89.0f) pitch = -89.0f;

		engine->cameraRotation.x = pitch;
		engine->cameraRotation.y = yaw;
	}

	// Movement
	Vector3 forward, right;
	engine->getCameraVectors(forward, right);
	Vector3 wishDir = { 0.0f, 0.0f, 0.0f };

	if (engine->getKey(KeyCode::W) == PRESS) wishDir += forward;
	if (engine->getKey(KeyCode::S) == PRESS) wishDir -= forward;
	if (engine->getKey(KeyCode::A) == PRESS) wishDir -= right;
	if (engine->getKey(KeyCode::D) == PRESS) wishDir += right;

	float moveSpeed = baseMoveSpeed;
	if (engine->getKey(KeyCode::LeftControl) == PRESS) moveSpeed *= runFactor;
	else if (engine->getKey(KeyCode::LeftShift) == PRESS) {
		moveSpeed /= crouchFactor;
		crouching = true;
	}
	else crouching = false;

	controller->Move(wishDir, moveSpeed, dt);
}

void PlayerScene::checkGamepad(Engine* engine) {
	float dt = engine->getDeltaTime();

	GamepadState* state = engine->getGamepad();

	Vector3 forward, right;

	float lookX = state->axes[GAMEPAD_AXIS_RIGHT_X];
	float lookY = state->axes[GAMEPAD_AXIS_RIGHT_Y];

	const float lookDeadzone = 0.12f;
	if (abs(lookX) < lookDeadzone) lookX = 0.0f;
	if (abs(lookY) < lookDeadzone) lookY = 0.0f;

	float stickSensitivity = 120.0f;

	yaw -= lookX * stickSensitivity * dt;
	pitch -= lookY * stickSensitivity * dt;

	// clamp pitch
	if (pitch > 89.0f)  pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;

	// apply camera rotation
	engine->cameraRotation.x = pitch;
	engine->cameraRotation.y = yaw;

	// update camera basis vectors
	engine->getCameraVectors(forward, right);

	float moveX = state->axes[GAMEPAD_AXIS_LEFT_X];
	float moveY = state->axes[GAMEPAD_AXIS_LEFT_Y];

	const float deadzone = 0.15f;
	if (abs(moveX) < deadzone) moveX = 0.0f;
	if (abs(moveY) < deadzone) moveY = 0.0f;

	Vector3 wishDir = { 0, 0, 0 };
	wishDir += forward * -moveY;
	wishDir += right * moveX;

	float moveSpeed = baseMoveSpeed;
	if (state->buttons[GAMEPAD_BUTTON_LEFT_BUMPER] && !crouching2 && !crouching) moveSpeed *= runFactor;
	else if (state->buttons[GAMEPAD_BUTTON_RIGHT_BUMPER]) { 
		moveSpeed /= crouchFactor;
		crouching2 = true;
	}
	else crouching2 = false;

	// apply movement to character controller
	controller->Move(wishDir, moveSpeed, dt);
}

void PlayerScene::processMovement(Engine* engine) {
	float dt = engine->getDeltaTime();
	float currentVel = controller->getVerticalVelocity();

	const float velThreshold = 0.5f;

	if (abs(currentVel) < velThreshold) {
		if (!isGrounded) isGrounded = true;
	}
	else {
		isGrounded = false;
	}

	lastVerticalVel = currentVel;

	float targetZ = -0.65f;
	Vector3 currentPos = engine->cameraPosition;
	if (isGrounded) {
		if (!initialized)
		{
			prevCameraPos = currentPos;
			initialized = true;
		}

		// --- velocity from camera position ---
		Vector3 velocity = (currentPos - prevCameraPos) / dt;

		// ignore Z (up axis)
		float speed = sqrt(velocity.x * velocity.x + velocity.y * velocity.y);

		// normalize speed
		float speedFactor = speed / bobbingMaxSpeed;
		if (speedFactor > 1.0f) speedFactor = 1.0f;

		// --- update bob time ---
		if (speed > 0.05f)
		{
			bobTime += dt * bobbingFrequency * speedFactor;
		}

		// --- compute bob ---
		float bob = sin(bobTime);
		float bobOffset = bob * bobbingAmplitude * speedFactor;

		// --- target Z ---
		targetZ += bobbingBaseZ;
		if (speed > 0.05f)
			targetZ += bobOffset;
	}
	else {
		targetZ = 0.0f;
	}

	// --- smooth apply to cameraOffset.z ---
	engine->cameraOffset.z += (targetZ - engine->cameraOffset.z) * dt * bobbingSmoothness;

	float cosBob = cos(bobTime);
	if (cosBob < 0 && previousCosBob >= 0) {
		controller->playSound(stepCount ? step2 : step1, 1.0f);
		engine->dualsense_playHaptics(stepCount ? step2 : step1, 1.0f);
		stepCount = !stepCount;
	}
	previousCosBob = cosBob;

	prevCameraPos = currentPos;
}

void PlayerScene::UpdateScene(Engine* engine) {
	float dt = engine->getDeltaTime();

	Vector2 screenSize = engine->getExtents();
	crosshair->position = { screenSize.x / 2, screenSize.y / 2 };

	GamepadState* state = engine->getGamepad();

	auto escKey = engine->getKey(KeyCode::Escape) == PRESS;
	auto menuKey = state->buttons[GAMEPAD_BUTTON_START];
	if ((escKey || menuKey) && !openNote && !openKeypad && !escape && !enforcePause) {
		if (!pauseKey) {
			pauseKey = true;

			pauseMenu = !pauseMenu;
			engine->setCursorMode(pauseMenu ? CursorMode::NORMAL : CursorMode::DISABLED);
		}
	}
	else {
		escape = escKey || menuKey;
		pauseKey = false;
	}

	if (!openNote && !openKeypad && !pauseMenu && !enforcePause) {
		checkKeyboard(engine);
		checkGamepad(engine);

		// Sync camera to controller
		Vector3 cPos = controller->getPosition();
		if (isCrouching()) cPos.z -= 0.5f;
		engine->cameraPosition = { cPos.x, cPos.y, cPos.z + 1.7f };

		GamepadState* state = engine->getGamepad();

		auto keyState = state->axes[GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.5f;
		if (keyState || engine->getMouseButton(MouseButton::Left) == PRESS) {
			if (!raycastKey) {
				raycastKey = true;
				float maxDistance = 10.0f;
				Vector3 rayOrigin = engine->cameraPosition;
				rayOrigin.z -= 0.6f;
				Vector3 forward, right;
				engine->getCameraVectors(forward, right);
				Vector3 rayDir = forward;

				RaycastHit hit = engine->raycast(rayOrigin, rayDir, maxDistance);

				RayDebug dbg;
				dbg.origin = rayOrigin;
				dbg.hit = hit.object;

				if (dbg.hit)
					dbg.hitOrEnd = hit.object->transform.position;
				else
					dbg.hitOrEnd = rayOrigin + rayDir * maxDistance;

				engine->pushRayDebug(dbg);

				if (hit.object) {
					char buffer[512];
					sprintf_s(buffer, "%s %s %d\n",
						hit.object->tag.c_str(),
						hit.object->name.c_str(),
						hit.object->getID()
					);

					OutputDebugStringA(buffer);

					if (hit.object->tag == "NoteObject") {
						openNote = true;
						noteToShow = static_cast<NoteObject*>(hit.object);
						noteToShow->playSound(noteSfx, 1.0f);
						engine->dualsense_playHaptics(noteSfx, 1.0f);
					}
					else
						if (hit.object->tag == "Keypad") {
							auto kp = static_cast<Keypad*>(hit.object);
							if (!kp->used) {
								openKeypad = true;
								keypadToShow = kp;
								memset(codeInputBuffer, 0, 16);
								engine->setCursorMode(CursorMode::NORMAL);
							}
						}
						else {
							raycastHandler(hit);
						}
				}
			}
		}
		else {
			raycastKey = false;
		}

		processMovement(engine);
	}
	else {
		firstMouse = true; // lock in place
		auto kb = engine->getKey(KeyCode::Escape) == PRESS;
		escape = kb;
		auto gp = state->buttons[GAMEPAD_BUTTON_CIRCLE];
		if (openKeypad || openNote) {
			if (kb || gp || closePopupFlag) {
				openNote = false;
				noteToShow = nullptr;

				openKeypad = false;
				keypadToShow = nullptr;

				engine->setCursorMode(CursorMode::DISABLED);

				closePopupFlag = false;
			}
		}
	}

	TickDoorAnimation(dt);
	TickDSColor(dt);
	TickTransition(dt);
}

inline float lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

GameObject* PlayerScene::CreateGameObject(Engine* engine, const char* objectType, const char* tag, const char* name, Transform transform, Mesh* mesh, Texture* texture, bool dynamic) {
	std::string finalTag = tag;

	GameObject* object;
	
	// TODO: Proper default material please
	if(!strcmp(objectType, "NoteObject")) {
		finalTag = "NoteObject";
		object = engine->createGameObject<NoteObject>(transform, mesh, texture, GlobalObjects::characterMaterial, dynamic);
		NoteObject* note = static_cast<NoteObject*>(object);
		note->text = getNoteText(name);
		object->tag = "NoteObject";
	}
	else if (!strcmp(objectType, "Keypad")) {
		finalTag = "Keypad";
		object = engine->createGameObject<Keypad>(transform, mesh, texture, GlobalObjects::characterMaterial, false);
		Keypad* kp = static_cast<Keypad*>(object);
		kp->code = getKeypadCode(name);
		if (!strcmp(name, "Keypad1")) {
			kp->OnSuccess = [this]() {
				PlayDoorAnim();
			};
		}
		object->tag = finalTag;
	}
	else if (!strcmp(objectType, "SimonSaysCube")) {
		object = engine->createGameObject<SimonSaysCube>(transform, mesh, texture, GlobalObjects::characterMaterial, false);
	}
	else if (!strcmp(objectType, "CupObject")) {
		object = engine->createGameObject<CupObject>(transform, mesh, texture, GlobalObjects::characterMaterial, false);
		object->tag = "CupObject";
	}
	else {
		object = engine->createGameObject<GameObject>(transform, mesh, texture, GlobalObjects::characterMaterial, dynamic);
		object->tag = finalTag;
	}

	object->name = name;
	return object;
}

std::string PlayerScene::getNoteText(std::string name) {
	return "Placeholder";
}

std::string PlayerScene::getKeypadCode(std::string name) {
	return "0000"; // placeholder
}

Scene* PlayerScene::getNextScene() {
	throw std::exception("getNextScene() Unimplemented function!");
}

// Replace PlayDoorAnim
void PlayerScene::PlayDoorAnim() {
	if (doorAnimRunning) return;
	doorAnimRunning = true;

	bool opening = !doorsUnlocked;
	doorsUnlocked = opening;

	doorLeftStart = LeftDoor->transform.position;
	doorRightStart = RightDoor->transform.position;
	doorLeftTarget = doorLeftStart;
	doorRightTarget = doorRightStart;

	float openDistance = -5.0f;
	if (opening) {
		doorLeftTarget.y -= openDistance;
		doorRightTarget.y += openDistance;
	}
	else {
		doorLeftTarget.y += openDistance;
		doorRightTarget.y -= openDistance;
	}

	doorAnimT = 0.0f;
}

// Replace DoorAnimation (no longer threaded, called from UpdateScene)
void PlayerScene::TickDoorAnimation(float dt) {
	if (!doorAnimRunning) return;

	const float speed = 1.0f;
	doorAnimT += dt / speed;
	if (doorAnimT > 1.0f) doorAnimT = 1.0f;

	LeftDoor->transform.position.y = lerp(doorLeftStart.y, doorLeftTarget.y, doorAnimT);
	RightDoor->transform.position.y = lerp(doorRightStart.y, doorRightTarget.y, doorAnimT);

	LeftDoor->updateTransform();
	RightDoor->updateTransform();

	if (doorAnimT >= 1.0f)
		doorAnimRunning = false;
}

// Replace pushDSColor
void PlayerScene::pushDSColor(Color newColor, float time) {
	dsColorActive = true;
	dsColorTimer = 0.0f;
	dsColorDuration = time;
	dsPendingColor = newColor;
	g_engine->dualsense_setLightbarColor(newColor.R, newColor.G, newColor.B);
}

// Replace dsColorThreadFunc (no longer threaded, called from UpdateScene)
void PlayerScene::TickDSColor(float dt) {
	if (!dsColorActive) return;

	dsColorTimer += dt;
	if (dsColorTimer >= dsColorDuration) {
		dsColorActive = false;
		g_engine->dualsense_setLightbarColor(idleColor.R, idleColor.G, idleColor.B);
	}
}

// Replace LaunchTransition
void PlayerScene::LaunchTransition() {
	transitionActive = true;
	transitionTimer = 0.0f;
}

// Replace TransitionFunc (no longer threaded, called from UpdateScene)
void PlayerScene::TickTransition(float dt) {
	if (!transitionActive) return;

	transitionTimer += dt;
	if (transitionTimer >= 2.0f) {
		transitionActive = false;
		sceneToLoad = getNextScene();
		loadNewScene = true;
	}
}

bool PlayerScene::isCrouching() {
	return crouching || crouching2;
}

void PlayerScene::raycastHandler(RaycastHit hit) {

}