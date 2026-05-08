#pragma once

#include <engine.h>
#include <engine_tool_ui.h>

#include "levelscenes.h"

class GlobalObjects {
public:
	static PhysicsMaterial* characterMaterial;
	static Level1Scene* level1;
	static Level2Scene* level2;
	static Level3Scene* level3;
	static LevelFinalScene* levelFinal;
	static IntroScene* intro;
	static CreditsScene* credits;
};

class UIFonts {
public:
	static UIFont defaultFont;
	static UIFont largeFont;
};