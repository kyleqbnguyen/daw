/*
    DAW - An open-source digital audio workstation
    Copyright (C) 2026 DAW Project Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "main_component.h"

MainComponent::MainComponent() {
  setSize(1280, 720);
}

MainComponent::~MainComponent() {}

void MainComponent::paint(juce::Graphics& g) {
  g.fillAll(juce::Colour(0xFF1E1E2E));

  g.setColour(juce::Colour(0xFFCDD6F4));
  g.setFont(juce::FontOptions(24.0f));
  g.drawText("DAW", getLocalBounds(), juce::Justification::centred, true);

  g.setColour(juce::Colour(0xFF6C7086));
  g.setFont(juce::FontOptions(14.0f));
  g.drawText("Tracktion Engine v" + tracktion::engine::Engine::getVersion(),
             getLocalBounds().translated(0, 40), juce::Justification::centred,
             true);
}

void MainComponent::resized() {}
