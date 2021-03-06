/*
  ==============================================================================

    CSVPresetsWriter.h
    Created: 17 Nov 2020 1:12:22pm
    Author:  Gwendal Le Vaillant

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include <string>
#include <vector>
#include <filesystem>  // C++17
namespace fs = std::filesystem;

#include "PluginProcessor.h"
#include "PluginEditor.h"


class CSVPresetsWriter {
    
    DexedAudioProcessorEditor* pluginEditor = 0;
    DexedAudioProcessor* pluginProcessor = 0;
    
    bool shouldWriteCatridges;
    
    /// CSV delimiter
    const char delimiter;
    
    std::string presetsFolder;
    std::string paramsNamesFileName;
    std::string currentSubFolder;
    std::string currentCartridgeName;

    std::vector<fs::directory_entry> subFolders;
    int currentCartridgeFileIndex = -1;
    std::vector<fs::directory_entry> cartridgeFiles;
    
    /// Scans a directory, looking for either sysex cartridges or sub-directories (recursive function)
    /// Stores the sub-directories and the cartridges filenames
    void scanDirectory(std::string path);
    
    void writeParamsNames();
    
    ///  Loads the next cartridge (from the pre-scanned folders) and recursively triggers itself on the Juce Message Thread
    /// The loading is done through the PluginEditor, which itself will call this->WriteCurrentCartridge
    void loadNextCartridge();
    
public:
    CSVPresetsWriter(DexedAudioProcessorEditor* _pluginEditor, DexedAudioProcessor* _pluginProcessor);
    
    /// Actually write all 32 presets from the current cartridge to 1 CSV file
    void WriteCurrentCartridge();
};
