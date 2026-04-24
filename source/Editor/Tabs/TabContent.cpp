#include "TabContent.h"
#include "../../Utility/Parameters.h"
#include "../Utility/ConnectorPainting.h"

// =============================================================================
// MainTabContent
// =============================================================================

MainTabContent::MainTabContent (PluginProcessor& p, ModulationModeState* modState, AnimationFrameSource* animSource)
    : waveformComponent (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }, &p.modDestOutputs[4], &p.modDestOutputs[5], ParamIDs::oscWaveform, ParamIDs::pulseWidth),
      filterDisplay (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }, &p.modDestOutputs[0], &p.modDestOutputs[1]),
      hpfDisplay (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }),
      subOscillatorComponent (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }, &p.modDestOutputs[8], &p.modDestOutputs[9], ParamIDs::subOsc, ParamIDs::subOscWave),
      detuneComponent (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }, &p.modDestOutputs[6], &p.modDestOutputs[7], ParamIDs::oscDetune, ParamIDs::oscWidth),
      chorusComponent (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }, &p.modDestOutputs[12], &p.modDestOutputs[13], ParamIDs::chorusDepth, ParamIDs::chorusRate),
      vibratoComponent (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }, &p.modDestOutputs[10], &p.modDestOutputs[11], ParamIDs::vibratoDepth, ParamIDs::vibratoRate),
      volumeComponent (p.parameters.getParameter (ParamIDs::volume), { &p.undoManager, &p.activeGestureCount, modState }, ParamIDs::volume),
      oscLevelComponent (p.parameters.getParameter (ParamIDs::mainOscLevel), nullptr, { &p.undoManager, &p.activeGestureCount, modState }, ParamIDs::mainOscLevel),
      noiseComponent (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }, &p.modDestOutputs[ModDest::noiseLevel], &p.modDestOutputs[ModDest::noiseTone], ParamIDs::noiseLevel, ParamIDs::noiseTone),
      chorusMixComponent (p.parameters.getParameter (ParamIDs::chorusMix), { &p.undoManager, &p.activeGestureCount, modState }),
      portamentoComponent (p.parameters.getParameter (ParamIDs::portamentoTime), { &p.undoManager, &p.activeGestureCount, modState }),
      delayComponent (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }),
      delayModComponent (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }),
      delayRateComponent (p.parameters.getParameter (ParamIDs::delayTime),
          dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter (ParamIDs::delayTempoSync)),
          dynamic_cast<juce::AudioParameterChoice*> (p.parameters.getParameter (ParamIDs::delayNoteDivision)),
          { &p.undoManager, &p.activeGestureCount, modState }),
      delaySyncToggle (dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter (ParamIDs::delayTempoSync))),
      delayHighpassComponent (p.parameters.getParameter (ParamIDs::delayHighpass), nullptr, { &p.undoManager, &p.activeGestureCount, modState }),
      delayLowpassComponent (p.parameters.getParameter (ParamIDs::delayLowpass), nullptr, { &p.undoManager, &p.activeGestureCount, modState }),
      reverbComponent (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }),
      reverbModComponent (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }),
      reverbSizeComponent (p.parameters.getParameter (ParamIDs::reverbSize), nullptr, { &p.undoManager, &p.activeGestureCount, modState }),
      reverbPreDelayComponent (p.parameters.getParameter (ParamIDs::reverbPreDelay), nullptr, { &p.undoManager, &p.activeGestureCount, modState }),
      reverbBassMultComponent (p.parameters.getParameter (ParamIDs::reverbBassMult), nullptr, { &p.undoManager, &p.activeGestureCount, modState }),
      reverbDampingComponent (p.parameters.getParameter (ParamIDs::reverbDamping), nullptr, { &p.undoManager, &p.activeGestureCount, modState })
{
    // Synth components
    scrollContent.addAndMakeVisible (waveformComponent);
    scrollContent.addAndMakeVisible (filterDisplay);
    scrollContent.addAndMakeVisible (hpfDisplay);
    scrollContent.addAndMakeVisible (subOscillatorComponent);
    scrollContent.addAndMakeVisible (detuneComponent);
    scrollContent.addAndMakeVisible (chorusComponent);
    scrollContent.addAndMakeVisible (vibratoComponent);
    scrollContent.addAndMakeVisible (volumeComponent);
    scrollContent.addAndMakeVisible (oscLevelComponent);
    scrollContent.addAndMakeVisible (noiseComponent);
    scrollContent.addAndMakeVisible (chorusMixComponent);
    scrollContent.addAndMakeVisible (portamentoComponent);

    // Effects components
    scrollContent.addAndMakeVisible (delayComponent);
    scrollContent.addAndMakeVisible (delayModComponent);
    scrollContent.addAndMakeVisible (delayRateComponent);
    scrollContent.addAndMakeVisible (delaySyncToggle);
    scrollContent.addAndMakeVisible (delayHighpassComponent);
    scrollContent.addAndMakeVisible (delayLowpassComponent);
    scrollContent.addAndMakeVisible (reverbComponent);
    scrollContent.addAndMakeVisible (reverbModComponent);
    scrollContent.addAndMakeVisible (reverbSizeComponent);
    scrollContent.addAndMakeVisible (reverbPreDelayComponent);
    scrollContent.addAndMakeVisible (reverbBassMultComponent);
    scrollContent.addAndMakeVisible (reverbDampingComponent);

    viewport.setViewedComponent (&scrollContent, false);
    viewport.setScrollBarsShown (false, false, true, false);
    viewport.onScroll = [this] { scrollIndicator.showIndicator(); };
    addAndMakeVisible (viewport);

    scrollIndicator.setViewport (&viewport);
    addAndMakeVisible (scrollIndicator);

    // Delay + reverb sub-components share a unified shell painted by scrollContent,
    // so they skip their own outer box/border.
    delayComponent.setDrawOuterBox (false);
    delayModComponent.setDrawOuterBox (false);
    delayRateComponent.setDrawOuterBox (false);
    delaySyncToggle.setDrawOuterBox (false);
    delayHighpassComponent.setDrawOuterBox (false);
    delayLowpassComponent.setDrawOuterBox (false);
    reverbComponent.setDrawOuterBox (false);
    reverbModComponent.setDrawOuterBox (false);
    reverbSizeComponent.setDrawOuterBox (false);
    reverbPreDelayComponent.setDrawOuterBox (false);
    reverbBassMultComponent.setDrawOuterBox (false);
    reverbDampingComponent.setDrawOuterBox (false);

    scrollContent.onPaint = [this] (juce::Graphics& g)
    {
        g.fillAll (PRIMARY_COLOR);

        ConnectorPainting::drawEffectChainShell (g,
            delayComponent.getBounds(),
            delayModComponent.getBounds(),
            delayRateComponent.getBounds(),
            delaySyncToggle.getBounds(),
            delayHighpassComponent.getBounds(),
            delayLowpassComponent.getBounds());

        ConnectorPainting::drawEffectChainShell (g,
            reverbComponent.getBounds(),
            reverbModComponent.getBounds(),
            reverbSizeComponent.getBounds(),
            reverbPreDelayComponent.getBounds(),
            reverbBassMultComponent.getBounds(),
            reverbDampingComponent.getBounds());
    };

    scrollContent.onPaintOverChildren = [this] (juce::Graphics& g)
    {
        ConnectorPainting::chorusComponentConnector (g,
            chorusMixComponent.getBounds(),
            chorusComponent.getBounds());

        ConnectorPainting::oscComponentConnector (g,
            oscLevelComponent.getBounds(),
            waveformComponent.getBounds());

        ConnectorPainting::drawHorizontalConnectorSimple (g,
            waveformComponent.getBounds(),
            detuneComponent.getBounds());
    };
}

void MainTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);
}

void MainTabContent::resized()
{
    auto area = getLocalBounds();
    viewport.setBounds (area);
    scrollIndicator.setBounds (area);

    int availableWidth = viewport.getWidth();
    int unitSize = availableWidth / 6;
    int totalHeight = unitSize * 3;

    scrollContent.setSize (availableWidth, totalHeight);

    // Row 1 (synth): [Vol/ChMix/Porto grid] [Chorus] [Vibrato] [FilterDisplay = 3 units]
    auto row1 = juce::Rectangle<int> (0, 0, availableWidth, unitSize);
    auto mixGrid = row1.removeFromLeft (unitSize);
    auto mixGridTop = mixGrid.removeFromTop (mixGrid.getHeight() / 2);
    volumeComponent.setBounds (mixGridTop.removeFromLeft (mixGridTop.getWidth() / 2).reduced (Style::componentGap));
    chorusMixComponent.setBounds (mixGridTop.reduced (Style::componentGap));
    oscLevelComponent.setBounds (mixGrid.removeFromLeft (mixGrid.getWidth() / 2).reduced (Style::componentGap));
    portamentoComponent.setBounds (mixGrid.reduced (Style::componentGap));
    oscLevelComponent.setLabelPosition (SingleParameterComponent::LabelPosition::Bottom);
    portamentoComponent.setLabelPosition (SingleParameterComponent::LabelPosition::Bottom);
    chorusComponent.setBounds (row1.removeFromLeft (unitSize).reduced (Style::componentGap));
    vibratoComponent.setBounds (row1.removeFromLeft (unitSize).reduced (Style::componentGap));
    filterDisplay.setBounds (row1.reduced (Style::componentGap));

    // Row 2 (synth): [SubOsc] [Waveform] [Detune] [Noise] [HPFDisplay = 2 units]
    auto row2 = juce::Rectangle<int> (0, unitSize, availableWidth, unitSize);
    waveformComponent.setBounds (row2.removeFromLeft (unitSize).reduced (Style::componentGap));
    detuneComponent.setBounds (row2.removeFromLeft (unitSize).reduced (Style::componentGap));
    subOscillatorComponent.setBounds (row2.removeFromLeft (unitSize).reduced (Style::componentGap));
    noiseComponent.setBounds (row2.removeFromLeft (unitSize).reduced (Style::componentGap));
    hpfDisplay.setBounds (row2.reduced (Style::componentGap));

    // Row 3 (effects): [Delay] [DelayMod] [Delay 2x2] [Reverb] [ReverbMod] [Reverb 2x2]
    auto row3 = juce::Rectangle<int> (0, unitSize * 2, availableWidth, unitSize);
    delayComponent.setBounds (row3.removeFromLeft (unitSize).reduced (Style::componentGap));
    delayModComponent.setBounds (row3.removeFromLeft (unitSize).reduced (Style::componentGap));

    auto delayGrid = row3.removeFromLeft (unitSize);
    auto delayGridTop = delayGrid.removeFromTop (delayGrid.getHeight() / 2);
    delayRateComponent.setBounds (delayGridTop.removeFromLeft (delayGridTop.getWidth() / 2).reduced (Style::componentGap));
    delaySyncToggle.setBounds (delayGridTop.reduced (Style::componentGap));
    delayHighpassComponent.setBounds (delayGrid.removeFromLeft (delayGrid.getWidth() / 2).reduced (Style::componentGap));
    delayLowpassComponent.setBounds (delayGrid.reduced (Style::componentGap));
    delayHighpassComponent.setLabelPosition (SingleParameterComponent::LabelPosition::Bottom);
    delayLowpassComponent.setLabelPosition (SingleParameterComponent::LabelPosition::Bottom);

    reverbComponent.setBounds (row3.removeFromLeft (unitSize).reduced (Style::componentGap));
    reverbModComponent.setBounds (row3.removeFromLeft (unitSize).reduced (Style::componentGap));

    auto reverbGrid = row3;
    auto reverbGridTop = reverbGrid.removeFromTop (reverbGrid.getHeight() / 2);
    reverbSizeComponent.setBounds (reverbGridTop.removeFromLeft (reverbGridTop.getWidth() / 2).reduced (Style::componentGap));
    reverbPreDelayComponent.setBounds (reverbGridTop.reduced (Style::componentGap));
    reverbBassMultComponent.setBounds (reverbGrid.removeFromLeft (reverbGrid.getWidth() / 2).reduced (Style::componentGap));
    reverbDampingComponent.setBounds (reverbGrid.reduced (Style::componentGap));
    reverbBassMultComponent.setLabelPosition (SingleParameterComponent::LabelPosition::Bottom);
    reverbDampingComponent.setLabelPosition (SingleParameterComponent::LabelPosition::Bottom);
}

// =============================================================================
// KeyboardTabContent
// =============================================================================

KeyboardTabContent::KeyboardTabContent (PluginProcessor& p, AnimationFrameSource* animSource)
    : oscilloscope (p, animSource)
{
    addAndMakeVisible (oscilloscope);
}

void KeyboardTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);
}

void KeyboardTabContent::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (30);
    oscilloscope.setBounds (area.reduced (Style::componentGap));
}

// =============================================================================
// ModulationTabContent
// =============================================================================

ModulationTabContent::ModulationTabContent (PluginProcessor& p, AnimationFrameSource* animSource)
    : matrixComponent (p.modTree, p.modSourceOutputs.data(), &p.undoManager, &p.activeGestureCount, animSource)
{
    matrixViewport.setViewedComponent (&matrixComponent, false);
    matrixViewport.setScrollBarsShown (false, false, true, false);
    addAndMakeVisible (matrixViewport);

    scrollIndicator.setViewport (&matrixViewport);
    addAndMakeVisible (scrollIndicator);
    matrixViewport.onScroll = [this] { scrollIndicator.showIndicator(); };
}

void ModulationTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);
}

void ModulationTabContent::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (30);
    matrixViewport.setBounds (area.reduced (Style::componentGap));
    scrollIndicator.setBounds (matrixViewport.getBounds());
    matrixComponent.setSize (matrixViewport.getWidth(), 43 * 16);
}

// =============================================================================

// =============================================================================
// ExperimentTabContent
// =============================================================================

ExperimentTabContent::ExperimentTabContent (PluginProcessor& p)
#if SYNTHDEMO_DEV_MODE
    : processorRef (p)
#endif
{
    manualBpmSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    manualBpmSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 24);
    manualBpmSlider.setColour (juce::Slider::textBoxTextColourId, TEXT_COLOR);
    manualBpmSlider.setColour (juce::Slider::textBoxBackgroundColourId, SECONDARY_COLOR);
    manualBpmSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    manualBpmSlider.setTextValueSuffix (" BPM");
    addAndMakeVisible (manualBpmSlider);
    manualBpmAttachment = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::manualBpm, manualBpmSlider);

    manualBpmLabel.setColour (juce::Label::textColourId, TEXT_COLOR);
    manualBpmLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (manualBpmLabel);

    // Reverb sliders
    auto setupSlider = [this] (juce::Slider& slider)
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);
        slider.setColour (juce::Slider::textBoxTextColourId, TEXT_COLOR);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, SECONDARY_COLOR);
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (slider);
    };

    setupSlider (reverbDecaySlider);
    setupSlider (reverbSizeSlider);
    setupSlider (reverbDampingSlider);
    setupSlider (reverbBassMultSlider);
    setupSlider (reverbModRateSlider);
    setupSlider (reverbModDepthSlider);
    setupSlider (reverbDiffusionSlider);
    setupSlider (reverbPreDelaySlider);
    setupSlider (reverbWidthSlider);
    setupSlider (reverbLevelSlider);

    reverbDecayAttachment     = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbDecay, reverbDecaySlider);
    reverbSizeAttachment      = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbSize, reverbSizeSlider);
    reverbDampingAttachment   = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbDamping, reverbDampingSlider);
    reverbBassMultAttachment  = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbBassMult, reverbBassMultSlider);
    reverbModRateAttachment   = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbModRate, reverbModRateSlider);
    reverbModDepthAttachment  = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbModDepth, reverbModDepthSlider);
    reverbDiffusionAttachment = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbDiffusion, reverbDiffusionSlider);
    reverbPreDelayAttachment  = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbPreDelay, reverbPreDelaySlider);
    reverbWidthAttachment     = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbWidth, reverbWidthSlider);
    reverbLevelAttachment       = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbLevel, reverbLevelSlider);

    // Delay sliders
    setupSlider (delayTimeSlider);
    setupSlider (delayFeedbackSlider);
    setupSlider (delayMixSlider);
    setupSlider (delayLowpassSlider);
    setupSlider (delayHighpassSlider);
    setupSlider (delaySaturationSlider);
    setupSlider (delayModRateSlider);
    setupSlider (delayModDepthSlider);
    setupSlider (delayDiffusionSlider);

    delayTimeAttachment       = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::delayTime, delayTimeSlider);
    delayFeedbackAttachment   = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::delayFeedback, delayFeedbackSlider);
    delayMixAttachment        = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::delayMix, delayMixSlider);
    delayLowpassAttachment    = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::delayLowpass, delayLowpassSlider);
    delayHighpassAttachment   = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::delayHighpass, delayHighpassSlider);
    delaySaturationAttachment = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::delaySaturation, delaySaturationSlider);
    delayModRateAttachment    = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::delayModRate, delayModRateSlider);
    delayModDepthAttachment   = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::delayModDepth, delayModDepthSlider);
    delayDiffusionAttachment  = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::delayDiffusion, delayDiffusionSlider);

    // Delay note division combo box
    addAndMakeVisible (delayNoteDivisionCombo);
    delayNoteDivisionAttachment = std::make_unique<ComboBoxAttachment> (p.parameters, ParamIDs::delayNoteDivision, delayNoteDivisionCombo);

    // Delay toggles
    delayTempoSyncToggle = std::make_unique<CustomToggleComponent> (
        dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter (ParamIDs::delayTempoSync)), "Sync");
    addAndMakeVisible (*delayTempoSyncToggle);

    delayPingPongToggle = std::make_unique<CustomToggleComponent> (
        dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter (ParamIDs::delayPingPong)), "Ping Pong");
    addAndMakeVisible (*delayPingPongToggle);

    delayOnToggle = std::make_unique<CustomToggleComponent> (
        dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter (ParamIDs::delayOn)), "Delay");
    addAndMakeVisible (*delayOnToggle);

    reverbOnToggle = std::make_unique<CustomToggleComponent> (
        dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter (ParamIDs::reverbOn)), "Reverb");
    addAndMakeVisible (*reverbOnToggle);

#if SYNTHDEMO_DEV_MODE
    presetSectionLabel.setColour (juce::Label::textColourId, TEXT_COLOR);
    presetSectionLabel.setFont (juce::Font (Style::fontComponent));
    addAndMakeVisible (presetSectionLabel);

    presetNameEditor.setColour (juce::TextEditor::backgroundColourId, SECONDARY_COLOR);
    presetNameEditor.setColour (juce::TextEditor::textColourId, TEXT_COLOR);
    presetNameEditor.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    presetNameEditor.setTextToShowWhenEmpty ("Preset name...", TEXT_COLOR.withAlpha (0.4f));
    addAndMakeVisible (presetNameEditor);

    categoryCombo.setEditableText (true);
    categoryCombo.addItem ("Uncategorized", 1);
    auto categories = p.presetManager.getCategories();
    for (int i = 0; i < categories.size(); ++i)
        categoryCombo.addItem (categories[i], i + 2);
    categoryCombo.setSelectedId (1);
    addAndMakeVisible (categoryCombo);

    savePresetButton.setColour (juce::TextButton::buttonColourId, SECONDARY_COLOR);
    savePresetButton.setColour (juce::TextButton::textColourOffId, TEXT_COLOR);
    savePresetButton.onClick = [this]
    {
        auto name = presetNameEditor.getText().trim();
        if (name.isEmpty())
            return;

        auto category = categoryCombo.getText().trim();
        if (category.isEmpty() || category == "Uncategorized")
            category = "";

        auto stateTree = processorRef.buildStateTree();
        if (processorRef.presetManager.savePreset (name, category, stateTree))
        {
            // Refresh category combo
            categoryCombo.clear();
            categoryCombo.addItem ("Uncategorized", 1);
            auto cats = processorRef.presetManager.getCategories();
            for (int i = 0; i < cats.size(); ++i)
                categoryCombo.addItem (cats[i], i + 2);
            categoryCombo.setSelectedId (1);
        }
    };
    addAndMakeVisible (savePresetButton);
#endif
}

void ExperimentTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);

    auto area = getLocalBounds();
    int halfW = area.getWidth() / 2;

    // Manual BPM header
    g.setColour (TEXT_COLOR);
    g.setFont (Style::fontHeading);
    g.drawText ("Manual BPM", area.removeFromTop (35), juce::Justification::centred);

    area.removeFromTop (30); // BPM slider space

    int sliderH = 24;
    int spacing = 4;
    int labelW = 80;
    int contentTop = area.getY();

    // Reverb header (left half) - leave space for toggle on left
    auto reverbHeaderArea = juce::Rectangle<int> (0, contentTop, halfW, 25);
    g.setFont (Style::fontHeading);
    g.drawText ("Reverb", reverbHeaderArea, juce::Justification::centred);

    // Delay header (right half) - leave space for toggle on left
    auto delayHeaderArea = juce::Rectangle<int> (halfW, contentTop, halfW, 25);
    g.drawText ("BBD Delay", delayHeaderArea, juce::Justification::centred);

    g.setFont (Style::fontBody);
    int sliderTop = contentTop + 28;

    // Reverb labels (2 columns within left half)
    int reverbLeftX = 5;
    int reverbRightX = halfW / 2 + 5;

    const char* reverbLeftLabels[] = { "Decay", "Size", "Damping", "Bass Mult", "Mod Rate" };
    const char* reverbRightLabels[] = { "Mod Depth", "Diffusion", "Pre-Delay", "Width", "Mix" };

    for (int i = 0; i < 5; ++i)
    {
        int y = sliderTop + i * (sliderH + spacing);
        g.drawText (reverbLeftLabels[i], reverbLeftX, y, labelW, sliderH, juce::Justification::centredRight);
        g.drawText (reverbRightLabels[i], reverbRightX, y, labelW, sliderH, juce::Justification::centredRight);
    }

    // Delay labels (2 columns within right half)
    int delayLeftX = halfW + 5;
    int delayRightX = halfW + halfW / 2 + 5;

    const char* delayLeftLabels[] = { "Time", "Mix", "Highpass", "Mod Rate", "Diffusion" };
    const char* delayRightLabels[] = { "Feedback", "Lowpass", "Saturation", "Mod Depth" };

    for (int i = 0; i < 5; ++i)
    {
        int y = sliderTop + i * (sliderH + spacing);
        g.drawText (delayLeftLabels[i], delayLeftX, y, labelW, sliderH, juce::Justification::centredRight);
    }
    for (int i = 0; i < 4; ++i)
    {
        int y = sliderTop + i * (sliderH + spacing);
        g.drawText (delayRightLabels[i], delayRightX, y, labelW, sliderH, juce::Justification::centredRight);
    }
}

void ExperimentTabContent::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (30);
    int halfW = area.getWidth() / 2;

    // BPM row at top
    area.removeFromTop (35); // header space
    auto bpmArea = area.removeFromTop (30).withSizeKeepingCentre (250, 28);
    manualBpmSlider.setBounds (bpmArea);

    int sliderH = 24;
    int spacing = 4;
    int labelW = 80;
    int sliderW = halfW / 2 - labelW - 15;
    int sliderTop = area.getY() + 28; // after section headers

    // On/off toggles next to section headers
    int onToggleW = 55;
    int onToggleH = 22;
    int headerY = area.getY();
    reverbOnToggle->setBounds (5, headerY + 2, onToggleW, onToggleH);
    delayOnToggle->setBounds (halfW + 5, headerY + 2, onToggleW, onToggleH);

    // Reverb sliders: 2 columns within left half
    int reverbLeftSliderX = 5 + labelW + 5;
    int reverbRightSliderX = halfW / 2 + 5 + labelW + 5;

    juce::Slider* reverbLeft[] = { &reverbDecaySlider, &reverbSizeSlider, &reverbDampingSlider, &reverbBassMultSlider, &reverbModRateSlider };
    juce::Slider* reverbRight[] = { &reverbModDepthSlider, &reverbDiffusionSlider, &reverbPreDelaySlider, &reverbWidthSlider, &reverbLevelSlider };

    for (int i = 0; i < 5; ++i)
    {
        int y = sliderTop + i * (sliderH + spacing);
        reverbLeft[i]->setBounds (reverbLeftSliderX, y, sliderW, sliderH);
        reverbRight[i]->setBounds (reverbRightSliderX, y, sliderW, sliderH);
    }

    // Delay sliders: 2 columns within right half
    int delayLeftSliderX = halfW + 5 + labelW + 5;
    int delayRightSliderX = halfW + halfW / 2 + 5 + labelW + 5;

    juce::Slider* delayLeft[] = { &delayTimeSlider, &delayMixSlider, &delayHighpassSlider, &delayModRateSlider, &delayDiffusionSlider };
    juce::Slider* delayRight[] = { &delayFeedbackSlider, &delayLowpassSlider, &delaySaturationSlider, &delayModDepthSlider };

    for (int i = 0; i < 5; ++i)
    {
        int y = sliderTop + i * (sliderH + spacing);
        delayLeft[i]->setBounds (delayLeftSliderX, y, sliderW, sliderH);
    }
    for (int i = 0; i < 4; ++i)
    {
        int y = sliderTop + i * (sliderH + spacing);
        delayRight[i]->setBounds (delayRightSliderX, y, sliderW, sliderH);
    }

    // Toggle/combo row below delay sliders
    int toggleRowY = sliderTop + 4 * (sliderH + spacing);
    int toggleW = 70;
    int comboW = 90;
    delayTempoSyncToggle->setBounds (delayRightSliderX, toggleRowY, toggleW, sliderH);
    delayNoteDivisionCombo.setBounds (delayRightSliderX + toggleW + 5, toggleRowY, comboW, sliderH);
    delayPingPongToggle->setBounds (delayRightSliderX + toggleW + comboW + 10, toggleRowY, toggleW + 10, sliderH);

#if SYNTHDEMO_DEV_MODE
    auto presetArea = getLocalBounds().removeFromBottom (70).reduced (10, 5);
    presetSectionLabel.setBounds (presetArea.removeFromTop (20));
    auto saveRow = presetArea.removeFromTop (30);
    presetNameEditor.setBounds (saveRow.removeFromLeft (200).reduced (2));
    categoryCombo.setBounds (saveRow.removeFromLeft (150).reduced (2));
    savePresetButton.setBounds (saveRow.removeFromLeft (100).reduced (2));
#endif
}
