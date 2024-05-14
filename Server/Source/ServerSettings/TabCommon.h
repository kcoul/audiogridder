/*
 * Copyright (c) 2024 Andreas Pohl
 * Licensed under MIT (https://github.com/apohl79/audiogridder/blob/master/COPYING)
 *
 * Author: Kieran Coulter
 */

#pragma once

static int totalWidth = 600;

#ifdef JUCE_LINUX
static int totalHeight = 100;
static int extraBorderTB = 20;
#else
static int totalHeight = 80;
static int extraBorderTB = 0;
#endif

static int borderLR = 15;  // left/right border
static int borderTB = 15;  // top/bottom border
static int rowHeight = 30;
static int fieldWidth = 50;
static int wideFieldWidth = 250;
static int fieldHeight = 25;
static int labelWidth = 350;
static int labelHeight = 35;
static int headerHeight = 18;
static int checkBoxWidth = 25;
static int checkBoxHeight = 25;
static int largeFieldRows = 2;
static int largeFieldWidth = 250;
static int largeFieldHeight = largeFieldRows * rowHeight - 10;

static auto getLabelBounds = [](int r) {
    return juce::Rectangle<int>(borderLR, extraBorderTB + borderTB + r * rowHeight, labelWidth, labelHeight);
};
static auto getFieldBounds = [](int r) {
    return juce::Rectangle<int>(totalWidth - fieldWidth - borderLR, extraBorderTB + borderTB + r * rowHeight + 3,
                                fieldWidth, fieldHeight);
};
static auto getWideFieldBounds = [](int r) {
    return juce::Rectangle<int>(totalWidth - wideFieldWidth - borderLR, extraBorderTB + borderTB + r * rowHeight + 3,
                                wideFieldWidth, fieldHeight);
};
static auto getCheckBoxBounds = [](int r) {
    return juce::Rectangle<int>(totalWidth - checkBoxWidth - borderLR, extraBorderTB + borderTB + r * rowHeight + 3,
                                checkBoxWidth, checkBoxHeight);
};
static auto getIOCheckBoxBounds = [](int r) {
    return juce::Rectangle<int>(totalWidth - checkBoxWidth - 2*borderLR, extraBorderTB + borderTB + r * rowHeight + 3,
                                checkBoxWidth, checkBoxHeight);
};
static auto getLargeFieldBounds = [](int r) {
    return juce::Rectangle<int>(totalWidth - largeFieldWidth - borderLR, extraBorderTB + borderTB + r * rowHeight + 3,
                                largeFieldWidth, largeFieldHeight);
};
static auto getHeaderBounds = [](int r) {
    return juce::Rectangle<int>(borderLR, extraBorderTB + borderTB + r * rowHeight + 7, totalWidth - borderLR * 2,
                                headerHeight);
};

struct MainSettings {
    const String& name;
    int id;
    int mode;
};

struct FormatSettings {
    bool au;
    bool vst3;
    bool vst2;
    bool vst2NoStandard;
    bool lv2;
    const StringArray& vst3Folders;
    const StringArray& vst2Folders;
    const StringArray& lv2Folders;
};

struct CaptureSettings {
    bool capOff;
    bool localMode;
    bool capFFmpeg;
    bool diffDetect;
    bool winOnTop;
    int FFmpegQuality;
    int offsetX;
    int offsetY;
    float screenQuality;
};

struct IOSettings {
    bool enableNativeIO;
    AudioDeviceManager& deviceManager;
    int minAudioInputChannels;
    int maxAudioInputChannels;
    int minAudioOutputChannels;
    int maxAudioOutputChannels;
    bool showMidiInputOptions;
    bool showMidiOutputSelector;
    bool showChannelsAsStereoPairs;
    bool hideAdvancedOptionsWithButton;
};