#include "TabContent.h"
#include "../../Utility/Parameters.h"

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
      volumeComponent (p.parameters.getParameter (ParamIDs::volume), { &p.undoManager, &p.activeGestureCount, modState }),
      noiseComponent (p.parameters.getParameter (ParamIDs::noiseLevel), { &p.undoManager, &p.activeGestureCount, modState }),
      chorusMixComponent (p.parameters.getParameter (ParamIDs::chorusMix), { &p.undoManager, &p.activeGestureCount, modState }),
      portamentoComponent (p.parameters.getParameter (ParamIDs::portamentoTime), { &p.undoManager, &p.activeGestureCount, modState }),
      delayComponent (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }),
      delayModComponent (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }),
      delayRateComponent (p.parameters.getParameter (ParamIDs::delayTime),
          dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter (ParamIDs::delayTempoSync)),
          dynamic_cast<juce::AudioParameterChoice*> (p.parameters.getParameter (ParamIDs::delayNoteDivision)),
          { &p.undoManager, &p.activeGestureCount, modState }),
      delayBpmToggle (dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter (ParamIDs::delayTempoSync)), "BPM"),
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
    scrollContent.addAndMakeVisible (noiseComponent);
    scrollContent.addAndMakeVisible (chorusMixComponent);
    scrollContent.addAndMakeVisible (portamentoComponent);

    // Effects components
    scrollContent.addAndMakeVisible (delayComponent);
    scrollContent.addAndMakeVisible (delayModComponent);
    scrollContent.addAndMakeVisible (delayRateComponent);
    scrollContent.addAndMakeVisible (delayBpmToggle);
    scrollContent.addAndMakeVisible (delayHighpassComponent);
    scrollContent.addAndMakeVisible (delayLowpassComponent);
    scrollContent.addAndMakeVisible (reverbComponent);
    scrollContent.addAndMakeVisible (reverbModComponent);
    scrollContent.addAndMakeVisible (reverbSizeComponent);
    scrollContent.addAndMakeVisible (reverbPreDelayComponent);
    scrollContent.addAndMakeVisible (reverbBassMultComponent);
    scrollContent.addAndMakeVisible (reverbDampingComponent);

    viewport.setViewedComponent (&scrollContent, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);
}

void MainTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);
}

void MainTabContent::resized()
{
    auto area = getLocalBounds();
    viewport.setBounds (area);

    int availableWidth = viewport.getWidth() - viewport.getScrollBarThickness();
    int unitSize = availableWidth / 6;
    int totalHeight = unitSize * 3;

    scrollContent.setSize (availableWidth, totalHeight);

    // Row 1 (synth): [2x2 grid] [Chorus] [Vibrato] [FilterDisplay = 3 units]
    auto row1 = juce::Rectangle<int> (0, 0, availableWidth, unitSize);
    auto mixCol = row1.removeFromLeft (unitSize);
    auto topHalf = mixCol.removeFromTop (mixCol.getHeight() / 2);
    volumeComponent.setBounds (topHalf.removeFromLeft (topHalf.getWidth() / 2).reduced (5));
    noiseComponent.setBounds (topHalf.reduced (5));
    chorusMixComponent.setBounds (mixCol.removeFromLeft (mixCol.getWidth() / 2).reduced (5));
    portamentoComponent.setBounds (mixCol.reduced (5));
    chorusComponent.setBounds (row1.removeFromLeft (unitSize).reduced (5));
    vibratoComponent.setBounds (row1.removeFromLeft (unitSize).reduced (5));
    filterDisplay.setBounds (row1.reduced (5));

    // Row 2 (synth): [SubOsc] [Waveform] [Detune] [HPFDisplay = 3 units]
    auto row2 = juce::Rectangle<int> (0, unitSize, availableWidth, unitSize);
    subOscillatorComponent.setBounds (row2.removeFromLeft (unitSize).reduced (5));
    waveformComponent.setBounds (row2.removeFromLeft (unitSize).reduced (5));
    detuneComponent.setBounds (row2.removeFromLeft (unitSize).reduced (5));
    hpfDisplay.setBounds (row2.reduced (5));

    // Row 3 (effects): [Delay] [DelayMod] [Delay 2x2] [Reverb] [ReverbMod] [Reverb 2x2]
    auto row3 = juce::Rectangle<int> (0, unitSize * 2, availableWidth, unitSize);
    delayComponent.setBounds (row3.removeFromLeft (unitSize).reduced (5));
    delayModComponent.setBounds (row3.removeFromLeft (unitSize).reduced (5));

    auto delayGrid = row3.removeFromLeft (unitSize);
    auto delayGridTop = delayGrid.removeFromTop (delayGrid.getHeight() / 2);
    delayRateComponent.setBounds (delayGridTop.removeFromLeft (delayGridTop.getWidth() / 2).reduced (5));
    delayBpmToggle.setBounds (delayGridTop.reduced (5));
    delayHighpassComponent.setBounds (delayGrid.removeFromLeft (delayGrid.getWidth() / 2).reduced (5));
    delayLowpassComponent.setBounds (delayGrid.reduced (5));

    reverbComponent.setBounds (row3.removeFromLeft (unitSize).reduced (5));
    reverbModComponent.setBounds (row3.removeFromLeft (unitSize).reduced (5));

    auto reverbGrid = row3;
    auto reverbGridTop = reverbGrid.removeFromTop (reverbGrid.getHeight() / 2);
    reverbSizeComponent.setBounds (reverbGridTop.removeFromLeft (reverbGridTop.getWidth() / 2).reduced (5));
    reverbPreDelayComponent.setBounds (reverbGridTop.reduced (5));
    reverbBassMultComponent.setBounds (reverbGrid.removeFromLeft (reverbGrid.getWidth() / 2).reduced (5));
    reverbDampingComponent.setBounds (reverbGrid.reduced (5));
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
    oscilloscope.setBounds (area.reduced (5));
}

// =============================================================================
// ModulationTabContent
// =============================================================================

ModulationTabContent::ModulationTabContent (PluginProcessor& p, AnimationFrameSource* animSource)
    : matrixComponent (p.modTree, p.modSourceOutputs.data(), &p.undoManager, &p.activeGestureCount, animSource)
{
    matrixViewport.setViewedComponent (&matrixComponent, false);
    matrixViewport.setScrollBarsShown (true, false);
    addAndMakeVisible (matrixViewport);
}

void ModulationTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);
}

void ModulationTabContent::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (30);
    matrixViewport.setBounds (area.reduced (5));
    matrixComponent.setSize (matrixViewport.getWidth() - matrixViewport.getScrollBarThickness(), 43 * 16);
}

// =============================================================================
// EffectsTabContent
// =============================================================================

EffectsTabContent::EffectsTabContent (PluginProcessor& p, ModulationModeState* modState, AnimationFrameSource* animSource)
    : delayComponent (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }),
      delayModComponent (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }),
      delayRateComponent (p.parameters.getParameter (ParamIDs::delayTime),
          dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter (ParamIDs::delayTempoSync)),
          dynamic_cast<juce::AudioParameterChoice*> (p.parameters.getParameter (ParamIDs::delayNoteDivision)),
          { &p.undoManager, &p.activeGestureCount, modState }),
      delayBpmToggle (dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter (ParamIDs::delayTempoSync)), "BPM"),
      delayHighpassComponent (p.parameters.getParameter (ParamIDs::delayHighpass), nullptr, { &p.undoManager, &p.activeGestureCount, modState }),
      delayLowpassComponent (p.parameters.getParameter (ParamIDs::delayLowpass), nullptr, { &p.undoManager, &p.activeGestureCount, modState }),
      reverbComponent (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }),
      reverbModComponent (p.parameters, { &p.undoManager, &p.activeGestureCount, modState, animSource }),
      reverbSizeComponent (p.parameters.getParameter (ParamIDs::reverbSize), nullptr, { &p.undoManager, &p.activeGestureCount, modState }),
      reverbPreDelayComponent (p.parameters.getParameter (ParamIDs::reverbPreDelay), nullptr, { &p.undoManager, &p.activeGestureCount, modState }),
      reverbBassMultComponent (p.parameters.getParameter (ParamIDs::reverbBassMult), nullptr, { &p.undoManager, &p.activeGestureCount, modState }),
      reverbDampingComponent (p.parameters.getParameter (ParamIDs::reverbDamping), nullptr, { &p.undoManager, &p.activeGestureCount, modState })
{
    addAndMakeVisible (delayComponent);
    addAndMakeVisible (delayModComponent);
    addAndMakeVisible (delayRateComponent);
    addAndMakeVisible (delayBpmToggle);
    addAndMakeVisible (delayHighpassComponent);
    addAndMakeVisible (delayLowpassComponent);

    addAndMakeVisible (reverbComponent);
    addAndMakeVisible (reverbModComponent);
    addAndMakeVisible (reverbSizeComponent);
    addAndMakeVisible (reverbPreDelayComponent);
    addAndMakeVisible (reverbBassMultComponent);
    addAndMakeVisible (reverbDampingComponent);
}

void EffectsTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);
}

void EffectsTabContent::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (30);
    auto rowHeight = area.getHeight() / 2;

    // --- Delay row ---
    auto delayRow = area.removeFromTop (rowHeight);
    auto squareSize = rowHeight;
    delayComponent.setBounds (delayRow.removeFromLeft (squareSize).reduced (5));
    delayModComponent.setBounds (delayRow.removeFromLeft (squareSize).reduced (5));
    auto delayGrid = delayRow;
    auto delayGridTop = delayGrid.removeFromTop (delayGrid.getHeight() / 2);
    delayRateComponent.setBounds (delayGridTop.removeFromLeft (delayGridTop.getWidth() / 2).reduced (5));
    delayBpmToggle.setBounds (delayGridTop.reduced (5));
    delayHighpassComponent.setBounds (delayGrid.removeFromLeft (delayGrid.getWidth() / 2).reduced (5));
    delayLowpassComponent.setBounds (delayGrid.reduced (5));

    // --- Reverb row ---
    auto reverbRow = area;
    reverbComponent.setBounds (reverbRow.removeFromLeft (squareSize).reduced (5));
    reverbModComponent.setBounds (reverbRow.removeFromLeft (squareSize).reduced (5));
    auto reverbGrid = reverbRow;
    auto reverbGridTop = reverbGrid.removeFromTop (reverbGrid.getHeight() / 2);
    reverbSizeComponent.setBounds (reverbGridTop.removeFromLeft (reverbGridTop.getWidth() / 2).reduced (5));
    reverbPreDelayComponent.setBounds (reverbGridTop.reduced (5));
    reverbBassMultComponent.setBounds (reverbGrid.removeFromLeft (reverbGrid.getWidth() / 2).reduced (5));
    reverbDampingComponent.setBounds (reverbGrid.reduced (5));
}

// =============================================================================
// ExperimentTabContent
// =============================================================================

ExperimentTabContent::ExperimentTabContent (PluginProcessor& p)
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
    setupSlider (reverbMixSlider);

    reverbDecayAttachment     = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbDecay, reverbDecaySlider);
    reverbSizeAttachment      = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbSize, reverbSizeSlider);
    reverbDampingAttachment   = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbDamping, reverbDampingSlider);
    reverbBassMultAttachment  = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbBassMult, reverbBassMultSlider);
    reverbModRateAttachment   = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbModRate, reverbModRateSlider);
    reverbModDepthAttachment  = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbModDepth, reverbModDepthSlider);
    reverbDiffusionAttachment = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbDiffusion, reverbDiffusionSlider);
    reverbPreDelayAttachment  = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbPreDelay, reverbPreDelaySlider);
    reverbWidthAttachment     = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbWidth, reverbWidthSlider);
    reverbMixAttachment       = std::make_unique<SliderAttachment> (p.parameters, ParamIDs::reverbMix, reverbMixSlider);

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
    delayNoteDivisionCombo.setColour (juce::ComboBox::backgroundColourId, SECONDARY_COLOR);
    delayNoteDivisionCombo.setColour (juce::ComboBox::textColourId, TEXT_COLOR);
    delayNoteDivisionCombo.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
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
}

void ExperimentTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);

    auto area = getLocalBounds();
    int halfW = area.getWidth() / 2;

    // Manual BPM header
    g.setColour (TEXT_COLOR);
    g.setFont (16.0f);
    g.drawText ("Manual BPM", area.removeFromTop (35), juce::Justification::centred);

    area.removeFromTop (30); // BPM slider space

    int sliderH = 24;
    int spacing = 4;
    int labelW = 80;
    int contentTop = area.getY();

    // Reverb header (left half) - leave space for toggle on left
    auto reverbHeaderArea = juce::Rectangle<int> (0, contentTop, halfW, 25);
    g.setFont (16.0f);
    g.drawText ("Reverb", reverbHeaderArea, juce::Justification::centred);

    // Delay header (right half) - leave space for toggle on left
    auto delayHeaderArea = juce::Rectangle<int> (halfW, contentTop, halfW, 25);
    g.drawText ("BBD Delay", delayHeaderArea, juce::Justification::centred);

    g.setFont (12.0f);
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
    juce::Slider* reverbRight[] = { &reverbModDepthSlider, &reverbDiffusionSlider, &reverbPreDelaySlider, &reverbWidthSlider, &reverbMixSlider };

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
}
