/*
  ==============================================================================

    SamSamplerPlugin.cpp
    Created: 19 Jan 2026 5:30:00pm
    Author:  White Room Audio

    JUCE AudioProcessor wrapper for Sam Sampler with SF2 SoundFont support.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "dsp/SamSamplerDSP.h"

//==============================================================================
class SamSamplerPlugin  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SamSamplerPlugin()
        : AudioProcessor (BusesProperties()
                           .withInput ("Input",  juce::AudioChannelSet::stereo())
                           .withOutput ("Output", juce::AudioChannelSet::stereo())),
          currentPresetIndex (0),
          sampleRate (48000.0)
    {
        // Initialize parameters
        addParameter (masterVolParam = new juce::AudioParameterFloat ("master", "Master", 0.0f, 2.0f, 1.1f));
        addParameter (pitchParam = new juce::AudioParameterFloat ("pitch", "Pitch", 0.5f, 2.0f, 1.0f));
        addParameter (sampleStartParam = new juce::AudioParameterFloat ("start", "Sample Start", 0.0f, 1.0f, 0.0f));
        addParameter (sampleEndParam = new juce::AudioParameterFloat ("end", "Sample End", 0.0f, 1.0f, 1.0f));
        addParameter (loopEnabledParam = new juce::AudioParameterBool ("loop", "Loop Enabled", false));
        addParameter (loopStartParam = new juce::AudioParameterFloat ("loopStart", "Loop Start", 0.0f, 1.0f, 0.0f));
        addParameter (loopEndParam = new juce::AudioParameterFloat ("loopEnd", "Loop End", 0.0f, 1.0f, 1.0f));
        addParameter (crossfadeParam = new juce::AudioParameterFloat ("crossfade", "Crossfade", 0.0f, 0.1f, 0.01f));

        addParameter (attackParam = new juce::AudioParameterFloat ("attack", "Attack", 0.001f, 5.0f, 0.01f));
        addParameter (holdParam = new juce::AudioParameterFloat ("hold", "Hold", 0.0f, 2.0f, 0.0f));
        addParameter (decayParam = new juce::AudioParameterFloat ("decay", "Decay", 0.001f, 5.0f, 0.1f));
        addParameter (sustainParam = new juce::AudioParameterFloat ("sustain", "Sustain", 0.0f, 1.0f, 0.7f));
        addParameter (releaseParam = new juce::AudioParameterFloat ("release", "Release", 0.001f, 5.0f, 0.2f));

        addParameter (filterCutoffParam = new juce::AudioParameterFloat ("filterCutoff", "Filter Cutoff", 20.0f, 20000.0f, 20000.0f));
        addParameter (filterResParam = new juce::AudioParameterFloat ("filterRes", "Filter Res", 0.0f, 1.0f, 0.0f));
        addParameter (filterTypeParam = new juce::AudioParameterInt ("filterType", "Filter Type", 0, 3, 0));

        addParameter (reverbMixParam = new juce::AudioParameterFloat ("reverb", "Reverb", 0.0f, 1.0f, 0.0f));
        addParameter (delayMixParam = new juce::AudioParameterFloat ("delay", "Delay", 0.0f, 1.0f, 0.0f));
        addParameter (driveParam = new juce::AudioParameterFloat ("drive", "Drive", 0.0f, 1.0f, 0.0f));

        addParameter (structureParam = new juce::AudioParameterFloat ("structure", "Structure", 0.0f, 1.0f, 0.5f));

        addParameter (stereoWidthParam = new juce::AudioParameterFloat ("stereoWidth", "Stereo Width", 0.0f, 1.0f, 0.5f));

        // Load factory presets
        loadFactoryPresets();

        // Initialize DSP with default preset
        if (!factoryPresets.empty())
        {
            currentPreset = factoryPresets[0];
            applyPresetToDSP();
        }
    }

    ~SamSamplerPlugin() override = default;

    //==============================================================================
    void prepareToPlay (double newSampleRate, int samplesPerBlock) override
    {
        sampleRate = newSampleRate;
        dsp.prepare (sampleRate, samplesPerBlock);
    }

    void releaseResources() override
    {
        dsp.reset();
    }

    void processBlock (juce::AudioBuffer<float>& buffer,
                       juce::MidiBuffer& midiMessages) override
    {
        juce::ScopedNoDenormals noDenormals;
        auto totalNumInputChannels  = getTotalNumInputChannels();
        auto totalNumOutputChannels = getTotalNumOutputChannels();

        // Clear output buffer
        for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
            buffer.clear (i, 0, buffer.getNumSamples());

        // Update DSP parameters from host
        updateDSPParameters();

        // Process MIDI events
        for (const auto metadata : midiMessages)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn())
            {
                auto midiNote = message.getNoteNumber();
                auto velocity = message.getVelocity() / 127.0f;

                DSP::ScheduledEvent event;
                event.type = DSP::ScheduledEvent::NOTE_ON;
                event.data.note.midiNote = midiNote;
                event.data.note.velocity = velocity;
                dsp.handleEvent (event);
            }
            else if (message.isNoteOff())
            {
                auto midiNote = message.getNoteNumber();
                auto velocity = message.getVelocity() / 127.0f;

                DSP::ScheduledEvent event;
                event.type = DSP::ScheduledEvent::NOTE_OFF;
                event.data.note.midiNote = midiNote;
                event.data.note.velocity = velocity;
                dsp.handleEvent (event);
            }
            else if (message.isPitchWheel())
            {
                auto pitchBend = message.getPitchWheelValue();
                dsp.setParameter ("pitchBend", (pitchBend - 8192) / 8192.0f);
            }
        }

        // Process audio
        auto* outputLeft = buffer.getWritePointer (0);
        auto* outputRight = buffer.getWritePointer (1);

        float* outputs[2] = { outputLeft, outputRight };
        dsp.process (outputs, 2, buffer.getNumSamples());
    }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override
    {
        return nullptr;
    }

    bool hasEditor() const override
    {
        return false;
    }

    //==============================================================================
    const juce::String getName() const override
    {
        return "Sam Sampler";
    }

    bool acceptsMidi() const override
    {
        return true;
    }

    bool producesMidi() const override
    {
        return false;
    }

    double getTailLengthSeconds() const override
    {
        return 0.0;
    }

    //==============================================================================
    int getNumPrograms() override
    {
        return static_cast<int>(factoryPresets.size());
    }

    int getCurrentProgram() override
    {
        return currentPresetIndex;
    }

    void setCurrentProgram (int index) override
    {
        if (index >= 0 && index < static_cast<int>(factoryPresets.size()))
        {
            currentPresetIndex = index;
            currentPreset = factoryPresets[index];
            applyPresetToDSP();
        }
    }

    const juce::String getProgramName (int index) override
    {
        if (index >= 0 && index < static_cast<int>(factoryPresets.size()))
            return factoryPresets[index].name;

        return {};
    }

    void changeProgramName (int index, const juce::String& newName) override
    {
        if (index >= 0 && index < static_cast<int>(factoryPresets.size()))
            factoryPresets[index].name = newName;
    }

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override
    {
        juce::ValueTree state ("state");

        state.setProperty ("master", masterVolParam->get(), nullptr);
        state.setProperty ("pitch", pitchParam->get(), nullptr);
        state.setProperty ("start", sampleStartParam->get(), nullptr);
        state.setProperty ("end", sampleEndParam->get(), nullptr);
        state.setProperty ("loop", loopEnabledParam->get(), nullptr);
        state.setProperty ("loopStart", loopStartParam->get(), nullptr);
        state.setProperty ("loopEnd", loopEndParam->get(), nullptr);
        state.setProperty ("crossfade", crossfadeParam->get(), nullptr);
        state.setProperty ("attack", attackParam->get(), nullptr);
        state.setProperty ("hold", holdParam->get(), nullptr);
        state.setProperty ("decay", decayParam->get(), nullptr);
        state.setProperty ("sustain", sustainParam->get(), nullptr);
        state.setProperty ("release", releaseParam->get(), nullptr);
        state.setProperty ("filterCutoff", filterCutoffParam->get(), nullptr);
        state.setProperty ("filterRes", filterResParam->get(), nullptr);
        state.setProperty ("filterType", filterTypeParam->get(), nullptr);
        state.setProperty ("reverb", reverbMixParam->get(), nullptr);
        state.setProperty ("delay", delayMixParam->get(), nullptr);
        state.setProperty ("drive", driveParam->get(), nullptr);
        state.setProperty ("structure", structureParam->get(), nullptr);
        state.setProperty ("stereoWidth", stereoWidthParam->get(), nullptr);
        state.setProperty ("preset", currentPresetIndex, nullptr);

        juce::MemoryOutputStream stream (destData, false);
        state.writeToStream (stream);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        auto xml = juce::XmlDocument::parse (juce::String ((const char*) data, size_t (sizeInBytes)));

        if (xml != nullptr)
        {
            auto state = juce::ValueTree::fromXml (*xml);

            if (state.isValid())
            {
                *masterVolParam = state.getProperty ("master", 1.1f);
                *pitchParam = state.getProperty ("pitch", 1.0f);
                *sampleStartParam = state.getProperty ("start", 0.0f);
                *sampleEndParam = state.getProperty ("end", 1.0f);
                *loopEnabledParam = state.getProperty ("loop", false);
                *loopStartParam = state.getProperty ("loopStart", 0.0f);
                *loopEndParam = state.getProperty ("loopEnd", 1.0f);
                *crossfadeParam = state.getProperty ("crossfade", 0.01f);
                *attackParam = state.getProperty ("attack", 0.01f);
                *holdParam = state.getProperty ("hold", 0.0f);
                *decayParam = state.getProperty ("decay", 0.1f);
                *sustainParam = state.getProperty ("sustain", 0.7f);
                *releaseParam = state.getProperty ("release", 0.2f);
                *filterCutoffParam = state.getProperty ("filterCutoff", 20000.0f);
                *filterResParam = state.getProperty ("filterRes", 0.0f);
                *filterTypeParam = state.getProperty ("filterType", 0);
                *reverbMixParam = state.getProperty ("reverb", 0.0f);
                *delayMixParam = state.getProperty ("delay", 0.0f);
                *driveParam = state.getProperty ("drive", 0.0f);
                *structureParam = state.getProperty ("structure", 0.5f);
                *stereoWidthParam = state.getProperty ("stereoWidth", 0.5f);

                currentPresetIndex = state.getProperty ("preset", 0);

                if (currentPresetIndex >= 0 && currentPresetIndex < static_cast<int>(factoryPresets.size()))
                {
                    currentPreset = factoryPresets[currentPresetIndex];
                }
            }
        }

        applyPresetToDSP();
    }

private:
    //==============================================================================
    struct Preset
    {
        juce::String name;
        float master;
        float pitch;
        float sampleStart;
        float sampleEnd;
        bool loopEnabled;
        float loopStart;
        float loopEnd;
        float crossfade;
        float attack;
        float hold;
        float decay;
        float sustain;
        float release;
        float filterCutoff;
        float filterRes;
        int filterType;
        float reverbMix;
        float delayMix;
        float drive;
        float structure;
        float stereoWidth;
    };

    //==============================================================================
    void loadFactoryPresets()
    {
        factoryPresets.clear();

        // Preset 1: Piano (short attack, medium decay)
        Preset piano;
        piano.name = "Piano";
        piano.master = 1.1f;
        piano.pitch = 1.0f;
        piano.sampleStart = 0.0f;
        piano.sampleEnd = 1.0f;
        piano.loopEnabled = false;
        piano.attack = 0.01f;
        piano.hold = 0.0f;
        piano.decay = 0.5f;
        piano.sustain = 0.3f;
        piano.release = 0.3f;
        piano.filterCutoff = 20000.0f;
        piano.filterRes = 0.0f;
        piano.filterType = 0;
        piano.reverbMix = 0.2f;
        piano.delayMix = 0.0f;
        piano.drive = 0.0f;
        piano.structure = 0.5f;
        piano.stereoWidth = 0.5f;
        factoryPresets.push_back (piano);

        // Preset 2: Strings (slow attack, long release)
        Preset strings;
        strings.name = "Strings";
        strings.master = 1.0f;
        strings.pitch = 1.0f;
        strings.sampleStart = 0.0f;
        strings.sampleEnd = 1.0f;
        strings.loopEnabled = true;
        strings.loopStart = 0.2f;
        strings.loopEnd = 0.8f;
        strings.crossfade = 0.05f;
        strings.attack = 0.3f;
        strings.hold = 0.0f;
        strings.decay = 0.2f;
        strings.sustain = 0.8f;
        strings.release = 1.0f;
        strings.filterCutoff = 8000.0f;
        strings.filterRes = 0.2f;
        strings.filterType = 0;
        strings.reverbMix = 0.4f;
        strings.delayMix = 0.1f;
        strings.drive = 0.0f;
        strings.structure = 0.3f;
        strings.stereoWidth = 0.7f;
        factoryPresets.push_back (strings);

        // Preset 3: Choir
        Preset choir;
        choir.name = "Choir";
        choir.master = 1.0f;
        choir.pitch = 1.0f;
        choir.sampleStart = 0.0f;
        choir.sampleEnd = 1.0f;
        choir.loopEnabled = true;
        choir.loopStart = 0.3f;
        choir.loopEnd = 0.9f;
        choir.crossfade = 0.1f;
        choir.attack = 0.2f;
        choir.hold = 0.0f;
        choir.decay = 0.1f;
        choir.sustain = 0.7f;
        choir.release = 0.5f;
        choir.filterCutoff = 5000.0f;
        choir.filterRes = 0.1f;
        choir.filterType = 0;
        choir.reverbMix = 0.5f;
        choir.delayMix = 0.0f;
        choir.drive = 0.0f;
        choir.structure = 0.4f;
        choir.stereoWidth = 0.8f;
        factoryPresets.push_back (choir);

        // Preset 4: Organ (fast attack, percussive)
        Preset organ;
        organ.name = "Organ";
        organ.master = 1.0f;
        organ.pitch = 1.0f;
        organ.sampleStart = 0.0f;
        organ.sampleEnd = 1.0f;
        organ.loopEnabled = true;
        organ.loopStart = 0.1f;
        organ.loopEnd = 0.9f;
        organ.crossfade = 0.01f;
        organ.attack = 0.01f;
        organ.hold = 0.0f;
        organ.decay = 0.1f;
        organ.sustain = 1.0f;
        organ.release = 0.1f;
        organ.filterCutoff = 20000.0f;
        organ.filterRes = 0.0f;
        organ.filterType = 0;
        organ.reverbMix = 0.3f;
        organ.delayMix = 0.0f;
        organ.drive = 0.1f;
        organ.structure = 0.3f;
        organ.stereoWidth = 0.5f;
        factoryPresets.push_back (organ);

        // Preset 5: Brass
        Preset brass;
        brass.name = "Brass";
        brass.master = 1.0f;
        brass.pitch = 1.0f;
        brass.sampleStart = 0.0f;
        brass.sampleEnd = 1.0f;
        brass.loopEnabled = true;
        brass.loopStart = 0.2f;
        brass.loopEnd = 0.8f;
        brass.crossfade = 0.02f;
        brass.attack = 0.1f;
        brass.hold = 0.0f;
        brass.decay = 0.2f;
        brass.sustain = 0.8f;
        brass.release = 0.2f;
        brass.filterCutoff = 6000.0f;
        brass.filterRes = 0.3f;
        brass.filterType = 0;
        brass.reverbMix = 0.2f;
        brass.delayMix = 0.0f;
        brass.drive = 0.2f;
        brass.structure = 0.6f;
        brass.stereoWidth = 0.6f;
        factoryPresets.push_back (brass);

        // Preset 6: Bass
        Preset bass;
        bass.name = "Bass";
        bass.master = 1.2f;
        bass.pitch = 1.0f;
        bass.sampleStart = 0.0f;
        bass.sampleEnd = 1.0f;
        bass.loopEnabled = false;
        bass.attack = 0.01f;
        bass.hold = 0.0f;
        bass.decay = 0.3f;
        bass.sustain = 0.4f;
        bass.release = 0.2f;
        bass.filterCutoff = 2000.0f;
        bass.filterRes = 0.2f;
        bass.filterType = 0;
        bass.reverbMix = 0.1f;
        bass.delayMix = 0.0f;
        bass.drive = 0.1f;
        bass.structure = 0.4f;
        bass.stereoWidth = 0.3f;
        factoryPresets.push_back (bass);

        // Preset 7: Guitar
        Preset guitar;
        guitar.name = "Guitar";
        guitar.master = 1.0f;
        guitar.pitch = 1.0f;
        guitar.sampleStart = 0.0f;
        guitar.sampleEnd = 1.0f;
        guitar.loopEnabled = false;
        guitar.attack = 0.01f;
        guitar.hold = 0.0f;
        guitar.decay = 0.4f;
        guitar.sustain = 0.5f;
        guitar.release = 0.3f;
        guitar.filterCutoff = 8000.0f;
        guitar.filterRes = 0.1f;
        guitar.filterType = 0;
        guitar.reverbMix = 0.3f;
        guitar.delayMix = 0.15f;
        guitar.drive = 0.1f;
        guitar.structure = 0.5f;
        guitar.stereoWidth = 0.6f;
        factoryPresets.push_back (guitar);

        // Preset 8: Synth Pad
        Preset synthpad;
        synthpad.name = "Synth Pad";
        synthpad.master = 1.0f;
        synthpad.pitch = 1.0f;
        synthpad.sampleStart = 0.0f;
        synthpad.sampleEnd = 1.0f;
        synthpad.loopEnabled = true;
        synthpad.loopStart = 0.1f;
        synthpad.loopEnd = 0.9f;
        synthpad.crossfade = 0.08f;
        synthpad.attack = 0.4f;
        synthpad.hold = 0.0f;
        synthpad.decay = 0.1f;
        synthpad.sustain = 0.8f;
        synthpad.release = 1.5f;
        synthpad.filterCutoff = 4000.0f;
        synthpad.filterRes = 0.3f;
        synthpad.filterType = 0;
        synthpad.reverbMix = 0.5f;
        synthpad.delayMix = 0.2f;
        synthpad.drive = 0.0f;
        synthpad.structure = 0.7f;
        synthpad.stereoWidth = 0.8f;
        factoryPresets.push_back (synthpad);

        // Preset 9: Percussive
        Preset perc;
        perc.name = "Percussive";
        perc.master = 1.0f;
        perc.pitch = 1.0f;
        perc.sampleStart = 0.0f;
        perc.sampleEnd = 1.0f;
        perc.loopEnabled = false;
        perc.attack = 0.001f;
        perc.hold = 0.0f;
        perc.decay = 0.2f;
        perc.sustain = 0.0f;
        perc.release = 0.1f;
        perc.filterCutoff = 12000.0f;
        perc.filterRes = 0.0f;
        perc.filterType = 0;
        perc.reverbMix = 0.2f;
        perc.delayMix = 0.0f;
        perc.drive = 0.0f;
        perc.structure = 0.2f;
        perc.stereoWidth = 0.4f;
        factoryPresets.push_back (perc);

        // Preset 10: Ambient Texture
        Preset ambient;
        ambient.name = "Ambient Texture";
        ambient.master = 0.9f;
        ambient.pitch = 0.5f;  // Down an octave
        ambient.sampleStart = 0.0f;
        ambient.sampleEnd = 1.0f;
        ambient.loopEnabled = true;
        ambient.loopStart = 0.0f;
        ambient.loopEnd = 1.0f;
        ambient.crossfade = 0.1f;
        ambient.attack = 0.8f;
        ambient.hold = 0.0f;
        ambient.decay = 0.5f;
        ambient.sustain = 0.6f;
        ambient.release = 3.0f;
        ambient.filterCutoff = 3000.0f;
        ambient.filterRes = 0.4f;
        ambient.filterType = 0;
        ambient.reverbMix = 0.7f;
        ambient.delayMix = 0.4f;
        ambient.drive = 0.05f;
        ambient.structure = 0.8f;
        ambient.stereoWidth = 1.0f;
        factoryPresets.push_back (ambient);
    }

    //==============================================================================
    void applyPresetToDSP()
    {
        dsp.setParameter ("masterVolume", currentPreset.master);
        dsp.setParameter ("basePitch", currentPreset.pitch);
        dsp.setParameter ("sampleStart", currentPreset.sampleStart);
        dsp.setParameter ("sampleEnd", currentPreset.sampleEnd);
        dsp.setParameter ("loopEnabled", currentPreset.loopEnabled ? 1.0f : 0.0f);
        dsp.setParameter ("loopStart", currentPreset.loopStart);
        dsp.setParameter ("loopEnd", currentPreset.loopEnd);
        dsp.setParameter ("crossfade", currentPreset.crossfade);
        dsp.setParameter ("envAttack", currentPreset.attack);
        dsp.setParameter ("envHold", currentPreset.hold);
        dsp.setParameter ("envDecay", currentPreset.decay);
        dsp.setParameter ("envSustain", currentPreset.sustain);
        dsp.setParameter ("envRelease", currentPreset.release);
        dsp.setParameter ("filterCutoff", currentPreset.filterCutoff);
        dsp.setParameter ("filterResonance", currentPreset.filterRes);
        dsp.setParameter ("filterType", (float)currentPreset.filterType);
        dsp.setParameter ("reverbMix", currentPreset.reverbMix);
        dsp.setParameter ("delayMix", currentPreset.delayMix);
        dsp.setParameter ("drive", currentPreset.drive);
        dsp.setParameter ("structure", currentPreset.structure);
        dsp.setParameter ("stereoWidth", currentPreset.stereoWidth);
    }

    //==============================================================================
    void updateDSPParameters()
    {
        dsp.setParameter ("masterVolume", masterVolParam->get());
        dsp.setParameter ("basePitch", pitchParam->get());
        dsp.setParameter ("sampleStart", sampleStartParam->get());
        dsp.setParameter ("sampleEnd", sampleEndParam->get());
        dsp.setParameter ("loopEnabled", loopEnabledParam->get() ? 1.0f : 0.0f);
        dsp.setParameter ("loopStart", loopStartParam->get());
        dsp.setParameter ("loopEnd", loopEndParam->get());
        dsp.setParameter ("crossfade", crossfadeParam->get());
        dsp.setParameter ("envAttack", attackParam->get());
        dsp.setParameter ("envHold", holdParam->get());
        dsp.setParameter ("envDecay", decayParam->get());
        dsp.setParameter ("envSustain", sustainParam->get());
        dsp.setParameter ("envRelease", releaseParam->get());
        dsp.setParameter ("filterCutoff", filterCutoffParam->get());
        dsp.setParameter ("filterResonance", filterResParam->get());
        dsp.setParameter ("filterType", (float)filterTypeParam->get());
        dsp.setParameter ("reverbMix", reverbMixParam->get());
        dsp.setParameter ("delayMix", delayMixParam->get());
        dsp.setParameter ("drive", driveParam->get());
        dsp.setParameter ("structure", structureParam->get());
        dsp.setParameter ("stereoWidth", stereoWidthParam->get());
    }

    //==============================================================================
    // DSP instance
    DSP::SamSamplerDSP dsp;

    // Parameters
    juce::AudioParameterFloat* masterVolParam;
    juce::AudioParameterFloat* pitchParam;
    juce::AudioParameterFloat* sampleStartParam;
    juce::AudioParameterFloat* sampleEndParam;
    juce::AudioParameterBool* loopEnabledParam;
    juce::AudioParameterFloat* loopStartParam;
    juce::AudioParameterFloat* loopEndParam;
    juce::AudioParameterFloat* crossfadeParam;
    juce::AudioParameterFloat* attackParam;
    juce::AudioParameterFloat* holdParam;
    juce::AudioParameterFloat* decayParam;
    juce::AudioParameterFloat* sustainParam;
    juce::AudioParameterFloat* releaseParam;
    juce::AudioParameterFloat* filterCutoffParam;
    juce::AudioParameterFloat* filterResParam;
    juce::AudioParameterInt* filterTypeParam;
    juce::AudioParameterFloat* reverbMixParam;
    juce::AudioParameterFloat* delayMixParam;
    juce::AudioParameterFloat* driveParam;
    juce::AudioParameterFloat* structureParam;
    juce::AudioParameterFloat* stereoWidthParam;

    // Preset system
    std::vector<Preset> factoryPresets;
    Preset currentPreset;
    int currentPresetIndex;

    // State
    double sampleRate;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamSamplerPlugin)
};

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SamSamplerPlugin();
}
