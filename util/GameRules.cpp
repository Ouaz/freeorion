#include "GameRules.h"

namespace {
    std::vector<GameRulesFn>& GameRulesRegistry() {
        static std::vector<GameRulesFn> game_rules_registry;
        return game_rules_registry;
    }
}


/////////////////////////////////////////////
// Free Functions
/////////////////////////////////////////////
bool RegisterGameRules(GameRulesFn function) {
    GameRulesRegistry().emplace_back(function);
    return true;
}

GameRules& GetGameRules() {
    static GameRules game_rules;
    if (!GameRulesRegistry().empty()) {
        DebugLogger() << "Adding options rules";
        for (GameRulesFn fn : GameRulesRegistry())
            fn(game_rules);
        GameRulesRegistry().clear();
    }

    return game_rules;
}


/////////////////////////////////////////////////////
// GameRule
/////////////////////////////////////////////////////
GameRule::GameRule(Type type_, std::string name_, boost::any value_,
                   boost::any default_value_, std::string description_,
                   std::unique_ptr<ValidatorBase>&& validator_, bool engine_internal_,
                   std::string category_) :
    OptionsDB::Option(static_cast<char>(0), std::move(name_), std::move(value_),
                      std::move(default_value_), std::move(description_),
                      std::move(validator_), engine_internal_, false, true, "setup.rules"),
    type(type_),
    category(std::move(category_))
{}


/////////////////////////////////////////////////////
// GameRules
/////////////////////////////////////////////////////
bool GameRules::Empty() const {
    CheckPendingGameRules();
    return m_game_rules.empty();
}

std::unordered_map<std::string, GameRule>::const_iterator GameRules::begin() const {
    CheckPendingGameRules();
    return m_game_rules.begin();
}

std::unordered_map<std::string, GameRule>::const_iterator GameRules::end() const {
    CheckPendingGameRules();
    return m_game_rules.end();
}

std::unordered_map<std::string, GameRule>::iterator GameRules::begin() {
    CheckPendingGameRules();
    return m_game_rules.begin();
}

std::unordered_map<std::string, GameRule>::iterator GameRules::end() {
    CheckPendingGameRules();
    return m_game_rules.end();
}

bool GameRules::RuleExists(const std::string& name) const {
    CheckPendingGameRules();
    return m_game_rules.count(name);
}

bool GameRules::RuleExists(const std::string& name, GameRule::Type type) const {
    if (type == GameRule::Type::INVALID)
        return false;
    CheckPendingGameRules();
    auto rule_it = m_game_rules.find(name);
    if (rule_it == m_game_rules.end())
        return false;
    return rule_it->second.type == type;
}

GameRule::Type GameRules::GetType(const std::string& name) const {
    CheckPendingGameRules();
    auto rule_it = m_game_rules.find(name);
    if (rule_it == m_game_rules.end())
        return GameRule::Type::INVALID;
    return rule_it->second.type;
}

bool GameRules::RuleIsInternal(const std::string& name) const {
    CheckPendingGameRules();
    auto rule_it = m_game_rules.find(name);
    if (rule_it == m_game_rules.end())
        return false;
    return rule_it->second.IsInternal();
}

const std::string& GameRules::GetDescription(const std::string& rule_name) const {
    CheckPendingGameRules();
    auto it = m_game_rules.find(rule_name);
    if (it == m_game_rules.end())
        throw std::runtime_error(("GameRules::GetDescription(): No option called \"" + rule_name + "\" could be found.").c_str());
    return it->second.description;
}

const ValidatorBase* GameRules::GetValidator(const std::string& rule_name) const {
    CheckPendingGameRules();
    auto it = m_game_rules.find(rule_name);
    if (it == m_game_rules.end())
        throw std::runtime_error(("GameRules::GetValidator(): No option called \"" + rule_name + "\" could be found.").c_str());
    return it->second.validator.get();
}

void GameRules::ClearExternalRules() {
    CheckPendingGameRules();
    auto it = m_game_rules.begin();
    while (it != m_game_rules.end()) {
        bool engine_internal = it->second.storable; // OptionsDB::Option member used to store if this option is engine-internal
        if (!engine_internal)
            it = m_game_rules.erase(it);
        else
            ++it;
    }
}

void GameRules::ResetToDefaults() {
    CheckPendingGameRules();
    for (auto& it : m_game_rules)
        it.second.SetToDefault();
}

std::map<std::string, std::string> GameRules::GetRulesAsStrings() const {
    CheckPendingGameRules();
    std::map<std::string, std::string> retval;
    for (auto& [rule_name, rule_value] : m_game_rules)
        retval.emplace(rule_name, rule_value.ValueToString());
    return retval;
}

void GameRules::Add(Pending::Pending<GameRules>&& future)
{ m_pending_rules = std::move(future); }

void GameRules::SetFromStrings(const std::map<std::string, std::string>& names_values) {
    CheckPendingGameRules();
    DebugLogger() << "Setting Rules from Strings:";
    for (auto& [name, value] : names_values)
        DebugLogger() << "  " << name << " : " << value;

    ResetToDefaults();
    for (auto& [name, value] : names_values) {
        auto rule_it = m_game_rules.find(name);
        if (rule_it == m_game_rules.end()) {
            InfoLogger() << "GameRules::serialize received unrecognized rule: " << name;
            continue;
        }
        try {
            rule_it->second.SetFromString(value);
        } catch (const boost::bad_lexical_cast&) {
            ErrorLogger() << "Unable to set rule: " << name << " to value: " << value
                          << " - couldn't cast string to allowed value for this option";
        } catch (...) {
            ErrorLogger() << "Unable to set rule: " << name << " to value: " << value;
        }
    }

    DebugLogger() << "After Setting Rules:";
    for (auto& [name, value] : m_game_rules)
        DebugLogger() << "  " << name << " : " << value.ValueToString();
}

void GameRules::CheckPendingGameRules() const {
    if (!m_pending_rules)
        return;

    auto parsed_new_rules = Pending::WaitForPending(m_pending_rules);
    if (!parsed_new_rules)
        return;

    for (auto& [name, value] : *parsed_new_rules) {
        if (m_game_rules.count(name)) {
            ErrorLogger() << "GameRules::Add<>() : GameRule " << name << " was added twice. Skipping ...";
            continue;
        }
        m_game_rules.emplace(name, std::move(value));
    }

    DebugLogger() << "Registered and Parsed Game Rules:";
    for (auto& [name, value] : GetRulesAsStrings())
        DebugLogger() << " ... " << name << " : " << value;
}
