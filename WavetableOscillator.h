/*
  ==============================================================================

    WavetableOscillator.h
    Created: 13 Aug 2020 1:55:26pm
    Author:  ewana

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class WavetableOscillator
{
public:
    WavetableOscillator(const juce::AudioSampleBuffer& wavetableToUse) : wavetable(wavetableToUse), tableSize(wavetable.getNumSamples() - 1)
    {
        jassert(wavetable.getNumChannels() == 1);
    }

    void setFrequency(float frequency, float sampleRate)
    {
        auto tableSizeOverSampleRate = (float)tableSize / sampleRate;
        tableDelta = frequency * tableSizeOverSampleRate;
    }

    forcedinline float getNextSample() noexcept
    {
        auto index0 = (unsigned int)currentIndex;
        auto index1 = index0 + 1;

        auto frac = currentIndex - (float)index0;

        auto* table = wavetable.getReadPointer(0);
        auto value0 = table[index0];
        auto value1 = table[index1];

        //interpolate
        auto currentSample = value0 + frac * (value1 - value0);

        if ((currentIndex += tableDelta) > (float)tableSize)
            currentIndex -= (float)tableSize;

        return currentSample;
    }

private:
    const juce::AudioSampleBuffer& wavetable;
    const int tableSize;
    float currentIndex = 0.0f, tableDelta = 0.0f;
};

class MainContentComponent : public juce::AudioAppComponent,
    public juce::Timer
{
public:
    MainContentComponent()
    {
        //Frequency shift stuff
        addAndMakeVisible(frequencySlider);
        frequencySlider.setRange(50.0, 20000.0);
        frequencySlider.setSkewFactorFromMidPoint(500.0); 
        frequencySlider.setBounds(20, 50, 200, 200);
        frequencySlider.onValueChange = [this]
        {
            oscFreq = frequencySlider.getValue();
        };
        addAndMakeVisible(freqPicker);
        freqPicker.setBounds(20, 70, 100, 100);
        freqPicker.setText("Freuency Selection", juce::dontSendNotification);

        //says toggle but its a slider between the osc's 
        addAndMakeVisible(sineToggle);
        sineToggle.setRange(0.0, 1.0);
        sineToggle.onValueChange = [this]
        {
            float value = sineToggle.getValue();
            if (value <= 0.33) {
                osc = 0;
            }
            else if (value <= 0.66) {
                osc = 1;
            }
            else
                osc = 2;
        };
        sineToggle.setBounds(20, 200, 200, 200);
        addAndMakeVisible(wavePicker);
        wavePicker.setBounds(20, 230, 100, 100);
        wavePicker.setText("Wave Selection", juce::dontSendNotification);

        //cpu usage levels
        cpuUsageLabel.setText("CPU Usage", juce::dontSendNotification);
        cpuUsageText.setJustificationType(juce::Justification::right);
        addAndMakeVisible(cpuUsageLabel);
        addAndMakeVisible(cpuUsageText);
    
        //create the wave tables
        createPureSineWavetable();
        createSquareWaveTable();
        createBrokenTriangleWaveTable();

        setSize(300, 400);
        setAudioChannels(0, 2); 
        startTimer(50);
    }

    ~MainContentComponent() override
    {
        shutdownAudio();
    }

    void resized() override
    {
        cpuUsageLabel.setBounds(10, 10, getWidth() - 20, 20);
        cpuUsageText.setBounds(10, 10, getWidth() - 20, 20);
    }

    void timerCallback() override
    {
        auto cpu = deviceManager.getCpuUsage() * 100;
        cpuUsageText.setText(juce::String(cpu, 6) + " %", juce::dontSendNotification);
    }

    void createHarmonicSineWavetable()
    {
        sineTable.setSize(1, (int)tableSize + 1);
        sineTable.clear();

        auto* samples = sineTable.getWritePointer(0);

        int harmonics[] = { 1, 3, 5, 6, 7, 9, 13, 15 };
        float harmonicWeights[] = { 0.5f, 0.1f, 0.05f, 0.125f, 0.09f, 0.005f, 0.002f, 0.001f };     // [1]

        jassert(juce::numElementsInArray(harmonics) == juce::numElementsInArray(harmonicWeights));

        for (auto harmonic = 0; harmonic < juce::numElementsInArray(harmonics); ++harmonic)
        {
            auto angleDelta = juce::MathConstants<double>::twoPi / (double)(tableSize - 1) * harmonics[harmonic]; // [2]
            auto currentAngle = 0.0;

            for (unsigned int i = 0; i < tableSize; ++i)
            {
                auto sample = std::sin(currentAngle);
                samples[i] += (float)sample * harmonicWeights[harmonic];                           
                currentAngle += angleDelta;
            }
        }

        samples[tableSize] = samples[0];
    }

    void createPureSineWavetable()
    {
        pureSineTable.setSize(1, (int)tableSize + 1);
        pureSineTable.clear();

        auto* samples = pureSineTable.getWritePointer(0);

        auto angleDelta = juce::MathConstants<double>::twoPi / (double)(tableSize - 1); 
        auto currentAngle = 0.0;

        for (unsigned int i = 0; i < tableSize; ++i)
        {
            auto sample = std::sin(currentAngle);
            samples[i] += (float)sample;                          
            currentAngle += angleDelta;
        }
        samples[tableSize] = samples[0];
    }

    void createSquareWaveTable()
    {
        squareTable.setSize(1, (int)tableSize + 1);
        squareTable.clear();

        auto* samples = squareTable.getWritePointer(0);

        for (unsigned int i = 0; i < tableSize; ++i) {
            if (i < tableSize / 2)
                samples[i] = 1;
            else
                samples[i] = -1;
        }
        samples[tableSize] = samples[0];
    }

    void createBrokenTriangleWaveTable()
    {
        brokenTriangleTable.setSize(1, (int)tableSize + 1);
        brokenTriangleTable.clear();

        auto* samples = brokenTriangleTable.getWritePointer(0);

        unsigned short int tableDivision = tableSize / 4;
        for (unsigned int i = 0; i < tableDivision; ++i) {
            float value = i / tableDivision;
            samples[i] = value;
        }
        for (unsigned int i = tableDivision; i < (3 * tableDivision); ++i) {
            float value = (i -(tableDivision - i)) / tableDivision;
            samples[i] = value;
        }
        for (unsigned int i = (3 * tableDivision); i < (4 * tableDivision); ++i) {
            float value = (i - (3 * tableDivision)) / tableDivision;
            samples[i] = value;
        }
        samples[tableSize] = samples[0];
    }

    void prepareToPlay(int, double sampleRate) override
    {
        auto* oscillator1 = new WavetableOscillator(brokenTriangleTable);
        auto* oscillator2 = new WavetableOscillator(squareTable); 
        auto* oscillator3 = new WavetableOscillator(pureSineTable);
        oscillators.add(oscillator1);
        oscillators.add(oscillator2);
        oscillators.add(oscillator3);
        level = 0.25f;
    }

    void releaseResources() override {}

    
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        auto* leftBuffer = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
        auto* rightBuffer = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);

        bufferToFill.clearActiveBufferRegion();

        WavetableOscillator* oscillator = oscillators.getUnchecked(osc);
        oscillator->setFrequency(oscFreq, (float)44100);

        for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
        {
            auto levelSample = oscillator->getNextSample() * level;

            leftBuffer[sample] += levelSample;
            rightBuffer[sample] += levelSample;
        }
    }

private:
    juce::Label cpuUsageLabel;
    juce::Label cpuUsageText;
    juce::Label freqPicker;
    juce::Label wavePicker;

    const unsigned int tableSize = 1 << 10;
    float level = 0.0f, oscFreq = 50.0;
    int osc = 0;

    juce::AudioSampleBuffer sineTable;
    juce::AudioSampleBuffer pureSineTable;
    juce::AudioSampleBuffer squareTable;
    juce::AudioSampleBuffer brokenTriangleTable;
    juce::OwnedArray<WavetableOscillator> oscillators;

    juce::Slider frequencySlider;
    juce::Slider sineToggle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContentComponent)
};
