/**
 * \file Test_ThemeSwitching.cpp
 * \brief Unit tests for UIColors theme switching behavior.
 * \author Xavier Wah-Huen Lusetti
 *
 * \details
 * Verifies theme system behavior for:
 * - UIColors::SetTheme transitions across Dark, Light, and Neon themes.
 * - Synchronization of global color accessors with the active Palette.
 * - Correctness of GetCurrentTheme tracking.
 * - Stability when toggling themes repeatedly.
 */

// Requires wxWidgets headers (wxColour). Only compiled when
// YTBLND_BUILD_UI_TESTS=ON.

#include "gtest/gtest.h"
#include "../UILayer/UIColors.hpp"

// Helper: ensure themes are loaded from the fixture file before each test
class ThemeSwitchTest : public ::testing::Test {
protected:
    void SetUp() override {
        UIColors::ThemeMap.clear();
        UIColors::ThemeOrder.clear();
        UIColors::Current = nullptr;

        wxString themePath = wxString::FromUTF8(YTBLND_SRC_DIR) + "/resources/theme.txt";
        bool loaded = UIColors::LoadThemesFromFile(themePath);
        ASSERT_TRUE(loaded) << "Failed to load theme.txt from: " << themePath;
    }
};

TEST_F(ThemeSwitchTest, LoadsThreeThemes) {
    EXPECT_EQ(UIColors::ThemeOrder.size(), 3u);
    EXPECT_EQ(UIColors::ThemeOrder[0], "Dark Mode");
    EXPECT_EQ(UIColors::ThemeOrder[1], "Light Mode");
    EXPECT_EQ(UIColors::ThemeOrder[2], "Neon Mode");
}

TEST_F(ThemeSwitchTest, DefaultsToDarkModeAfterLoad) {
    ASSERT_NE(UIColors::Current, nullptr);
    EXPECT_EQ(UIColors::Current->Name, "Dark Mode");
}

TEST_F(ThemeSwitchTest, SetThemeDarkUpdatesColors) {
    UIColors::SetTheme("Dark Mode");
    ASSERT_NE(UIColors::Current, nullptr);
    EXPECT_EQ(UIColors::Current->Name, "Dark Mode");
    EXPECT_EQ(UIColors::Background(), wxColour("#0D0D0D"));
    EXPECT_EQ(UIColors::Accent(), wxColour("#8A00FF"));
}

TEST_F(ThemeSwitchTest, SetThemeLightUpdatesColors) {
    UIColors::SetTheme("Light Mode");
    ASSERT_NE(UIColors::Current, nullptr);
    EXPECT_EQ(UIColors::Current->Name, "Light Mode");
    EXPECT_EQ(UIColors::Background(), wxColour("#F5F5F5"));
    EXPECT_EQ(UIColors::Accent(), wxColour("#0066CC"));
    EXPECT_EQ(UIColors::TextPrimary(), wxColour("#212121"));
}

TEST_F(ThemeSwitchTest, SetThemeNeonUpdatesColors) {
    UIColors::SetTheme("Neon Mode");
    ASSERT_NE(UIColors::Current, nullptr);
    EXPECT_EQ(UIColors::Current->Name, "Neon Mode");
    EXPECT_EQ(UIColors::Background(), wxColour("#000000"));
    EXPECT_EQ(UIColors::Accent(), wxColour("#39FF14"));
    EXPECT_EQ(UIColors::Danger(), wxColour("#FF0099"));
}

TEST_F(ThemeSwitchTest, SwitchingBackAndForthPreservesValues) {
    UIColors::SetTheme("Light Mode");
    wxColour lightBg = UIColors::Background();

    UIColors::SetTheme("Neon Mode");
    EXPECT_NE(UIColors::Background(), lightBg);

    UIColors::SetTheme("Light Mode");
    EXPECT_EQ(UIColors::Background(), lightBg);
}

TEST_F(ThemeSwitchTest, AllTenGettersWorkForNeon) {
    UIColors::SetTheme("Neon Mode");
    EXPECT_EQ(UIColors::Background(),    wxColour("#000000"));
    EXPECT_EQ(UIColors::Surface(),       wxColour("#0F0F0F"));
    EXPECT_EQ(UIColors::SurfaceRaised(), wxColour("#1E1E1E"));
    EXPECT_EQ(UIColors::Accent(),        wxColour("#39FF14"));
    EXPECT_EQ(UIColors::AccentHover(),   wxColour("#66FF00"));
    EXPECT_EQ(UIColors::TextPrimary(),   wxColour("#FFFFFF"));
    EXPECT_EQ(UIColors::TextSecondary(), wxColour("#00FFFF"));
    EXPECT_EQ(UIColors::TextMuted(),     wxColour("#008A8A"));
    EXPECT_EQ(UIColors::Separator(),     wxColour("#39FF14"));
    EXPECT_EQ(UIColors::Danger(),        wxColour("#FF0099"));
}

TEST_F(ThemeSwitchTest, FallbackDarkModeWhenNoFileLoaded) {
    UIColors::ThemeMap.clear();
    UIColors::ThemeOrder.clear();
    UIColors::Current = nullptr;

    UIColors::SetTheme("Dark Mode");
    ASSERT_NE(UIColors::Current, nullptr);
    EXPECT_EQ(UIColors::Current->Name, "Dark Mode");
    EXPECT_EQ(UIColors::Accent(), wxColour("#8A00FF"));
}

TEST_F(ThemeSwitchTest, UnknownThemeDoesNotCrash) {
    Palette* before = UIColors::Current;
    UIColors::SetTheme("Nonexistent Theme");
    EXPECT_EQ(UIColors::Current, before);
}
