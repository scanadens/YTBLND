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

TEST(ThemeSwitchTest, SetThemeDarkUpdatesGlobals) {
    UIColors::SetTheme(ThemeType::Dark);
    EXPECT_EQ(UIColors::Current, &UIColors::DarkMode);
    EXPECT_EQ(UIColors::Background(), UIColors::DarkMode.Background);
    EXPECT_EQ(UIColors::Accent(), UIColors::DarkMode.Accent);
    EXPECT_EQ(UIColors::GetCurrentTheme(), ThemeType::Dark);
}

TEST(ThemeSwitchTest, SetThemeLightUpdatesGlobals) {
    UIColors::SetTheme(ThemeType::Light);
    EXPECT_EQ(UIColors::Current, &UIColors::LightMode);
    EXPECT_EQ(UIColors::Background(), UIColors::LightMode.Background);
    EXPECT_EQ(UIColors::Accent(), UIColors::LightMode.Accent);
    EXPECT_EQ(UIColors::TextPrimary(), UIColors::LightMode.TextPrimary);
    EXPECT_EQ(UIColors::GetCurrentTheme(), ThemeType::Light);
}

TEST(ThemeSwitchTest, SetThemeNeonUpdatesGlobals) {
    UIColors::SetTheme(ThemeType::Neon);
    EXPECT_EQ(UIColors::Current, &UIColors::NeonMode);
    EXPECT_EQ(UIColors::Background(), UIColors::NeonMode.Background);
    EXPECT_EQ(UIColors::Danger(), UIColors::NeonMode.Danger);
    EXPECT_EQ(UIColors::GetCurrentTheme(), ThemeType::Neon);
}

TEST(ThemeSwitchTest, SwitchingBackAndForthPreservesValues) {
    UIColors::SetTheme(ThemeType::Light);
    wxColour lightBg = UIColors::Background();

    UIColors::SetTheme(ThemeType::Neon);
    EXPECT_NE(UIColors::Background(), lightBg);

    UIColors::SetTheme(ThemeType::Light);
    EXPECT_EQ(UIColors::Background(), lightBg);

    UIColors::SetTheme(ThemeType::Dark);
}

TEST(ThemeSwitchTest, GetCurrentThemeTracksChanges) {
    UIColors::SetTheme(ThemeType::Dark);
    EXPECT_EQ(UIColors::GetCurrentTheme(), ThemeType::Dark);

    UIColors::SetTheme(ThemeType::Light);
    EXPECT_EQ(UIColors::GetCurrentTheme(), ThemeType::Light);

    UIColors::SetTheme(ThemeType::Neon);
    EXPECT_EQ(UIColors::GetCurrentTheme(), ThemeType::Neon);

    UIColors::SetTheme(ThemeType::Dark);
}

TEST(ThemeSwitchTest, AllTenGlobalsUpdateOnSwitch) {
    UIColors::SetTheme(ThemeType::Neon);
    const Palette& n = UIColors::NeonMode;
    EXPECT_EQ(UIColors::Background(),    n.Background);
    EXPECT_EQ(UIColors::Surface(),       n.Surface);
    EXPECT_EQ(UIColors::SurfaceRaised(), n.SurfaceRaised);
    EXPECT_EQ(UIColors::Accent(),        n.Accent);
    EXPECT_EQ(UIColors::AccentHover(),   n.AccentHover);
    EXPECT_EQ(UIColors::TextPrimary(),   n.TextPrimary);
    EXPECT_EQ(UIColors::TextSecondary(), n.TextSecondary);
    EXPECT_EQ(UIColors::TextMuted(),     n.TextMuted);
    EXPECT_EQ(UIColors::Separator(),     n.Separator);
    EXPECT_EQ(UIColors::Danger(),        n.Danger);

    UIColors::SetTheme(ThemeType::Dark);
}
