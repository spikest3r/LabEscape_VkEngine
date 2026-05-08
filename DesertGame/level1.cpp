#include "levelscenes.h"
#include "globalobjects.h"

void Level1Scene::UpdateScene(Engine* engine) {
	PlayerScene::UpdateScene(engine);
}

std::string Level1Scene::getNoteText(std::string name) {
	if (name == "Note1") {
		return "Hello, world!";
	}
	if (name == "Note2") {
		std::string riddle = "I'm less than a dozen, but not by much.\nI'm perfect for counting — fingers and such.\nI'm the base of your math, the metric you know,\nIn binary code, I'm a one-zero show.\nI follow the line when nine says \"go,\"\nAnd with me, the decimal starts to flow.\nA = this number";
		return riddle;
	}
	if (name == "Note3") {
		std::string hint = "Passcode to the gate is:\nA + B\nA is our ID\nB = first 3 digits of e";
		return hint;
	}
	if (name == "Note4") {
		return "2024 ID a.k.a. A is: 13";
	}
	if (name == "Note5") {
		std::string mixture = "Mixture:\n- Nitric Acid\n- Hydrofluoric Acid\n- Stored under dry ice and lead shielding\n! Reactive with skin, air, and stupidity.\n! Will detonate if moved improperly.\nNote: Previous test yielded plasma\n arc and loss of ceiling tiles.";
		return mixture;
	}
	return "Placeholder. Check note name!"; // placeholder to catch invalid notes
}

// one per scene
std::string Level1Scene::getKeypadCode(std::string kpName) {
	return "1271";
}

Scene* Level1Scene::getNextScene() {
	return GlobalObjects::level2;
}