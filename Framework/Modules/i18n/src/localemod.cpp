#ifdef UIMGUI_I18N_MODULE_ENABLED
#include "localemod.hpp"
#include "yaml.h"
#include <Utilities.hpp>
#include <Core/Global.hpp>

UImGui::LocaleManager::~LocaleManager() noexcept
{
    for (auto& a : translations)
    {
        for (auto& f : a)
        {
            f.first.clear();
            f.second.clear();
        }
        a.clear();
    }
}

void UImGui::LocaleManager::openLocaleConfig() noexcept
{
    YAML::Node node;
    try
    {
        node = YAML::LoadFile("../Config/Translations/translation-base.yaml");
    }
    catch (YAML::BadFile&)
    {
        Logger::log("Couldn't open the translation base file!", UVK_LOG_TYPE_WARNING);
        return;
    }

    if (node["origin-locale"])
    {
        auto str = Utility::toLower(node["origin-locale"].as<std::string>().c_str());
        auto it = str.find('-');
        if (it != std::string::npos)
            str[it] = '_';

        defaultLayout = UImGui::Locale::getLocaleID(str);
        if (defaultLayout == static_cast<UImGui::LocaleTypes>(-1))
        {
            Logger::log("A non-valid default layout string was submitted! The layout will not be used until the error is fixed! String: ", UVK_LOG_TYPE_WARNING, str);
            return;
        }
        currentLayout = defaultLayout;
    }

    const auto& strings = node["strings"];
    if (strings)
        for (const auto& a : strings)
            translations[static_cast<int>(currentLayout)].emplace_back( a.as<std::string>(), a.as<std::string>() );
    if (exists(std_filesystem::path("../Config/Translations/")))
    {
        YAML::Node node2;
        for (auto& a : std_filesystem::directory_iterator(std_filesystem::path("../Config/Translations")))
        {
            if (a.path().filename() != "translation-base.yaml" && (a.path().extension().string() == ".yaml" || a.path().extension().string() == ".yml"))
            {
                auto tmp = a.path().filename().string();
                tmp.erase(tmp.find_last_of('.'));
                auto id = Locale::getLocaleID(Utility::toLower(tmp.c_str()));
                if (id == static_cast<LocaleTypes>(-1))
                    continue;

                try
                {
                    node2 = YAML::LoadFile(a.path().string());
                }
                catch (YAML::BadFile&)
                {
                    continue;
                }
                if (node2["strings"])
                    for (const auto& f : node2["strings"])
                        if (f["string"] && f["translation"])
                            translations[static_cast<int>(id)].emplace_back( f["string"].as<std::string>(), f["translation"].as<std::string>() );
            }
        }
        Logger::log("Successfully loaded translations!", UVK_LOG_TYPE_SUCCESS);
    }
    else
        Logger::log("Couldn't open the Config/Translations directory, no translations were loaded, make sure you at least provide an empty translation file or your program will crash in production!", UVK_LOG_TYPE_ERROR);
}

UImGui::FString& UImGui::LocaleManager::getLocaleString(UImGui::String original, UImGui::LocaleTypes locale) noexcept
{
    auto& arr = translations[static_cast<int>(locale)];
    for (auto& a : arr)
        if (a.first == original)
            return a.second;
    Logger::log("Couldn't find the translated string for locale ", UVK_LOG_TYPE_WARNING, Locale::getLocaleName(locale, true), ". Reverting to default layout ", Locale::getLocaleName(defaultLayout, true), "!");

    return emptyString;
}

const char* UImGui::Locale::getLocaleName(UImGui::LocaleTypes types, bool bShort) noexcept
{
    if (bShort)
        return localeStrings[static_cast<int>(types)];
    return localeStringFull[static_cast<int>(types)];
}

UImGui::LocaleTypes UImGui::Locale::getLocaleID(const UImGui::FString& str) noexcept
{
    for (size_t ret = 0; ret < static_cast<size_t>(LocaleTypes::COUNT); ret++)
        if (localeStrings[ret] == str)
            return static_cast<LocaleTypes>(ret);
    return static_cast<LocaleTypes>(-1);
}

const UImGui::FString& UImGui::Locale::getLocaleString(const UImGui::FString& original, UImGui::LocaleTypes locale) noexcept
{
    return Modules::get().localeManager.getLocaleString(original.c_str(), locale);
}

UImGui::LocaleTypes& UImGui::Locale::getCurrentLayout() noexcept
{
    return Modules::get().localeManager.currentLayout;
}

UImGui::LocaleTypes& UImGui::Locale::getFallbackLayout() noexcept
{
    return Modules::get().localeManager.defaultLayout;
}

const UImGui::FString &UImGui::Locale::getLocaleString(const UImGui::FString& original) noexcept
{
    return getLocaleString(original, getCurrentLayout());
}

#endif