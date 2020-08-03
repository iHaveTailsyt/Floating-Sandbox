/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-08-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "LocalizationManager.h"

#include <Game/ResourceLocator.h>

#include <GameCore/Log.h>

#include <algorithm>
#include <filesystem>

static wxLanguage constexpr TranslationsMsgIdLangId = wxLANGUAGE_ENGLISH; // The language used for the message id's

static std::string const TranslationsDomainName = "ui_strings";

std::unique_ptr<LocalizationManager> LocalizationManager::CreateInstance(std::optional<std::string> desiredLanguageIdentifier)
{
    // Create list of available languages
    auto availableLanguages = MakeAvailableLanguages();

    // See desired language
    std::optional<LanguageInfo> desiredLanguageInfo;
    wxLanguage localeLanguage = wxLANGUAGE_DEFAULT; // Let wxWidgets choose the language by default
    if (desiredLanguageIdentifier.has_value())
    {
        // Make sure the specified identifier is a language supported by us
        auto const it = std::find_if(
            availableLanguages.cbegin(),
            availableLanguages.cend(),
            [&desiredLanguageIdentifier](auto const & al)
            {
                return al.Identifier == *desiredLanguageIdentifier;
            });

        if (it != availableLanguages.cend())
        {
            // Get the wxWidgets language ID, if any            
            if (auto const wxLangInfo = wxLocale::FindLanguageInfo(*desiredLanguageIdentifier); 
                wxLangInfo != nullptr)
            {
                localeLanguage = static_cast<wxLanguage>(wxLangInfo->Language);
                desiredLanguageInfo = *it;
            }
        }
        else
        {
            LogMessage("WARNING: language \"", desiredLanguageIdentifier.value_or("<N/A>"), "\" is not a language supported by Floating Sandbox");
        }
    }

    // Create wxWidgets locale for this language
    auto locale = std::make_unique<wxLocale>();
    auto res = locale->Init(localeLanguage);
    if (!res)
    {
        LogMessage("WARNING: failed locale initialization with language ", localeLanguage);
    }
    else
    {
        // Add our catalog
        locale->AddCatalogLookupPathPrefix(ResourceLocator::GetLanguagesRootPath().string());
        res = locale->AddCatalog(TranslationsDomainName, TranslationsMsgIdLangId);
        if (!res && localeLanguage != TranslationsMsgIdLangId
            && localeLanguage != wxLANGUAGE_DEFAULT) // AddCatalog returns false for msgIdLang & default
        {
            LogMessage("WARNING: failed locale catalog initialization with language ", localeLanguage);
        }

        if (auto const translations = wxTranslations::Get();
            translations != nullptr)
        {
            translations->AddStdCatalog();
        }
    }

    // Get enforced language
    if (auto const translations = wxTranslations::Get();
        translations != nullptr)
    {
        auto enforcedLanguage = translations->GetBestTranslation(TranslationsDomainName, TranslationsMsgIdLangId);
        LogMessage("Enforced language for desired language \"", desiredLanguageIdentifier.value_or("<N/A>"),
            "\": \"", enforcedLanguage, "\"");
    }

    return std::unique_ptr<LocalizationManager>(
        new LocalizationManager(
            desiredLanguageInfo,
            std::move(availableLanguages),
            std::move(locale)));

    // TODOOLD
    /*
    // Initialize enforced as default language
    wxLanguage enforcedLanguageId = TranslationsMsgIdLangId;
    LanguageInfo const * enforcedLanguageInfo = FindLanguageInfo(enforcedLanguageId, availableLanguages);
    assert(nullptr != enforcedLanguageInfo);

    // Get wxWidgets language ID and LanguageInfo
    if (languageIdentifier.has_value())
    {
        // Make sure the specified identifier is a language supported by us
        auto const it = std::find_if(
            availableLanguages.cbegin(),
            availableLanguages.cend(),
            [&languageIdentifier](auto const & al)
            {
                return al.Identifier == *languageIdentifier;
            });

        if (it != availableLanguages.cend())
        {
            // Get the wxWidgets language ID, if any
            auto const wxLangInfo = wxLocale::FindLanguageInfo(*languageIdentifier);
            if (wxLangInfo != nullptr)
            {
                languageId = static_cast<wxLanguage>(wxLangInfo->Language);
            }

            languageInfo = &(*it);
        }
        else
        {
            LogMessage("WARNING: language \"" + *languageIdentifier + "\" is not a language supported by Floating Sandbox");
        }
    }

    // Create wxWidgets locale for this language
    auto locale = std::make_unique<wxLocale>();
    auto res = locale->Init(languageId);
    if (!res)
    {
        LogMessage("WARNING: locale initialization with language ", languageId, " failed");
    }
    else
    {
        // Add catalog for this language
        locale->AddCatalogLookupPathPrefix(ResourceLocator::GetLanguagesRootPath().string());
        res = locale->AddCatalog(TranslationsDomainName, TranslationsMsgIdLangId);
        if (!res && languageId != TranslationsMsgIdLangId) // AddCatalog returns false for msgIdLang
        {
            LogMessage("WARNING: locale catalog initialization with language ", languageId, " failed");
        }
        else
        {
            LogMessage("Successfully set language ", languageId);
        }
    }

    assert(nullptr != languageInfo);

    return std::unique_ptr<LocalizationManager>(
        new LocalizationManager(
            *languageInfo, // TODO
            *languageInfo, // TODO
            std::move(availableLanguages),
            std::move(locale)));
    */
}

void LocalizationManager::StoreDesiredLanguage(std::optional<std::string> const & languageIdentifier)
{
    if (languageIdentifier.has_value())
    {
        auto languageInfo = FindLanguageInfo(
            *languageIdentifier,
            mAvailableLanguages);

        if (nullptr == languageInfo)
        {
            throw std::logic_error("Unrecognized language identifier \"" + *languageIdentifier + "\"");
        }

        mDesiredLanguage = *languageInfo;
    }
    else
    {
        mDesiredLanguage.reset();
    }
}

std::string LocalizationManager::MakeLanguageIdentifier(wxString const & canonicalLanguageName)
{
    return canonicalLanguageName.BeforeFirst('_').ToStdString();
}

std::vector<LocalizationManager::LanguageInfo> LocalizationManager::MakeAvailableLanguages()
{
    std::vector<LanguageInfo> languages;

    //
    // Enumerate all directories under our "languages" root
    //

    for (auto const & entryIt : std::filesystem::directory_iterator(ResourceLocator::GetLanguagesRootPath()))
    {
        if (std::filesystem::is_directory(entryIt.path()))
        {
            // Make sure it's recognized by wxWidgets as a language
            std::string const languageName = entryIt.path().stem().string();
            auto const wxLangInfo = wxLocale::FindLanguageInfo(languageName);
            if (wxLangInfo == nullptr)
            {
                LogMessage("WARNING: language directory \"", languageName, "\" is not a recognized language");
            }
            else
            {
                // Accepted as a valid language
                languages.emplace_back(
                    wxLangInfo->Description.ToStdString(),
                    MakeLanguageIdentifier(wxLangInfo->CanonicalName),
                    static_cast<wxLanguage>(wxLangInfo->Language));
            }
        }
    }

    //
    // Add the language of our msgid's
    //

    languages.emplace_back(MakeDefaultLanguage());

    //
    // Sort and distinct
    //

    std::sort(
        languages.begin(),
        languages.end(),
        [](auto const & lhs, auto const & rhs)
        {
            return lhs.Name < rhs.Name;
        });

    languages.erase(
        std::unique(
            languages.begin(),
            languages.end(),
            [](auto const & lhs, auto const & rhs)
            {
                return lhs.Name == rhs.Name;
            }),
        languages.end());

    return languages;
}

LocalizationManager::LanguageInfo LocalizationManager::MakeDefaultLanguage()
{
    auto const wxEnLangInfo = wxLocale::GetLanguageInfo(TranslationsMsgIdLangId);
    assert(nullptr != wxEnLangInfo);

    return LanguageInfo(
        wxEnLangInfo->Description.ToStdString(),
        wxEnLangInfo->CanonicalName.ToStdString(),
        TranslationsMsgIdLangId);
}

LocalizationManager::LanguageInfo const * LocalizationManager::FindLanguageInfo(
    wxLanguage languageId,
    std::vector<LanguageInfo> const & availableLanguages)
{
    auto const it = std::find_if(
        availableLanguages.cbegin(),
        availableLanguages.cend(),
        [&languageId](auto const & al)
        {
            return al.LanguageId == languageId;
        });

    if (it != availableLanguages.cend())
        return &(*it);
    else
        return nullptr;
}

LocalizationManager::LanguageInfo const * LocalizationManager::FindLanguageInfo(
    std::string const & languageIdentifier,
    std::vector<LanguageInfo> const & availableLanguages)
{
    auto const it = std::find_if(
        availableLanguages.cbegin(),
        availableLanguages.cend(),
        [&languageIdentifier](auto const & al)
        {
            return al.Identifier == languageIdentifier;
        });

    if (it != availableLanguages.cend())
        return &(*it);
    else
        return nullptr;
}