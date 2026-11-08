#include "SuggestPopup.hpp"

#include <algorithm>
#include <ctime>
#include <random>

// Cached for the whole game session so we only hit the API once.
static std::vector<Suggestion> s_cache;

static constexpr char const* LIST_URL = "https://pointercrate.com/api/v2/demons/listed/?limit=75";

static int todayKey() {
	std::time_t t = std::time(nullptr);
	std::tm* lt = std::localtime(&t);
	return (lt->tm_year + 1900) * 10000 + (lt->tm_mon + 1) * 100 + lt->tm_mday;
}

static int levelCountSetting() {
	return static_cast<int>(Mod::get()->getSettingValue<int64_t>("suggestion-count"));
}

SuggestPopup* SuggestPopup::create() {
	auto ret = new SuggestPopup();
	float height = 110.f + levelCountSetting() * 32.f;
	if (ret->initAnchored(380.f, height)) {
		ret->autorelease();
		return ret;
	}
	CC_SAFE_DELETE(ret);
	return nullptr;
}

bool SuggestPopup::setup() {
	this->setTitle("Today's Suggestions");

	m_status = CCLabelBMFont::create("Loading Demon List...", "bigFont.fnt");
	m_status->setScale(0.5f);
	m_mainLayer->addChildAtPosition(m_status, Anchor::Center);

	auto rerollSpr = ButtonSprite::create("Reroll", "goldFont.fnt", "GJ_button_01.png", 0.8f);
	auto rerollBtn = CCMenuItemSpriteExtra::create(rerollSpr, this, menu_selector(SuggestPopup::onReroll));
	m_buttonMenu->addChildAtPosition(rerollBtn, Anchor::Bottom, {0.f, 26.f});

	if (!s_cache.empty()) {
		m_status->setVisible(false);
		this->buildToday(false);
	}
	else {
		this->fetchList();
	}
	return true;
}

void SuggestPopup::fetchList() {
	m_status->setString("Loading Demon List...");
	m_status->setVisible(true);

	m_listener.bind([this](web::WebTask::Event* e) {
		auto res = e->getValue();
		if (!res) return; // progress / cancelled events

		if (!res->ok()) {
			this->showError("Could not reach the list.\nCheck your connection and press Reroll.");
			return;
		}

		auto json = res->json();
		if (json.isErr()) {
			this->showError("The list returned bad data.\nPress Reroll to retry.");
			return;
		}

		auto arr = json.unwrap().asArray();
		if (arr.isErr()) {
			this->showError("The list returned bad data.\nPress Reroll to retry.");
			return;
		}

		std::vector<Suggestion> list;
		auto items = arr.unwrap();
		for (auto& item : items) {
			Suggestion s;
			s.levelID = static_cast<int>(item["level_id"].asInt().unwrapOr(0));
			s.position = static_cast<int>(item["position"].asInt().unwrapOr(0));
			s.name = item["name"].asString().unwrapOr("Unknown");
			if (s.levelID > 0 && s.position > 0) list.push_back(std::move(s));
		}

		if (list.empty()) {
			this->showError("The list was empty.\nPress Reroll to retry.");
			return;
		}

		s_cache = std::move(list);
		m_status->setVisible(false);
		this->buildToday(false);
	});

	web::WebRequest req;
	req.userAgent("GeodeMod/DailyLevelSuggestions");
	m_listener.setFilter(req.get(LIST_URL));
}

void SuggestPopup::showError(std::string const& msg) {
	this->clearRows();
	m_status->setString(msg.c_str());
	m_status->setVisible(true);
}

void SuggestPopup::clearRows() {
	for (auto& n : m_rowNodes) {
		if (n) n->removeFromParent();
	}
	m_rowNodes.clear();
}

void SuggestPopup::buildToday(bool reroll) {
	auto mod = Mod::get();
	int64_t minP = mod->getSettingValue<int64_t>("min-position");
	int64_t maxP = mod->getSettingValue<int64_t>("max-position");
	if (minP > maxP) std::swap(minP, maxP);
	bool skipDone = mod->getSettingValue<bool>("skip-completed");
	size_t count = static_cast<size_t>(levelCountSetting());
	int key = todayKey();

	std::vector<Suggestion> picks;

	// Reuse today's saved picks so the list stays stable all day
	// (even after you beat one of them).
	if (!reroll && mod->getSavedValue<int>("day-key", 0) == key) {
		auto ids = mod->getSavedValue<std::vector<int>>("today-ids", {});
		for (int id : ids) {
			auto it = std::find_if(s_cache.begin(), s_cache.end(), [id](Suggestion const& s) {
				return s.levelID == id;
			});
			if (it != s_cache.end()) picks.push_back(*it);
		}
	}

	if (picks.size() != count) {
		picks.clear();

		std::vector<Suggestion> pool;
		auto stats = GameStatsManager::sharedState();
		for (auto const& s : s_cache) {
			if (s.position < minP || s.position > maxP) continue;
			if (skipDone && stats->hasCompletedOnlineLevel(s.levelID)) continue;
			pool.push_back(s);
		}

		std::mt19937 rng(reroll ? std::random_device{}() : static_cast<uint32_t>(key));
		std::shuffle(pool.begin(), pool.end(), rng);
		if (pool.size() > count) pool.resize(count);
		picks = std::move(pool);

		std::vector<int> ids;
		for (auto const& p : picks) ids.push_back(p.levelID);
		mod->setSavedValue<int>("day-key", key);
		mod->setSavedValue<std::vector<int>>("today-ids", ids);
	}

	if (picks.empty()) {
		this->showError("No levels left in that range!\nAdjust the mod settings.");
		return;
	}

	this->render(picks);
}

void SuggestPopup::render(std::vector<Suggestion> const& picks) {
	this->clearRows();
	m_status->setVisible(false);

	auto stats = GameStatsManager::sharedState();

	for (size_t i = 0; i < picks.size(); i++) {
		auto const& s = picks[i];
		float y = -62.f - static_cast<float>(i) * 32.f;

		if (stats->hasCompletedOnlineLevel(s.levelID)) {
			auto check = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
			check->setScale(0.6f);
			m_buttonMenu->addChildAtPosition(check, Anchor::Top, {-165.f, y});
			m_rowNodes.push_back(check);
		}

		auto label = CCLabelBMFont::create(fmt::format("#{}  {}", s.position, s.name).c_str(), "goldFont.fnt");
		label->limitLabelWidth(220.f, 0.7f, 0.2f);
		label->setAnchorPoint({0.f, 0.5f});
		m_buttonMenu->addChildAtPosition(label, Anchor::Top, {-145.f, y});
		m_rowNodes.push_back(label);

		auto spr = ButtonSprite::create("View", "goldFont.fnt", "GJ_button_01.png", 0.7f);
		auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(SuggestPopup::onOpenLevel));
		btn->setTag(s.levelID);
		m_buttonMenu->addChildAtPosition(btn, Anchor::Top, {140.f, y});
		m_rowNodes.push_back(btn);
	}
}

void SuggestPopup::onReroll(CCObject*) {
	if (s_cache.empty()) {
		this->fetchList();
		return;
	}
	this->buildToday(true);
}

void SuggestPopup::onOpenLevel(CCObject* sender) {
	int id = static_cast<CCNode*>(sender)->getTag();
	auto search = GJSearchObject::create(SearchType::Search, std::to_string(id));
	auto scene = LevelBrowserLayer::scene(search);
	CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
}
