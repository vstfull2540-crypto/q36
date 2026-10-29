#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

ArmNongMuangLA2AAudioProcessorEditor::ArmNongMuangLA2AAudioProcessorEditor(ArmNongMuangLA2AAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p) {
    background = juce::ImageCache::getFromMemory(BinaryData::LA2A_Interface_png, BinaryData::LA2A_Interface_pngSize);
    // Start at 50% of the PNG native layout. All hit zones and controls scale in resized().
    setSize(749, 388);
    setResizable(true, true);
    setResizeLimits(375, 194, 2996, 1550);
    getConstrainer()->setFixedAspectRatio(1498.0 / 775.0);

    setupRotary(gain, "GAIN"); setupRotary(peak, "PEAK REDUCTION");
    setupRotary(attack, "ATTACK"); setupRotary(release, "RELEASE");
    addAndMakeVisible(gain); addAndMakeVisible(peak);
    // Attack/release remain automatable parameters but are not drawn on this LA-2A faceplate.
    attack.setVisible(false); release.setVisible(false);
    gain.setRange(0, 100, 0.1); peak.setRange(0, 100, 0.1);
    gain.setDoubleClickReturnValue(false, 50.0); peak.setDoubleClickReturnValue(false, 25.0); attack.setRange(0.1, 10, 0.01); release.setRange(50, 5000, 1);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, "gain", gain);
    peakAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, "peak", peak);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, "attack", attack);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, "release", release);

    addHitButton(topPower, "top power"); topPower.setTooltip("Power: ON / OFF"); topPower.onClick = [this] { setPower(!powerOn); };
    addHitButton(preset, "preset menu"); preset.onClick = [this] {
        juce::PopupMenu menu; menu.addItem(1, "Default (Gain 50 / Peak Reduction 25)"); menu.addItem(2, "Save current settings to A"); menu.addItem(3, "Save current settings to B");
        menu.showMenuAsync(juce::PopupMenu::Options(), [this](int id) { if (id == 1) { processor.parameters.getParameter("gain")->setValueNotifyingHost(0.50f); processor.parameters.getParameter("peak")->setValueNotifyingHost(0.25f); } else if (id == 2) { saveSnapshot(0); status.setText("Current settings saved to A", juce::dontSendNotification); } else if (id == 3) { saveSnapshot(1); status.setText("Current settings saved to B", juce::dontSendNotification); } });
    };
    addHitButton(modelPicker, "load NAM model"); modelPicker.onClick = [this] { loadModelDialog(); };
    addHitButton(buttonA, "snapshot A"); buttonA.onClick = [this] { restoreSnapshot(0); };
    addHitButton(buttonB, "snapshot B"); buttonB.onClick = [this] { restoreSnapshot(1); };
    addHitButton(copyAB, "copy A to B"); copyAB.onClick = [this] { saveSnapshot(0); saveSnapshot(1); status.setText("Snapshot A copied to B", juce::dontSendNotification); };
    addHitButton(settings, "settings"); settings.onClick = [this] {
        juce::PopupMenu menu; menu.addItem(1, "Reset controls"); menu.addItem(2, "Unload NAM model");
        menu.showMenuAsync(juce::PopupMenu::Options(), [this](int id) {
            if (id == 1) { processor.parameters.getParameter("gain")->setValueNotifyingHost(0.50f); processor.parameters.getParameter("peak")->setValueNotifyingHost(0.25f); }
            if (id == 2) { processor.unloadNamFile(); status.setText(processor.getNamStatus(), juce::dontSendNotification); repaint(); }
        });
    };
    addHitButton(topMeterSwitch, "top meter mode");
    topMeterSwitch.setLookAndFeel(&switchLookAndFeel);
    topMeterSwitch.setClickingTogglesState(true);
    topMeterSwitch.setTooltip("UP: OUTPUT | DOWN: GAIN REDUCTION");
    topMeterSwitch.onClick = [this] { cycleMeter(); };
    topMeterSwitch.onLeverDragState = [this](bool upper) {
        const int index = upper ? 1 : 2;
        processor.parameters.getParameter("meter")->setValueNotifyingHost(static_cast<float>(index) / 2.0f);
        meterSwitch.setToggleState(upper, juce::dontSendNotification);
        topMeterSwitch.setToggleState(upper, juce::dontSendNotification);
    };

    addHitButton(meterSwitch, "meter mode");
    meterSwitch.setLookAndFeel(&switchLookAndFeel);
    meterSwitch.setClickingTogglesState(true);
    meterSwitch.setTooltip("Pull UP: OUTPUT | Pull DOWN: GAIN REDUCTION");
    meterSwitch.onClick = [this] { cycleMeter(); };
    meterSwitch.onLeverDragState = [this](bool upper) {
        const int index = upper ? 1 : 2;
        processor.parameters.getParameter("meter")->setValueNotifyingHost(static_cast<float>(index) / 2.0f);
        meterSwitch.setToggleState(upper, juce::dontSendNotification);
        topMeterSwitch.setToggleState(upper, juce::dontSendNotification);
    };
    addHitButton(bottomPower, "bottom power switch");
    bottomPower.setLookAndFeel(&switchLookAndFeel);
    bottomPower.setClickingTogglesState(true);
    bottomPower.setTooltip("Pull UP: ON | Pull DOWN: OFF");
    bottomPower.onClick = [this] { setPower(!powerOn); bottomPower.setToggleState(powerOn, juce::dontSendNotification); };
    bottomPower.onLeverDragState = [this](bool upper) { setPower(upper); bottomPower.setToggleState(upper, juce::dontSendNotification); };
    addHitButton(limitSwitch, "limit compress switch");
    limitSwitch.setLookAndFeel(&switchLookAndFeel);
    limitSwitch.setClickingTogglesState(true);
    limitSwitch.setTooltip("Pull UP: LIMIT | Pull DOWN: COMPRESS");
    limitSwitch.onClick = [this] { setLimit(!limitOn); limitSwitch.setToggleState(limitOn, juce::dontSendNotification); };
    limitSwitch.onLeverDragState = [this](bool upper) { setLimit(upper); limitSwitch.setToggleState(upper, juce::dontSendNotification); };

    addAndMakeVisible(status); status.setColour(juce::Label::textColourId, juce::Colours::white);
    status.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    status.setJustificationType(juce::Justification::centred);
    status.setFont(juce::Font(juce::FontOptions(11.0f)));
    status.setMinimumHorizontalScale(0.65f);
    status.setText(processor.getNamStatus(), juce::dontSendNotification);
    addAndMakeVisible(switchStatus);
    // Live numeric readouts are separate labels so the knob face remains clean.
    for (auto* valueLabel : { &gainValueLabel, &peakValueLabel }) {
        addAndMakeVisible(*valueLabel);
        valueLabel->setJustificationType(juce::Justification::centred);
        // Numeric readout: black text, transparent background, and direct editing.
        // A single click opens the inline editor; Enter commits the value to the slider/APVTS.
        valueLabel->setColour(juce::Label::textColourId, juce::Colours::black);
        valueLabel->setColour(juce::Label::textWhenEditingColourId, juce::Colours::black);
        valueLabel->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        valueLabel->setColour(juce::Label::backgroundWhenEditingColourId, juce::Colours::white.withAlpha(0.92f));
        valueLabel->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
        valueLabel->setColour(juce::Label::outlineWhenEditingColourId, juce::Colours::darkgrey);
        valueLabel->setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
        valueLabel->setEditable(true, true, false);
        // JUCE Label::onEditorShow is a no-argument callback.
        // Retrieve the active editor from the Label instead of accepting an argument.
        valueLabel->onEditorShow = [valueLabel]() {
            if (auto* editor = valueLabel->getCurrentTextEditor())
                editor->selectAll();
        };
        valueLabel->onTextChange = [this, valueLabel]()
        {
            const auto text = valueLabel->getText().trim();
            if (text.isEmpty() || !text.containsOnly("+-0123456789.eE"))
                return;
            const double parsed = text.getDoubleValue();
            if (std::isfinite(parsed))
            {
                auto& target = (valueLabel == &gainValueLabel) ? gain : peak;
                target.setValue(juce::jlimit(target.getMinimum(), target.getMaximum(), parsed),
                                juce::sendNotificationSync);
            }
        };
    }
    switchStatus.setColour(juce::Label::textColourId, juce::Colours::white);
    switchStatus.setColour(juce::Label::backgroundColourId, juce::Colour(0xdd171717));
    switchStatus.setJustificationType(juce::Justification::centred);
    switchStatus.setFont(juce::Font(juce::FontOptions(12.0f)));
    powerOn = processor.parameters.getRawParameterValue("power")->load() >= 0.5f;
    limitOn = processor.parameters.getRawParameterValue("limit")->load() >= 0.5f;
    bottomPower.setToggleState(powerOn, juce::dontSendNotification);
    limitSwitch.setToggleState(limitOn, juce::dontSendNotification);
    meterSwitch.setToggleState(processor.getMeterMode() == 1, juce::dontSendNotification);
    topMeterSwitch.setToggleState(processor.getMeterMode() == 1, juce::dontSendNotification);
    saveSnapshot(0); saveSnapshot(1);
    smoothedVuDb = 0.0f;
    startTimerHz(60);
}

ArmNongMuangLA2AAudioProcessorEditor::~ArmNongMuangLA2AAudioProcessorEditor() {
    // Reset each component explicitly. A mixed initializer_list of TextButton*
    // and LeverButton* cannot deduce one auto* element type on MSVC (C3535/C2440).
    topPower.setLookAndFeel(nullptr);
    preset.setLookAndFeel(nullptr);
    modelPicker.setLookAndFeel(nullptr);
    buttonA.setLookAndFeel(nullptr);
    buttonB.setLookAndFeel(nullptr);
    copyAB.setLookAndFeel(nullptr);
    settings.setLookAndFeel(nullptr);
    topMeterSwitch.setLookAndFeel(nullptr);
    meterSwitch.setLookAndFeel(nullptr);
    bottomPower.setLookAndFeel(nullptr);
    limitSwitch.setLookAndFeel(nullptr);
    gain.setLookAndFeel(nullptr);
    peak.setLookAndFeel(nullptr);
    attack.setLookAndFeel(nullptr);
    release.setLookAndFeel(nullptr);
}

void ArmNongMuangLA2AAudioProcessorEditor::NumericRotarySlider::mouseDoubleClick(const juce::MouseEvent& event) {
    juce::ignoreUnused(event);

    // JUCE 8 no longer exposes AlertWindow::runModalLoop(). Use an asynchronous
    // modal dialog so the plug-in never blocks the host's message thread.
    auto* dialog = new juce::AlertWindow(
        "Enter parameter value",
        "Type a numeric value for " + getName(),
        juce::MessageBoxIconType::NoIcon);
    dialog->addTextEditor("numericValue", getTextFromValue(getValue()), "Value:");
    dialog->addButton("Apply", 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    juce::Component::SafePointer<NumericRotarySlider> safeSlider(this);
    juce::Component::SafePointer<juce::AlertWindow> safeDialog(dialog);
    dialog->enterModalState(true,
        juce::ModalCallbackFunction::create([safeSlider, safeDialog](int result) {
            if (result != 1 || safeSlider == nullptr || safeDialog == nullptr)
                return;

            const auto text = safeDialog->getTextEditorContents("numericValue").trim();
            if (text.isEmpty() || !text.containsOnly("+-0123456789.eE"))
                return;

            const double parsed = text.getDoubleValue();
            if (std::isfinite(parsed))
                safeSlider->setValue(juce::jlimit(safeSlider->getMinimum(),
                    safeSlider->getMaximum(), parsed), juce::sendNotificationSync);
        }), true);
}

void ArmNongMuangLA2AAudioProcessorEditor::setupRotary(juce::Slider& slider, const juce::String& name) {
    slider.setName(name); slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setLookAndFeel(&pointerLookAndFeel);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
}
void ArmNongMuangLA2AAudioProcessorEditor::addHitButton(juce::TextButton& b, const juce::String& name) {
    b.setName(name); b.setButtonText(juce::String{}); b.setLookAndFeel(&invisibleLookAndFeel);
    b.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    b.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    b.setColour(juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
    b.setColour(juce::TextButton::textColourOnId, juce::Colours::transparentBlack);
    addAndMakeVisible(b);
}
void ArmNongMuangLA2AAudioProcessorEditor::SwitchLookAndFeel::drawButtonBackground(
    juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) {
    // Intentionally draw nothing. The original switches are part of the supplied
    // PNG faceplate. These transparent hit zones only provide mouse interaction;
    // they must never paint replacement lever artwork over the PNG.
}

void ArmNongMuangLA2AAudioProcessorEditor::PointerLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int w, int h, float pos,
    float start, float end, juce::Slider& slider) {
    juce::ignoreUnused(slider);

    // Entire knob is procedurally rendered with JUCE paths and gradients.
    // No knob PNG, bitmap, or external texture is used.
    const float side = static_cast<float>(juce::jmin(w, h));
    const float cx = x + w * 0.5f;
    const float cy = y + h * 0.5f;
    const float r = side * 0.485f;
    auto circle = [&](float radius) {
        return juce::Rectangle<float>(cx - radius, cy - radius,
                                      radius * 2.0f, radius * 2.0f);
    };
    auto radialShape = [&](float outerRadius, float innerRadius, int lobes) {
        juce::Path path;
        constexpr int stepsPerLobe = 16;
        const int count = lobes * stepsPerLobe;
        for (int i = 0; i <= count; ++i) {
            const float a = juce::MathConstants<float>::twoPi *
                            static_cast<float>(i) / static_cast<float>(count);
            const float wave = 0.5f + 0.5f * std::cos(a * static_cast<float>(lobes));
            const float smooth = wave * wave * (3.0f - 2.0f * wave);
            const float radius = innerRadius + (outerRadius - innerRadius) * smooth;
            const float px = cx + std::sin(a) * radius;
            const float py = cy - std::cos(a) * radius;
            if (i == 0) path.startNewSubPath(px, py); else path.lineTo(px, py);
        }
        path.closeSubPath();
        return path;
    };

    // Deep shadow and precision-machined outer bezel.
    g.setColour(juce::Colours::black.withAlpha(0.88f));
    g.fillEllipse(circle(r + side * 0.012f));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xfff4f1e9), cx-r, cy-r,
        juce::Colour(0xff282b2d), cx+r, cy+r, false));
    g.fillEllipse(circle(r));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff55595b), cx-r*0.88f, cy-r*0.86f,
        juce::Colour(0xff050607), cx+r*0.88f, cy+r*0.88f, false));
    g.fillEllipse(circle(r * 0.965f));
    g.setColour(juce::Colour(0xffd9d7ce).withAlpha(0.82f));
    g.drawEllipse(circle(r * 0.94f), side * 0.009f);

    // Satin black outer annulus with a fine inner chrome bead.
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3a3d3e), cx-r*0.72f, cy-r*0.82f,
        juce::Colour(0xff080a0b), cx+r*0.72f, cy+r*0.78f, false));
    g.fillEllipse(circle(r * 0.855f));
    g.setColour(juce::Colour(0xffa7a9a5).withAlpha(0.76f));
    g.drawEllipse(circle(r * 0.835f), side * 0.008f);
    g.setColour(juce::Colour(0xff050607).withAlpha(0.9f));
    g.drawEllipse(circle(r * 0.795f), side * 0.012f);

    // Sculpted scalloped grip: alternating machined lobes and valleys.
    auto scallop = radialShape(r * 0.735f, r * 0.625f, 10);
    g.setColour(juce::Colour(0xff030405).withAlpha(0.95f));
    g.fillPath(scallop, juce::AffineTransform::translation(side * 0.008f, side * 0.012f));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff777a78), cx-r*0.55f, cy-r*0.58f,
        juce::Colour(0xff080a0b), cx+r*0.55f, cy+r*0.62f, false));
    g.fillPath(scallop);
    g.setColour(juce::Colour(0xffb9b7ae).withAlpha(0.72f));
    g.strokePath(scallop, juce::PathStrokeType(side * 0.009f));
    auto innerBead = radialShape(r * 0.605f, r * 0.565f, 10);
    g.setColour(juce::Colour(0xff050607));
    g.fillPath(innerBead);
    g.setColour(juce::Colour(0xff8f918e).withAlpha(0.68f));
    g.strokePath(innerBead, juce::PathStrokeType(side * 0.006f));

    // Brushed aluminum cap and concentric machined lip.
    const float capR = r * 0.535f;
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xfff5f3eb), cx-capR*0.62f, cy-capR*0.72f,
        juce::Colour(0xff777a78), cx+capR*0.75f, cy+capR*0.82f, false));
    g.fillEllipse(circle(capR));
    g.setColour(juce::Colour(0xfff4f2e9).withAlpha(0.92f));
    g.drawEllipse(circle(capR), side * 0.010f);
    g.setColour(juce::Colour(0xff444744).withAlpha(0.42f));
    g.drawEllipse(circle(capR * 0.965f), side * 0.004f);

    // Fine radial lathe marks, clipped to the circular aluminum face.
    juce::Path machining;
    const float inner = capR * 0.12f;
    const float outer = capR * 0.94f;
    for (int i = 0; i < 180; ++i) {
        const float a = juce::MathConstants<float>::twoPi * static_cast<float>(i) / 180.0f;
        const float offset = (i % 4 == 0) ? side * 0.003f : 0.0f;
        machining.startNewSubPath(cx + std::cos(a) * (inner + offset),
                                  cy + std::sin(a) * (inner + offset));
        machining.lineTo(cx + std::cos(a) * outer, cy + std::sin(a) * outer);
    }
    {
        juce::Graphics::ScopedSaveState state(g);
        juce::Path capClip;
        capClip.addEllipse(circle(capR * 0.94f));
        g.reduceClipRegion(capClip);
        g.setColour(juce::Colour(0xff454744).withAlpha(0.22f));
        g.strokePath(machining, juce::PathStrokeType(side * 0.0012f));
        g.setColour(juce::Colour(0xffffffff).withAlpha(0.25f));
        g.drawEllipse(circle(capR * 0.10f), side * 0.005f);
    }

    // Raised silver position pointer follows the live normalized parameter.
    const float angle = start + pos * (end - start);
    juce::Path pointer;
    pointer.addRoundedRectangle(cx - side * 0.034f, cy - r * 0.945f,
                                side * 0.068f, r * 0.285f,
                                side * 0.012f);
    const auto transform = juce::AffineTransform::rotation(angle, cx, cy);
    g.setColour(juce::Colour(0xff020304).withAlpha(0.95f));
    g.fillPath(pointer, transform.translated(side * 0.006f, side * 0.009f));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffffffff), cx-side*0.025f, cy-r*0.94f,
        juce::Colour(0xff858986), cx+side*0.025f, cy-r*0.67f, false));
    g.fillPath(pointer, transform);
    g.setColour(juce::Colour(0xfff8f7f0).withAlpha(0.88f));
    g.strokePath(pointer, juce::PathStrokeType(side * 0.003f), transform);
}

void ArmNongMuangLA2AAudioProcessorEditor::setPower(bool on) {
    powerOn = on; processor.parameters.getParameter("power")->setValueNotifyingHost(on ? 1.0f : 0.0f); repaint();
}
void ArmNongMuangLA2AAudioProcessorEditor::setLimit(bool on) {
    limitOn = on; processor.parameters.getParameter("limit")->setValueNotifyingHost(on ? 1.0f : 0.0f); repaint();
}
void ArmNongMuangLA2AAudioProcessorEditor::cycleMeter() {
    // Two-position LA-2A meter switch: DOWN = Gain Reduction, UP = Output.
    // AudioParameterChoice stores choice indices as normalized values: index / (numChoices - 1).
    const float normalized = processor.parameters.getParameter("meter")->getValue();
    const int currentIndex = juce::jlimit(0, 2, juce::roundToInt(normalized * 2.0f));
    const int nextIndex = (currentIndex == 1) ? 2 : 1;
    processor.parameters.getParameter("meter")->setValueNotifyingHost(static_cast<float>(nextIndex) / 2.0f);
    meterSwitch.setToggleState(nextIndex == 1, juce::dontSendNotification);
    topMeterSwitch.setToggleState(nextIndex == 1, juce::dontSendNotification);
    repaint();
}
void ArmNongMuangLA2AAudioProcessorEditor::loadModelDialog() {
    chooser = std::make_unique<juce::FileChooser>(
        "Choose model file (.nam or .gen)", juce::File{}, "*.nam;*.gen");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) {
            const auto file = fc.getResult();
            if (!file.existsAsFile()) return;

            juce::String message;
            const bool loaded = processor.loadNamFile(file, message);
            status.setText(message, juce::dontSendNotification);
            // Keep a successful model filename clearly visible in white.
            // Errors remain red for quick diagnosis.
            status.setColour(juce::Label::textColourId,
                             loaded ? juce::Colours::white : juce::Colour(0xffff5555));
            status.setTooltip(file.getFullPathName());
            repaint();
        });
}
void ArmNongMuangLA2AAudioProcessorEditor::saveSnapshot(int slot) {
    auto state = processor.parameters.copyState();
    if (slot == 0) snapshotA = state.createCopy(); else snapshotB = state.createCopy();
}
void ArmNongMuangLA2AAudioProcessorEditor::restoreSnapshot(int slot) {
    const auto& state = (slot == 0 ? snapshotA : snapshotB);
    if (state.isValid()) processor.parameters.replaceState(state.createCopy());
    powerOn = processor.parameters.getRawParameterValue("power")->load() >= 0.5f;
    limitOn = processor.parameters.getRawParameterValue("limit")->load() >= 0.5f;
    bottomPower.setToggleState(powerOn, juce::dontSendNotification);
    limitSwitch.setToggleState(limitOn, juce::dontSendNotification);
    meterSwitch.setToggleState(processor.getMeterMode() == 1, juce::dontSendNotification);
    topMeterSwitch.setToggleState(processor.getMeterMode() == 1, juce::dontSendNotification);
}
void ArmNongMuangLA2AAudioProcessorEditor::timerCallback() {
    // Smooth the meter needle with an exponential follower at 60 Hz.
    const float targetVuDb = juce::jlimit(-20.0f, 3.0f, processor.getVuValueDb());
    smoothedVuDb += (targetVuDb - smoothedVuDb) * 0.22f;
    if (std::abs(targetVuDb - smoothedVuDb) < 0.025f) smoothedVuDb = targetVuDb;
    powerOn = processor.parameters.getRawParameterValue("power")->load() >= 0.5f;
    limitOn = processor.parameters.getRawParameterValue("limit")->load() >= 0.5f;
    bottomPower.setToggleState(powerOn, juce::dontSendNotification);
    limitSwitch.setToggleState(limitOn, juce::dontSendNotification);
    meterSwitch.setToggleState(processor.getMeterMode() == 1, juce::dontSendNotification);
    topMeterSwitch.setToggleState(processor.getMeterMode() == 1, juce::dontSendNotification);
    const auto meterMode = processor.getMeterMode();
    // Do not overwrite a value while its inline editor is active.
    if (!gainValueLabel.isBeingEdited())
        gainValueLabel.setText(juce::String(gain.getValue(), 1), juce::dontSendNotification);
    if (!peakValueLabel.isBeingEdited())
        peakValueLabel.setText(juce::String(peak.getValue(), 1), juce::dontSendNotification);
    switchStatus.setText("POWER " + juce::String(powerOn ? "ON" : "OFF")
        + "   |   " + juce::String(limitOn ? "LIMIT" : "COMPRESS")
        + "   |   METER " + juce::String(meterMode == 1 ? "OUTPUT" : (meterMode == 2 ? "GAIN REDUCTION" : "INPUT")),
        juce::dontSendNotification);
    repaint();
}

void ArmNongMuangLA2AAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black);
    if (background.isValid()) g.drawImage(background, getLocalBounds().toFloat());

    const float sx = getWidth() / 1498.0f;
    const float sy = getHeight() / 775.0f;
    // Animated LA-2A-style VU needle, drawn over the PNG meter.
    const float db = juce::jlimit(-20.0f, 3.0f, smoothedVuDb);
    const float norm = (db + 20.0f) / 23.0f;
    const float angle = juce::degreesToRadians(-151.0f + norm * 107.0f);
    const float cx = 749.0f * sx, cy = 362.0f * sy, radius = 145.0f * sx;
    const float ex = cx + std::cos(angle) * radius;
    const float ey = cy + std::sin(angle) * radius;
    g.setColour(juce::Colours::black.withAlpha(0.72f));
    g.drawLine(cx + sx, cy + sy, ex + sx, ey + sy, 4.0f * sx);
    g.setColour(juce::Colour(0xff21170b));
    g.drawLine(cx, cy, ex, ey, 2.5f * sx);
    g.setColour(juce::Colour(0xffd0b16d));
    g.fillEllipse(cx - 4.0f*sx, cy - 4.0f*sy, 8.0f*sx, 8.0f*sy);
    g.setColour(juce::Colour(0xff342617));
    g.drawEllipse(cx - 4.0f*sx, cy - 4.0f*sy, 8.0f*sx, 8.0f*sy, 1.0f*sx);
    modelPicker.setTooltip(processor.isNamLoaded()
        ? "Loaded model: " + processor.getLoadedNamName()
        : "Click to load a compatible .nam or .gen model. Built-in compressor remains available.");
}

void ArmNongMuangLA2AAudioProcessorEditor::resized() {
    const float sx = getWidth() / 1498.0f, sy = getHeight() / 775.0f;
    auto place = [&](juce::Component& c, float x, float y, float w, float h) {
        c.setBounds(juce::roundToInt(x*sx), juce::roundToInt(y*sy),
                    juce::roundToInt(w*sx), juce::roundToInt(h*sy));
    };
    // Coordinates are measured directly against the new 1498 x 775 PNG.
    place(gain, 195.0f, 231.0f, 190, 190); // Match original PNG knob outer ring and center
    place(peak, 1113.0f, 231.0f, 190, 190); // Match original PNG knob outer ring and center
    place(gainValueLabel, 210, 430, 160, 29);
    place(peakValueLabel, 1127, 430, 160, 29);
    place(attack, 0, 0, 1, 1);
    place(release, 0, 0, 1, 1);

    // Top strip: clickable zones follow the printed controls in the PNG.
    place(topPower, 80, 48, 64, 76);
    place(preset, 226, 54, 310, 57);
    place(modelPicker, 584, 54, 312, 57);
    place(buttonA, 946, 50, 54, 72);
    place(buttonB, 1014, 50, 54, 72);
    place(copyAB, 1096, 54, 70, 60);
    place(settings, 1215, 54, 62, 60);
    place(topMeterSwitch, 1335, 49, 38, 64);

    // Bottom lever switches: UP=LIMIT/ON/OUTPUT, DOWN=COMPRESS/OFF/GAIN REDUCTION.
    place(limitSwitch, 338, 638, 46, 78);
    place(bottomPower, 737, 638, 46, 78);
    place(meterSwitch, 1195, 638, 46, 78);
    // Visible model status/filename line under the model-load hit area.
    place(status, 584, 113, 312, 26);
    place(switchStatus, 520, 730, 458, 28);
}
