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


class CSVPresetsWriter {
    
    bool shouldWriteCatridges;
    
    std::string presetsFolder;
    std::string currentSubFolder;
    std::string currentCartridgeName;
    
    std::vector<fs::directory_entry> subFolders;
    std::vector<fs::directory_entry> cartridgeFiles;
    
    /// Returns the current path (subfolder inside the main presets' folder)
    std::string getCurrentPath()
    { return presetsFolder + (currentSubFolder.empty() ? "" : ("/" + currentSubFolder)); }
    
    /// Scans a directory, looking for either sysex cartridges or sub-directories (recursive function)
    /// Stores the sub-directories and the cartridges filenames
    void scanDirectory(std::string path);
    
public:
    CSVPresetsWriter();
    
    /// Stores the currently loaded preset - for later write to a CSV file
    void WriteCurrentCartridge(DexedAudioProcessor *processor);
};
