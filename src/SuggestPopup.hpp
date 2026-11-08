#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

struct Suggestion {
	int levelID = 0;
	int position = 0;
	std::string name;
};

class SuggestPopup : public Popup<> {
protected:
	EventListener<web::WebTask> m_listener;
	CCLabelBMFont* m_status = nullptr;
	std::vector<Ref<CCNode>> m_rowNodes;

	bool setup() override;

	void fetchList();
	void showError(std::string const& msg);
	void buildToday(bool reroll);
	void render(std::vector<Suggestion> const& picks);
	void clearRows();

	void onReroll(CCObject*);
	void onOpenLevel(CCObject* sender);

public:
	static SuggestPopup* create();
};
