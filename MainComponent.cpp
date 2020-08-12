#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.

    //Slider stuff
    addAndMakeVisible(frequencySlider);
    frequencySlider.setRange(50.0, 20000.0);
    frequencySlider.setSkewFactorFromMidPoint(500.0); // [4]
    frequencySlider.onValueChange = [this]
    {
        if (currentSampleRate > 0.0) {
            updateAngleDelta();
            repaint();
        }
    };

    addAndMakeVisible(volume);
    volume.setBounds(100, 50, 200, 200);
    volume.setRange(0.0, 1.0);
    volume.onValueChange = [this]
    {
        audioLevel = (double)volume.getValue();
    };

    setSize(400, 400);
    setAudioChannels (2, 2);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::updateAngleDelta()
{
    currentFrequency = frequencySlider.getValue();
    auto cyclesPerSample = currentFrequency / currentSampleRate;
    angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
    currentFrequencyString = juce::String(currentFrequency);
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    updateAngleDelta();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto level = audioLevel * 0.125f;
    auto* leftBuffer = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
    auto* rightBuffer = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);

    for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
    {
        auto currentSample = (float)std::sin(currentAngle);
        currentAngle += angleDelta;
        leftBuffer[sample] = currentSample * level;
        rightBuffer[sample] = currentSample * level;
    }
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::azure);
    g.drawFittedText(currentFrequencyString, getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    frequencySlider.setBounds(10, 10, getWidth() - 20, 20);
}
