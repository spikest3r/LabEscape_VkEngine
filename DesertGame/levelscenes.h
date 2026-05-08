#pragma once

#include "basescene.h"
#include <random>
#include "level3objects.h"

class Level1Scene : public PlayerScene {
	void UpdateScene(Engine* engine) override;
	std::string getNoteText(std::string name) override;
	std::string getKeypadCode(std::string kpName) override;
	Scene* getNextScene() override;
};

class Level2Scene : public PlayerScene {
	std::string getNoteText(std::string name) override;
	std::string getKeypadCode(std::string kpName) override;
	Scene* getNextScene() override;
	void DestroyScene(Engine* engine) override;
	void InitScene(Engine* engine) override;
	void UpdateScene(Engine* engine) override;
	void UICallback(Engine* engine) override;
private:
	std::random_device rd;
	std::mt19937 gen;
	std::uniform_real_distribution<float> dist;

	Trigger* beginTrigger;
	bool trackingPlayer = false;
	bool trackerActive = true;
	void CheckTracking();
	
	bool lost = false;
	bool reload = false;

	Sound* gameover;
	Sound* rumble;

	Texture* borderTex;
	UIElement* border;
	float borderOffset = 512.f;

	float stopTimeout = 5.0f;
	float stopTime = 5.0f;
	float stopTime_value = 5.0f;
	bool stop = false;
	float t = 0.0f;

	bool hasController = false;
};

class Level3Scene : public PlayerScene {
	void UpdateScene(Engine* engine) override;
	std::string getNoteText(std::string name) override;
	std::string getKeypadCode(std::string kpName) override;
	Scene* getNextScene() override;
	void EarlyInitScene(Engine* engine) override;
	void InitScene(Engine* engine) override;
	void raycastHandler(RaycastHit hit) override;
	void UICallback(Engine* engine) override;
	void DestroyScene(Engine* engine) override;
private:
	std::random_device rd;
	std::mt19937 gen;
	std::uniform_int_distribution<int> dist;
	std::uniform_int_distribution<int> simonDist;
	std::uniform_int_distribution<int> distBall;
	std::uniform_int_distribution<int> distGTW;

	std::string randomKeypadCode;
	char codeReveal[5];
	void RevealCodeDigit(int index);
	Sound* success;
	GameObject* sfxPlayer;

	// simon says minigame
	Trigger* simonSaysTrigger;
	Texture* digitEmpty;
	Texture* digits[5];
	SimonSaysCube* digitCubes[9];
	bool simonSaysRunning = false;
	bool simonSaysCompleted = false;
	int pattern[5];
	int userPattern[5];
	bool simonSaysInputBlocked = true;
	bool showingPattern = false;
	float timePerBlock = 1.0f;
	int patternLimit = 1;
	int indexShowing = 0;
	int indexTapping = 0;
	void ShowPattern();
	void GeneratePattern();

	// ball and cup minigame
	Trigger* ballAndCupTrigger;
	CupObject* cups[3];
	Vector3 cupPosition[3];
	Vector3 ballScale;
	GameObject* ball;
	void SwitchCups(int indexFrom, int indexTo);
	void SetBallState(bool visible);
	bool isSwitchingCups = false;
	float cupTime = 0.0f;
	CupObject* cupA;
	CupObject* cupB;
	Vector2 cupFrom;
	Vector2 cupTo;
	int ballIndex = 1;
	void ShuffleBallIndex();
	int shuffleIterations = 3;
	float cupLerpSpeed = 0.8f;
	void BeginShuffleGame();
	bool shuffleGameRunning = false;
	bool shuffleGameDone = false;
	int currentShuffleIteration = 1;
	bool awaitingCupRaycast = false;
	bool isUp = false;
	int cupUpAnimationStage = 0;
	float ballTime = 0.0f;
	float cupUpWait = 0.0f;
	void ShowBall();
	bool showedBall = false;
	bool initiatedShowBall = false;
	int selectedRight = 0;

	// guess the word
	bool gtwActive = false;
	bool gtwCompleted = false;
	int gtwWordIndex = 0;
	char gtwUserWord[16];
	const static int gtwWordCount = 3;
	static constexpr const char* gtwWordVariants[3] = {
		"facts",
		"panda",
		"alligator"
	};
	static constexpr const char* gtwWordVariantsScrambled[3] = {
		"atsfc",
		"napda",
		"lagratilo"
	};

	// ducks
	Sound* duckSound;
	Texture* duckTexture;
	Texture* activeDuckTexture;
	Trigger* duckTableTrigger;
	int duckCounter = 0;
	GameObject* puppetDuck;
	bool isMovingDuck;
	float movingDistance = 10.0f;
};

class LevelFinalScene : public PlayerScene {
	void InitScene(Engine* engine) override;
	void DestroyScene(Engine* engine) override;
	void UICallback(Engine* engine) override;
	Vector3 getSpawnPos() override;
	Sound* ambient;
	GameObject* ambientPlayer;
	Trigger* endTrigger;
	Sound* success;
};

// misc

class IntroScene : public BaseScene {
	void InitScene(Engine* engine) override;
	void DestroyScene(Engine* engine) override;
	GameObject* soundObject;
	Sound* menuSound;
	void MenuUICallback(Engine* engine);
};

class CreditsScene : public BaseScene {
public:
	struct RGB { double r, g, b; };
	struct HSV { double h, s, v; };
private:
	void InitScene(Engine* engine) override;
	void DestroyScene(Engine* engine) override;
	void UpdateScene(Engine* engine) override;
	GameObject* soundObject;
	Sound* menuSound;
	void MenuUICallback(Engine* engine);

	// Dualsense rainbow effect
	RGB HsvToRgb(HSV hsv);
	HSV hsv;
	float speed = 20.0f;
	float hue = 0.0f;
};