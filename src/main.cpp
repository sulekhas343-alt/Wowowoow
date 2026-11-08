#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "SuggestPopup.hpp"

using namespace geode::prelude;

class $modify(DailyMenuLayer, MenuLayer) {
	bool init() {
		if (!MenuLayer::init()) return false;

		auto spr = CircleButtonSprite::createWithSpriteFrameName(
			"GJ_starsIcon_001.png", 1.0f, CircleBaseColor::Green, CircleBaseSize::Medium
		);
		auto btn = CCMenuItemSpriteExtra::create(
			spr, this, menu_selector(DailyMenuLayer::onOpenSuggestions)
		);
		btn->setID("suggestions-button"_spr);

		if (auto menu = this->getChildByID("bottom-menu")) {
			menu->addChild(btn);
			menu->updateLayout();
		}
		return true;
	}

	void onOpenSuggestions(CCObject*) {
		if (auto popup = SuggestPopup::create()) popup->show();
	}
};
