#include "ModulesManager.hpp"
#include <Core/Global.hpp>
#include <yaml-cpp/yaml.h>
// The imspinner devs decided to just not include imgui
#include <imgui.h>
#include <imgui_internal.h>
#ifdef UIMGUI_SPINNERS_MODULE_ENABLED
    #include "Modules/Spinners/ThirdParty/imspinner/imspinner.h"
#endif

#ifdef UIMGUI_KNOBS_MODULE_ENABLED
    #define AddBezierCurve AddBezierCubic
    #include "Modules/Knobs/ThirdParty/imgui-knobs/imgui-knobs.h"
#endif

#define CHECK_MODULE_ENABLED(x)     if (mod[#x])    \
{                                                   \
    Modules::data().x = mod[#x].as<bool>();         \
}

#include <Utilities.hpp>
#include <Components/Instance.hpp>


void UImGui::ModulesManager::init(const FString& configDir)
{
    YAML::Node node;
    try
    {
        node = YAML::LoadFile(configDir + "Core/Modules.yaml");
    }
    catch (YAML::BadFile&)
    {
        Logger::log("No Modules.yaml config file found, using the default configuration!", UVK_LOG_TYPE_WARNING);
        return;
    }

    if (node["undo-max-transactions"])
        settings.maxTransactions = node["undo-max-transactions"].as<size_t>();

    initModules(UImGui_InitInfo_getProjectDir());

    // Put this one after we have initialized the locale module
#ifdef UIMGUI_LOCALE_MODULE_ENABLED
    if (node["current-locale"])
        Locale::getCurrentLayout() = Locale::getLocaleID(node["current-locale"].as<std::string>());
#endif
}

void UImGui::ModulesManager::save(const FString& configDir) const noexcept
{
    YAML::Emitter out;
    out << YAML::BeginMap;

    out << YAML::Key << "undo-max-transactions" << YAML::Value << settings.maxTransactions;
#ifdef UIMGUI_LOCALE_MODULE_ENABLED
    out << YAML::Key << "current-locale" << YAML::Value << Locale::getLocaleName(Locale::getCurrentLayout());
#endif
    out << YAML::EndMap;

    std::ofstream fout(configDir + "Core/Modules.yaml");
    fout << out.c_str();
}

UImGui::ModuleSettings& UImGui::Modules::data() noexcept
{
    return Modules::get().settings;
}

void UImGui::Modules::save() noexcept
{
    Modules::get().save(UImGui::Utility::getGlobal().instance->initInfo.configDir);
}

UImGui::ModulesManager& UImGui::Modules::get() noexcept
{
    return internalGlobal.modulesManagerr;
}

void UImGui::ModulesManager::initModules(const FString& projectDir)
{
    YAML::Node node;
    try
    {
        node = YAML::LoadFile(projectDir + "uvproj.yaml");
    }
    catch (YAML::BadFile&)
    {
        Logger::log("No uvproj.yaml config file found, using the default configuration!", UVK_LOG_TYPE_WARNING);
        return;
    }
    auto mod = node["enabled-modules"];
    if (!mod)
        return;

    CHECK_MODULE_ENABLED(os);
    CHECK_MODULE_ENABLED(dbus);
    CHECK_MODULE_ENABLED(uexec);
    CHECK_MODULE_ENABLED(theming);
    CHECK_MODULE_ENABLED(notifications);
    CHECK_MODULE_ENABLED(polkit);
    CHECK_MODULE_ENABLED(ufont);
    CHECK_MODULE_ENABLED(i18n);
    CHECK_MODULE_ENABLED(undo_redo);
    CHECK_MODULE_ENABLED(plotting);
    CHECK_MODULE_ENABLED(knobs);
    CHECK_MODULE_ENABLED(spinners);
    CHECK_MODULE_ENABLED(toggles);
    CHECK_MODULE_ENABLED(text_utils);
    CHECK_MODULE_ENABLED(cli_parser);
    CHECK_MODULE_ENABLED(xdg);

#ifdef UIMGUI_UNDO_MODULE_ENABLED
    if (Modules::data().undo_redo)
        stateTracker.init();
#endif
#ifdef UIMGUI_I18N_MODULE_ENABLED
    if (Modules::data().i18n)
        localeManager.openLocaleConfig();
#endif
}