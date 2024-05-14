/*
* Copyright (c) 2024 Andreas Pohl
* Licensed under MIT (https://github.com/apohl79/audiogridder/blob/master/COPYING)
*
* Author: Kieran Coulter
 */

#include "IOTab.hpp"

namespace e47 {

IOTab::IOTab(IOSettings ioSettings) : m_deviceSelector(ioSettings)
{
    int row = 0;

    m_nativeIOLbl.setText("Enable Native I/O:", NotificationType::dontSendNotification);
    m_nativeIOLbl.setBounds(getLabelBounds(row));
    addAndMakeVisible(m_nativeIOLbl);

    m_nativeIO.setBounds(getIOCheckBoxBounds(row));
    m_nativeIO.setToggleState(ioSettings.enableNativeIO, NotificationType::dontSendNotification);
    m_nativeIO.onClick = [this] { m_deviceSelector.toggleAudioSettings(m_nativeIO.getToggleState()); };
    addAndMakeVisible(m_nativeIO);

    addAndMakeVisible(m_deviceSelector);
}

void IOTab::paint(Graphics& g)
{
    auto bgColour = LookAndFeel::getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId);
    g.setColour(bgColour);
}

void IOTab::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(2*getLabelBounds(0).getHeight()); //TODO: Fixme: Hacky way to mix two approaches to partitioning GUI
    m_deviceSelector.setBounds(bounds);
}

}  // namespace e47