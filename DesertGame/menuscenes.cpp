#include "levelscenes.h"
#include "helpers.h"
#include <engine_tool_ui.h>
#include "globalobjects.h"

#ifdef _WIN32
#include <windows.h>
#endif

// intro scene

void IntroScene::InitScene(Engine* engine) {
	menuSound = engine->createSound("menusound", "assets/LabEscape.wav", true, false);
	soundObject = engine->createGameObject<GameObject>(Helpers::zeroTransform, nullptr, nullptr, nullptr, false);
	soundObject->playSound(menuSound, 1.0f);

	engine->SetUICallback(([this](Engine* engine)
	{
		this->MenuUICallback(engine);
	}));

	engine->setCursorMode(CursorMode::NORMAL);
	engine->setClearColor({ 0.3f,0.3f,0.3f });
	engine->dualsense_setLightbarColor(100, 100, 255);
}

void IntroScene::DestroyScene(Engine* engine) {
	engine->SetUICallback(nullptr);
	engine->setClearColor({ 0.0f,0.0f,0.0f });
	engine->dualsense_setLightbarColor(0, 0, 0);
}

void IntroScene::MenuUICallback(Engine* engine) {
	Vector2 extents = engine->getExtents();
	ToolUI::SetNextWindowSize({ 200,150 });
	ToolUI::SetNextWindowPos({ extents.x / 2 - 200 / 2, extents.y / 2 - 150 / 2 });
	ToolUI::Begin("Main menu");
	ToolUI::PushFont(UIFonts::largeFont);
	if (ToolUI::Button("Start game")) {
		sceneToLoad = GlobalObjects::level1;
		loadNewScene = true;
	}
	if (ToolUI::Button("Credits")) {
		sceneToLoad = GlobalObjects::credits;
		loadNewScene = true;
	}
	ToolUI::PopFont();
	ToolUI::End();
}

// credits scene

void CreditsScene::InitScene(Engine* engine) {
	menuSound = engine->createSound("menusound", "assets/Cresitd.wav", true, false);
	soundObject = engine->createGameObject<GameObject>(Helpers::zeroTransform, nullptr, nullptr, nullptr, false);
	soundObject->playSound(menuSound, 1.0f);

	engine->SetUICallback(([this](Engine* engine)
		{
			this->MenuUICallback(engine);
		}));

	engine->setCursorMode(CursorMode::NORMAL);
	engine->setClearColor({ 0.3f,0.3f,0.3f });

	hsv.s = 1;
	hsv.v = 1;
	hsv.h = 0;

	hue = 0.0f;
}

void CreditsScene::UpdateScene(Engine* engine) {
	// HSV DualSense rainbow
	float dt = engine->getDeltaTime();
	hue = fmod(hue + speed * dt, 360.0f);
	if (hue < 0) hue += 360.0f;
	hsv.h = hue;
	RGB rgb = HsvToRgb(hsv);
	engine->dualsense_setLightbarColor(rgb.r * 255, rgb.g * 255, rgb.b * 255);
}

void CreditsScene::DestroyScene(Engine* engine) {
	engine->SetUICallback(nullptr);
	engine->setClearColor({ 0.0f,0.0f,0.0f });
	engine->dualsense_setLightbarColor(0, 0, 0);
}

void OpenURL(const std::string& url) {
#ifdef _WIN32
	ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
	system(("xdg-open " + url).c_str());
#endif
}

inline void Link(const char* linkTitle, const char* link, bool sameline = true) {
	ToolUI::Text(linkTitle);
	if(sameline) ToolUI::SameLine();
	if (ToolUI::Button(link)) {
		OpenURL(link);
	}
}

void CreditsScene::MenuUICallback(Engine* engine) {
	Vector2 extents = engine->getExtents();
	ToolUI::SetNextWindowSize({ 600,350 });
	ToolUI::SetNextWindowPos({ extents.x / 2 - 600 / 2, extents.y / 2 - 350 / 2 });
	ToolUI::Begin("Credits");
	ToolUI::PushFont(UIFonts::largeFont);
	ToolUI::Text("Lead Developer - Oleh (spikest3r)");
	ToolUI::Text("Rebuilt in VkEngine");
	ToolUI::PopFont();
	
	Link("Duck by Poly by Google [CC-BY]", "(https://creativecommons.org/licenses/by/3.0/)", false);
	// ToolUI::SameLine();
	Link("via Poly Pizza", "(https://poly.pizza/m/6HpauUCfIAb)");

	ToolUI::Text("All other assets are from Unity Asset Store");
	ToolUI::Text("Monqo Studios, kuropen, BSW_Studio, reach the enD,\nAK STUDIO ART, Animatics Studio, TridentCorp, Navarone,\nBarking Dog, Simon Serge Pasi, OccaSoftware, ALP");
	// ToolUI::Text("\nhttps://olehsheremeta.com");
	Link("Engine: ", "https://olehsheremeta.com/projects/vkengine");
	Link("Original: ", "https://www.olehsheremeta.com/projects/lab-escape");
	Link("Repo: ", "https://github.com/spikest3r/LabEscape_VkEngine");
	ToolUI::PushFont(UIFonts::largeFont);
	if (ToolUI::Button("Return")) {
		sceneToLoad = GlobalObjects::intro;
		loadNewScene = true;
	}
	ToolUI::PopFont();
	ToolUI::End();
}

// generic hsv to rgb
CreditsScene::RGB CreditsScene::HsvToRgb(HSV hsv) {
	double r = 0, g = 0, b = 0;

	if (hsv.s <= 0.0) {       // Gray color
		r = g = b = hsv.v;
	}
	else {
		double hf = hsv.h / 60.0;
		int i = static_cast<int>(std::floor(hf));
		double f = hf - i;
		double pv = hsv.v * (1.0 - hsv.s);
		double qv = hsv.v * (1.0 - hsv.s * f);
		double tv = hsv.v * (1.0 - hsv.s * (1.0 - f));

		switch (i) {
			case 0: r = hsv.v; g = tv;    b = pv;    break; // Red sector
			case 1: r = qv;    g = hsv.v; b = pv;    break; // Yellow/Green
			case 2: r = pv;    g = hsv.v; b = tv;    break; // Green/Cyan
			case 3: r = pv;    g = qv;    b = hsv.v; break; // Blue sector
			case 4: r = tv;    g = pv;    b = hsv.v; break; // Magenta sector
			case 5: r = hsv.v; g = pv;    b = qv;    break; // Red/Magenta
			default: r = hsv.v; g = pv;   b = qv;    break; // Case for exactly 360
		}
	}

	// Return values normalized 0.0 to 1.0; multiply by 255 for 8-bit RGB
	return { r, g, b };
}