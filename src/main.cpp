#include "main.hpp"
#include "ModConfig.hpp"

#include "GlobalNamespace/BeatmapObjectSpawnMovementData.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "GlobalNamespace/NoteController.hpp"
#include "GlobalNamespace/ScoreController.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/LevelCompletionResults.hpp"
#include "GlobalNamespace/ResultsViewController.hpp"
#include "GlobalNamespace/NoteCutInfo.hpp"
#include "GlobalNamespace/PlatformLeaderboardViewController.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "GlobalNamespace/AudioTimeSyncController.hpp"
#include "GlobalNamespace/FileHelpers.hpp"

#include "config-utils/shared/config-utils.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/QuestUI.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Canvas.hpp"
#include "UnityEngine/CanvasRenderer.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Quaternion.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/RenderMode.hpp"
#include "UnityEngine/UI/CanvasScaler.hpp"
#include "UnityEngine/CanvasRenderer.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/SceneManagement/Scene.hpp"
#include "UnityEngine/Cloth.hpp"
#include "UnityEngine/PrimitiveType.hpp"

#include "HMUI/CurvedCanvasSettings.hpp"
#include "HMUI/Touchable.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"

#include "System/Action.hpp"

#include <fstream>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <string>

using namespace QuestUI;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace GlobalNamespace;
using namespace HMUI;

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup
DEFINE_CONFIG(ModConfig);

UnityEngine::Canvas* canvas;
UnityEngine::UI::VerticalLayoutGroup* layout;

Button* button = nullptr;
Button* button1 = nullptr;

UnityEngine::GameObject* screen;
UnityEngine::GameObject* screen1;

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
	static Configuration config(modInfo);
	config.Load();
	return config;
}

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
	static Logger* logger = new Logger(modInfo);
	return *logger;
}

std::vector<std::string> getFiles(std::string_view path) {
    std::vector<std::string> directories;
    if(!std::filesystem::is_directory(path))
        return directories;
    std::error_code ec;
    auto directory_iterator = std::filesystem::directory_iterator(path, std::filesystem::directory_options::none, ec);
    if (ec) {
        getLogger().info("Error reading directory at %s: %s", path.data(), ec.message().c_str());
    }
    for (const auto& entry : directory_iterator) {
        if(entry.is_regular_file())
            directories.push_back(entry.path().string());
    }
    return directories;
}

std::string RemoveExtension(std::string path)
{
    if(path.find_last_of(".") != std::string::npos)
        return path.substr(0, path.find_last_of("."));
    return path;
}

std::string GetFileName(std::string path, bool removeExtension)
{
    std::string result = "";
    if(path.find_last_of("/") != std::string::npos)
        result = path.substr(path.find_last_of("/")+1);
    else result = path;
    if (removeExtension) result = RemoveExtension(result);
    return result;;
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
	info.id = ID;
	info.version = VERSION;
	modInfo = info;
	
	getConfig().Load(); // Load the config file
	getLogger().info("Completed setup!");
}

MAKE_HOOK_MATCH(AudioTimeSyncController_StartSong, &AudioTimeSyncController::StartSong, void,
    AudioTimeSyncController* self,
    float startTimeOffset
) {
    AudioTimeSyncController_StartSong(self, startTimeOffset);

    if(getModConfig().Active.GetValue() && getModConfig().ShowInGame.GetValue()){
        UnityEngine::GameObject* screen = BeatSaberUI::CreateFloatingScreen(
            {120, 240},
            {getModConfig().X.GetValue() + 15.0f, getModConfig().Y.GetValue() + 17.0f, 25.0},
            {0, 0, 0},
            0.0f,
            false,
            false,
            1
        );

        UnityEngine::GameObject* screen1 = BeatSaberUI::CreateFloatingScreen(
            {120, 240},
            {(getModConfig().X.GetValue() * -1) - 15.0f, getModConfig().Y.GetValue() + 17.0f, 25.0},
            {0, 0, 0},
            0.0f,
            false,
            false,
            1
        );

        // UnityEngine::GameObject* plane = UnityEngine::GameObject::CreatePrimitive(UnityEngine::PrimitiveType::Plane);
        // plane->get_transform()->set_localPosition({ 1, 1, 1 });

        // auto cloth2 = plane->get_gameObject()->GetComponent<Cloth*>();
        // if (!cloth2) cloth2 = plane->get_gameObject()->AddComponent<Cloth*>();

        // cloth2->set_externalAcceleration({0, 0, -1});

        getLogger().info("Created Screens");

        auto cloth = screen->get_gameObject()->GetComponent<Cloth*>();
        if (!cloth) cloth = screen->get_gameObject()->AddComponent<Cloth*>();

        auto cloth1 = screen1->get_gameObject()->GetComponent<Cloth*>();
        if (!cloth1) cloth1 = screen1->get_gameObject()->AddComponent<Cloth*>();

        cloth->set_externalAcceleration({0, 0, -1});  
        cloth1->set_externalAcceleration({0, 0, -1});   
        
        // cloth->selfCollision = true;
        // cloth->useContinuousCollision = 0.0f;

        // cloth1->selfCollision = true;
        // cloth1->useContinuousCollision = 0.0f;

        std::string lpath = getModConfig().RightBanner.GetValue();
        auto lsprite_active = QuestUI::BeatSaberUI::FileToSprite(lpath);
        std::string lotherPath = getModConfig().RightBanner.GetValue();
        auto lsprite_inactive = QuestUI::BeatSaberUI::FileToSprite(lotherPath);

        std::string rpath = getModConfig().LeftBanner.GetValue();
        auto rsprite_active = QuestUI::BeatSaberUI::FileToSprite(rpath);
        std::string rotherPath = getModConfig().LeftBanner.GetValue();
        auto rsprite_inactive = QuestUI::BeatSaberUI::FileToSprite(rotherPath);

        getLogger().info("Loaded Imaged");

        auto button12 = QuestUI::BeatSaberUI::CreateUIButton(screen->get_transform(), "", "SettingsButton", {0, 0}, {10, 10}, nullptr);
        BeatSaberUI::SetButtonSprites(button12, lsprite_inactive, lsprite_active);

        auto button11 = QuestUI::BeatSaberUI::CreateUIButton(screen1->get_transform(), "", "SettingsButton", {0, 0}, {10, 10}, nullptr);
        BeatSaberUI::SetButtonSprites(button11, rsprite_inactive, rsprite_active);

        button12->get_transform()->set_localScale({16.0f, 16.0f, 16.0f});
        button12->set_interactable(false);

        button11->get_transform()->set_localScale({16.0f, 16.0f, 16.0f});
        button11->set_interactable(false);
    }
}

MAKE_HOOK_MATCH(MainMenuViewController_DidActivate, &MainMenuViewController::DidActivate, void,
    MainMenuViewController* self,
    bool firstActivation,
    bool addedToHierarchy,
    bool screenSystemEnabling
) {
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    // if(getModConfig().DeviceID.GetValue() == ""){
    //     auto json = WebUtils::GetJSON("http://192.168.11.22/api/v1/quest/register/custombanners");

    //     if(json->IsObject()){
    //         auto jsonObject = json->GetObject();

    //         if(jsonObject.HasMember("id") && jsonObject["id"].IsString()){
    //             getModConfig().DeviceID.SetValue(jsonObject["id"].GetString());
    //         }
    //     }
    // }

    if(!direxists("/sdcard/Pictures/banners/")){
        int makePath = mkpath("/sdcard/Pictures/banners/");
        if (makePath == -1){
            getLogger().error("Failed to make Folder!");
        }
    }

    if (firstActivation && getModConfig().Active.GetValue()) {
        screen = BeatSaberUI::CreateFloatingScreen(
            {120, 240},
            {getModConfig().X.GetValue() + 15.0f, getModConfig().Y.GetValue() + 17.0f, 25.0},
            {0, 0, 0},
            0.0f,
            false,
            false,
            1
        );

        screen1 = BeatSaberUI::CreateFloatingScreen(
            {120, 240},
            {(getModConfig().X.GetValue() * -1) - 15.0f, getModConfig().Y.GetValue() + 17.0f, 25.0},
            {0, 0, 0},
            0.0f,
            false,
            false,
            1
        );

        // UnityEngine::GameObject* plane = UnityEngine::GameObject::CreatePrimitive(UnityEngine::PrimitiveType::Plane);
        // plane->get_transform()->set_localPosition({ 1, 1, 1 });

        // auto cloth2 = plane->get_gameObject()->GetComponent<Cloth*>();
        // if (!cloth2) cloth2 = plane->get_gameObject()->AddComponent<Cloth*>();

        // cloth2->set_externalAcceleration({0, 0, -1});

        getLogger().info("Created Screens");

        auto cloth = screen->get_gameObject()->GetComponent<Cloth*>();
        if (!cloth) cloth = screen->get_gameObject()->AddComponent<Cloth*>();

        auto cloth1 = screen1->get_gameObject()->GetComponent<Cloth*>();
        if (!cloth1) cloth1 = screen1->get_gameObject()->AddComponent<Cloth*>();

        cloth->set_externalAcceleration({0, 0, -1});  
        cloth1->set_externalAcceleration({0, 0, -1});   
        
        // cloth->selfCollision = true;
        // cloth->useContinuousCollision = 0.0f;

        // cloth1->selfCollision = true;
        // cloth1->useContinuousCollision = 0.0f;

        std::string lpath = getModConfig().RightBanner.GetValue();
        auto lsprite_active = QuestUI::BeatSaberUI::FileToSprite(lpath);
        std::string lotherPath = getModConfig().RightBanner.GetValue();
        auto lsprite_inactive = QuestUI::BeatSaberUI::FileToSprite(lotherPath);

        std::string rpath = getModConfig().LeftBanner.GetValue();
        auto rsprite_active = QuestUI::BeatSaberUI::FileToSprite(rpath);
        std::string rotherPath = getModConfig().LeftBanner.GetValue();
        auto rsprite_inactive = QuestUI::BeatSaberUI::FileToSprite(rotherPath);

        getLogger().info("Loaded Imaged");

        button = QuestUI::BeatSaberUI::CreateUIButton(screen->get_transform(), "", "SettingsButton", {0, 0}, {10, 10}, nullptr);
        BeatSaberUI::SetButtonSprites(button, lsprite_inactive, lsprite_active);

        button1 = QuestUI::BeatSaberUI::CreateUIButton(screen1->get_transform(), "", "SettingsButton", {0, 0}, {10, 10}, nullptr);
        BeatSaberUI::SetButtonSprites(button1, rsprite_inactive, rsprite_active);

        button->get_transform()->set_localScale({16.0f, 16.0f, 16.0f});
        button->set_interactable(false);

        button1->get_transform()->set_localScale({16.0f, 16.0f, 16.0f});
        button1->set_interactable(false);
    }
};

std::string linkCode = "";

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    if(firstActivation){
        self->get_gameObject()->AddComponent<HMUI::Touchable*>();
        UnityEngine::GameObject* settings = BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());

        std::vector<std::string> files = getFiles("/sdcard/Pictures/banners/");

        getLogger().info("%lu files found", files.size());

        HMUI::ModalView* settingsModal = BeatSaberUI::CreateModal(settings->get_transform(), UnityEngine::Vector2(120.0f, 80.0f), [](HMUI::ModalView* modal){}, true);
        UnityEngine::GameObject* settingsm = BeatSaberUI::CreateScrollableModalContainer(settingsModal);

        // TMPro::TextMeshProUGUI* userText = BeatSaberUI::CreateText(settingsm->get_transform(), "PhazeSaber Link Settings");

        // if(getModConfig().DeviceID.GetValue() == ""){
        //     auto keyboard = BeatSaberUI::CreateStringSetting(settingsm->get_transform(), "Code", "", Vector2(0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), 
        //         [&](std::string_view value){
        //             linkCode = value.data();

        //             std::transform(linkCode.begin(), linkCode.end(), linkCode.begin(), toupper);

        //             getLogger().info("%s", linkCode.c_str());
        //         });

        //     BeatSaberUI::CreateUIButton(settingsm->get_transform(), "Link", "PlayButton", Vector2(0.0f, 0.0f), Vector2(10.0f, 10.0f), [userText, keyboard]() {
        //         getLogger().info("%s", linkCode.c_str());
        //         std::string url = "http://192.168.11.22/api/v1/quest/claim/" + linkCode + "/" + getModConfig().DeviceID.GetValue();
        //         auto json = WebUtils::GetJSON(url);

        //         getLogger().info("%s", url.c_str());

        //         if(json->IsObject()){
        //             auto jsonObject = json->GetObject();

        //             if(jsonObject.HasMember("success") && jsonObject["success"].IsBool()){
        //                 if(jsonObject["success"].GetBool()){
        //                     getLogger().info("%s", jsonObject["username"].GetString());

        //                     userText->set_text(il2cpp_utils::newcsstr(jsonObject["username"].GetString()));

        //                     GameObject::Destroy(keyboard);
        //                 } else{
        //                     getLogger().info("%s", jsonObject["error"].GetString());

        //                     userText->set_text(il2cpp_utils::newcsstr(jsonObject["error"].GetString()));
        //                 }
        //             }
        //         }
        //     });
        // }
        
        BeatSaberUI::CreateText(settingsm->get_transform(), "Qustom Banners Settings");

        BeatSaberUI::CreateToggle(settingsm->get_transform(), "Active", getModConfig().Active.GetValue(),
            [](bool value) { 
                getModConfig().Active.SetValue(value);
            });

        BeatSaberUI::CreateToggle(settingsm->get_transform(), "Show In Song", getModConfig().ShowInGame.GetValue(),
            [](bool value) { 
                getModConfig().ShowInGame.SetValue(value);
            });

        BeatSaberUI::CreateIncrementSetting(settingsm->get_transform(), "X Position", 2, 0.5f, getModConfig().X.GetValue(), -15.0f, 15.0f,
            [](float value) { 
                getModConfig().X.SetValue(value);
            });

        BeatSaberUI::CreateIncrementSetting(settingsm->get_transform(), "Y Position", 2, 0.5f, getModConfig().Y.GetValue(), -15.0f, 15.0f,
            [](float value) { 
                getModConfig().Y.SetValue(value);
            });

        BeatSaberUI::CreateUIButton(settingsm->get_transform(), "Save",
            [&]() {
                GameObject::Destroy(screen);
                GameObject::Destroy(screen1);

                if(getModConfig().Active.GetValue()){
                    screen = BeatSaberUI::CreateFloatingScreen(
                        {120, 240},
                        {getModConfig().X.GetValue() + 15.0f, getModConfig().Y.GetValue() + 17.0f, 25.0},
                        {0, 0, 0},
                        0.0f,
                        false,
                        false,
                        1
                    );

                    auto cloth = screen->get_gameObject()->GetComponent<Cloth*>();
                    if (!cloth) cloth = screen->get_gameObject()->AddComponent<Cloth*>();

                    cloth->set_externalAcceleration({0, 0, -1});
                    // cloth->selfCollision = true;
                    // cloth->useContinuousCollision = 0.0f;

                    std::string path = getModConfig().LeftBanner.GetValue();
                    auto sprite_active = QuestUI::BeatSaberUI::FileToSprite(path);
                    std::string otherPath = getModConfig().LeftBanner.GetValue();
                    auto sprite_inactive = QuestUI::BeatSaberUI::FileToSprite(otherPath);

                    button = QuestUI::BeatSaberUI::CreateUIButton(screen->get_transform(), "", "SettingsButton", {0, 0}, {10, 10}, nullptr);
                    BeatSaberUI::SetButtonSprites(button, sprite_inactive, sprite_active);

                    button->get_transform()->set_localScale({16.0f, 16.0f, 16.0f});
                    button->set_interactable(false);

                    screen1 = BeatSaberUI::CreateFloatingScreen(
                        {120, 240},
                        {(getModConfig().X.GetValue() * -1) - 15.0f, getModConfig().Y.GetValue() + 17.0f, 25.0},
                        {0, 0, 0},
                        0.0f,
                        false,
                        false,
                        1
                    );

                    auto cloth1 = screen1->get_gameObject()->GetComponent<Cloth*>();
                    if (!cloth1) cloth1 = screen1->get_gameObject()->AddComponent<Cloth*>();

                    cloth1->set_externalAcceleration({0, 0, -1});
                    // cloth1->selfCollision = true;
                    // cloth1->useContinuousCollision = 0.0f;

                    std::string path1 = getModConfig().RightBanner.GetValue();
                    auto sprite_active1 = QuestUI::BeatSaberUI::FileToSprite(path1);
                    std::string otherPath1 = getModConfig().RightBanner.GetValue();
                    auto sprite_inactive1 = QuestUI::BeatSaberUI::FileToSprite(otherPath1);

                    button1 = QuestUI::BeatSaberUI::CreateUIButton(screen1->get_transform(), "", "SettingsButton", {0, 0}, {10, 10}, nullptr);
                    BeatSaberUI::SetButtonSprites(button1, sprite_inactive1, sprite_active1);

                    button1->get_transform()->set_localScale({16.0f, 16.0f, 16.0f});
                    button1->set_interactable(false);
                }
            });
            

        BeatSaberUI::CreateUIButton(settings->get_transform(), "Settings",
            [settingsModal]() {
                getLogger().info("Button Clicked");

                settingsModal->Show(true, true, nullptr);
            });

        for (auto &file : files) {
            getLogger().info("Banner %s", file.c_str());

            HorizontalLayoutGroup* levelBarLayout = BeatSaberUI::CreateHorizontalLayoutGroup(settings->get_transform());
            GameObject* prefab = levelBarLayout->get_gameObject();

            levelBarLayout->set_childControlWidth(false);
            levelBarLayout->set_childForceExpandWidth(true);

            LayoutElement* levelBarLayoutElement = levelBarLayout->GetComponent<LayoutElement*>();
            levelBarLayoutElement->set_minHeight(9.0f);
            levelBarLayoutElement->set_minWidth(90.0f);

            BeatSaberUI::CreateText(levelBarLayoutElement->get_transform(), GetFileName(file.c_str(), true), UnityEngine::Vector2(0.0f, 0.0f));

            HMUI::ModalView* modal = BeatSaberUI::CreateModal(settings->get_transform(), UnityEngine::Vector2(60.0f, 40.0f), [](HMUI::ModalView* modal){}, true);
            UnityEngine::GameObject* container = BeatSaberUI::CreateScrollableModalContainer(modal);

            BeatSaberUI::CreateText(container->get_transform(), GetFileName(file.c_str(), false));

            std::string path = file.c_str();
            auto sprite_active = QuestUI::BeatSaberUI::FileToSprite(path);
            std::string otherPath = file.c_str();
            auto sprite_inactive = QuestUI::BeatSaberUI::FileToSprite(otherPath);

            Button* previewBtn = BeatSaberUI::CreateUIButton(container->get_transform(), "", "SettingsButton", {0, 0}, {10, 20}, nullptr);
            BeatSaberUI::SetButtonSprites(previewBtn, sprite_inactive, sprite_active);

            previewBtn->get_transform()->set_localScale({1.0f, 1.0f, 1.0f});

            BeatSaberUI::CreateUIButton(container->get_transform(), "Set Both Banners", "PlayButton", Vector2(0.0f, 0.0f), Vector2(24.0f, 15.0f), [file]() {
                getLogger().info("Set Both Banners: %s", file.c_str());

                getModConfig().LeftBanner.SetValue(file.c_str());
                getModConfig().RightBanner.SetValue(file.c_str());

                GameObject::Destroy(screen);
                GameObject::Destroy(screen1);

                screen = BeatSaberUI::CreateFloatingScreen(
                    {120, 240},
                    {getModConfig().X.GetValue() + 15.0f, getModConfig().Y.GetValue() + 17.0f, 25.0},
                    {0, 0, 0},
                    0.0f,
                    false,
                    false,
                    1
                );

                auto cloth = screen->get_gameObject()->GetComponent<Cloth*>();
                if (!cloth) cloth = screen->get_gameObject()->AddComponent<Cloth*>();

                cloth->set_externalAcceleration({0, 0, -1});
                // cloth->selfCollision = true;
                // cloth->useContinuousCollision = 0.0f;

                std::string path = getModConfig().RightBanner.GetValue();
                auto sprite_active = QuestUI::BeatSaberUI::FileToSprite(path);
                std::string otherPath = getModConfig().RightBanner.GetValue();
                auto sprite_inactive = QuestUI::BeatSaberUI::FileToSprite(otherPath);

                button = QuestUI::BeatSaberUI::CreateUIButton(screen->get_transform(), "", "SettingsButton", {0, 0}, {10, 10}, nullptr);
                BeatSaberUI::SetButtonSprites(button, sprite_inactive, sprite_active);

                button->get_transform()->set_localScale({16.0f, 16.0f, 16.0f});
                button->set_interactable(false);

                screen1 = BeatSaberUI::CreateFloatingScreen(
                    {120, 240},
                    {(getModConfig().X.GetValue() * -1) - 15.0f, getModConfig().Y.GetValue() + 17.0f, 25.0},
                    {0, 0, 0},
                    0.0f,
                    false,
                    false,
                    1
                );

                auto cloth1 = screen1->get_gameObject()->GetComponent<Cloth*>();
                if (!cloth1) cloth1 = screen1->get_gameObject()->AddComponent<Cloth*>();

                cloth1->set_externalAcceleration({0, 0, -1});
                // cloth1->selfCollision = true;
                // cloth1->useContinuousCollision = 0.0f;

                std::string path1 = getModConfig().LeftBanner.GetValue();
                auto sprite_active1 = QuestUI::BeatSaberUI::FileToSprite(path1);
                std::string otherPath1 = getModConfig().LeftBanner.GetValue();
                auto sprite_inactive1 = QuestUI::BeatSaberUI::FileToSprite(otherPath1);

                button1 = QuestUI::BeatSaberUI::CreateUIButton(screen1->get_transform(), "", "SettingsButton", {0, 0}, {10, 10}, nullptr);
                BeatSaberUI::SetButtonSprites(button1, sprite_inactive1, sprite_active1);

                button1->get_transform()->set_localScale({16.0f, 16.0f, 16.0f});
                button1->set_interactable(false);
            });

            BeatSaberUI::CreateUIButton(container->get_transform(), "Set Left Banner", "PlayButton", Vector2(0.0f, 0.0f), Vector2(24.0f, 15.0f), [file]() {
                getLogger().info("Set Left Banner: %s", file.c_str());

                getModConfig().LeftBanner.SetValue(file.c_str());

                GameObject::Destroy(screen1);

                screen1 = BeatSaberUI::CreateFloatingScreen(
                    {120, 240},
                    {(getModConfig().X.GetValue() * -1) - 15.0f, getModConfig().Y.GetValue() + 17.0f, 25.0},
                    {0, 0, 0},
                    0.0f,
                    false,
                    false,
                    1
                );

                auto cloth1 = screen1->get_gameObject()->GetComponent<Cloth*>();
                if (!cloth1) cloth1 = screen1->get_gameObject()->AddComponent<Cloth*>();

                cloth1->set_externalAcceleration({0, 0, -1});
                // cloth1->selfCollision = true;
                // cloth1->useContinuousCollision = 0.0f;

                std::string path1 = getModConfig().LeftBanner.GetValue();
                auto sprite_active1 = QuestUI::BeatSaberUI::FileToSprite(path1);
                std::string otherPath1 = getModConfig().LeftBanner.GetValue();
                auto sprite_inactive1 = QuestUI::BeatSaberUI::FileToSprite(otherPath1);

                button1 = QuestUI::BeatSaberUI::CreateUIButton(screen1->get_transform(), "", "SettingsButton", {0, 0}, {10, 10}, nullptr);
                BeatSaberUI::SetButtonSprites(button1, sprite_inactive1, sprite_active1);

                button1->get_transform()->set_localScale({16.0f, 16.0f, 16.0f});
                button1->set_interactable(false);
            });

            BeatSaberUI::CreateUIButton(container->get_transform(), "Set Right Banner", "PlayButton", Vector2(0.0f, 0.0f), Vector2(24.0f, 15.0f), [file]() {
                getLogger().info("Set Right Banner: %s", file.c_str());

                getModConfig().RightBanner.SetValue(file.c_str());

                GameObject::Destroy(screen);

                screen = BeatSaberUI::CreateFloatingScreen(
                    {120, 240},
                    {getModConfig().X.GetValue() + 15.0f, getModConfig().Y.GetValue() + 17.0f, 25.0},
                    {0, 0, 0},
                    0.0f,
                    false,
                    false,
                    1
                );

                auto cloth = screen->get_gameObject()->GetComponent<Cloth*>();
                if (!cloth) cloth = screen->get_gameObject()->AddComponent<Cloth*>();

                cloth->set_externalAcceleration({0, 0, -1});
                // cloth->selfCollision = true;
                // cloth->useContinuousCollision = 0.0f;

                std::string path = getModConfig().RightBanner.GetValue();
                auto sprite_active = QuestUI::BeatSaberUI::FileToSprite(path);
                std::string otherPath = getModConfig().RightBanner.GetValue();
                auto sprite_inactive = QuestUI::BeatSaberUI::FileToSprite(otherPath);

                button = QuestUI::BeatSaberUI::CreateUIButton(screen->get_transform(), "", "SettingsButton", {0, 0}, {10, 10}, nullptr);
                BeatSaberUI::SetButtonSprites(button, sprite_inactive, sprite_active);

                button->get_transform()->set_localScale({16.0f, 16.0f, 16.0f});
                button->set_interactable(false);
            });

            Button* usebutton = BeatSaberUI::CreateUIButton(levelBarLayoutElement->get_transform(), "", "PlayButton", Vector2(0.0f, 0.0f), Vector2(12.0f, 0.0f),
                [modal]() {
                    getLogger().info("Button Clicked");
                    
                    modal->Show(true, true, nullptr);
                });

            auto text = QuestUI::BeatSaberUI::CreateText(usebutton->get_transform(), "Use");
            text->set_alignment(TMPro::TextAlignmentOptions::Center);
        }
    } else{
        
    };
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
	il2cpp_functions::Init();
	getModConfig().Init(modInfo);

	LoggerContextObject logger = getLogger().WithContext("load");

	QuestUI::Init();
	QuestUI::Register::RegisterModSettingsViewController(modInfo, DidActivate);
    QuestUI::Register::RegisterMainMenuModSettingsViewController(modInfo, DidActivate);
    getLogger().info("Successfully installed Settings UI!");

	getLogger().info("Installing hooks...");
	INSTALL_HOOK(logger, MainMenuViewController_DidActivate);
    INSTALL_HOOK(logger, AudioTimeSyncController_StartSong);
	getLogger().info("Installed all hooks!");
}