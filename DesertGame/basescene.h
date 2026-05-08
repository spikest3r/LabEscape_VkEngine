#pragma once

#include <engine.h>
#include <scene.h>
#include "noteobject.h"
#include "keypad.h"
#include <thread>

struct Color {
	unsigned char R;
	unsigned char G;
	unsigned char B;
};

class BaseScene : public Scene {
public:
	bool loadNewScene = false;
	Scene* sceneToLoad = nullptr;
};

class PlayerScene : public BaseScene {
private:
	bool crouching = false;
	bool crouching2 = false;
	bool pauseMenu = false;
	bool pauseKey = false;
	bool escape = false;
protected:
	bool isCrouching();

	Engine* g_engine;

	void InitScene(Engine* engine) override;
	void DestroyScene(Engine* engine) override;
	void UpdateScene(Engine* engine) override;
	GameObject* CreateGameObject(Engine* engine, const char* objectType, const char* tag, const char* name, Transform transform, Mesh* mesh, Texture* texture, bool dynamic) override;
	ICharacterController* controller;

	virtual Vector3 getSpawnPos();

	// CheckMovement
	void checkKeyboard(Engine* engine);
	void checkGamepad(Engine* engine);
	void checkRaycast(Engine* engine);
	void processMovement(Engine* engine);

	virtual std::string getNoteText(std::string noteName);
	virtual std::string getKeypadCode(std::string kpName);
	virtual void raycastHandler(RaycastHit hit);

	virtual void UICallback(Engine* engine);

	bool openNote = false;
	NoteObject* noteToShow;

	bool openKeypad = false;
	Keypad* keypadToShow;
	char codeInputBuffer[16];
	void handleKeypadNumKey(int idx);

	void closePopup();
	bool closePopupFlag = false;

	float pitch = 0.0f;
	float yaw = -90.0f;
	bool firstMouse = true;
	float lastX = 0.0f;
	float lastY = 0.0f;
	Vector3 prevCameraPos = { 0,0,0 };
	bool stepCount = false;
	bool initialized = false;
	float lastVerticalVel = 0.0f;
	bool isGrounded = false;
	float previousCosBob = 0;

	// Movement Settings
	float bobTime = 0.0f;
	float bobbingAmplitude = 0.2f;
	float bobbingFrequency = 13.0f;
	float bobbingSmoothness = 10.0f;
	float bobbingMaxSpeed = 5.0f;
	float bobbingBaseZ = 0.0f;
	float baseMoveSpeed = 12.0f;
	float mouseSensitivity = 0.1f;

	// Scene resources
	Sound* step1;
	Sound* step2;
	Sound* ambientSfx;
	GameObject* ambientPlayer;

	Sound* sfx_keypadClick;
	Sound* sfx_keypadDenied;
	Sound* sfx_keypadGranted;
	Sound* sfx_keypadHapticClick;
	Sound* noteSfx;

	Trigger* exitTrigger;
	bool doorsUnlocked = false;

	void LaunchTransition();
	virtual Scene* getNextScene();

	// door anim
	GameObject* LeftDoor;
	GameObject* RightDoor;
	std::thread doorThread;
	
	// Door anim
	Vector3 doorLeftStart, doorRightStart;
	Vector3 doorLeftTarget, doorRightTarget;
	float   doorAnimT = 0.0f;
	bool	doorAnimRunning = false;

	// DS color
	bool    dsColorActive = false;
	float   dsColorTimer = 0.0f;
	float   dsColorDuration = 0.0f;
	Color   dsPendingColor = {};
	Color	idleColor = { 190,200,255 };

	// Transition
	bool    transitionActive = false;
	float   transitionTimer = 0.0f;

	void PlayDoorAnim();
	void TickDoorAnimation(float dt);
	void pushDSColor(Color newColor, float time);
	void TickDSColor(float dt);
	void TickTransition(float dt);

	UIElement* crosshair;
	Texture* crosshairTex;

	bool raycastKey = false;
	bool enforcePause = false;
};