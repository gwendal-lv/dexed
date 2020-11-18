/*
  ==============================================================================

    CSVPresetsWriter.cpp
    Created: 17 Nov 2020 1:12:22pm
    Author:  Gwendal Le Vaillant

  ==============================================================================
*/

#include <iostream>

#include "CSVPresetsWriter.h"

#include "Dexed.h"  // TRACE macro

CSVPresetsWriter::CSVPresetsWriter(DexedAudioProcessorEditor* _pluginEditor) : pluginEditor(_pluginEditor) {
    presetsFolder = "/Users/gwendal/Music/DX7_AllTheWeb";
    shouldWriteCatridges = false;  // to prevent the writing during startup
    
    // Pre-process all available files in directory and sub-directories
    scanDirectory(presetsFolder);
    std::cout << "Presets scan results: " << subFolders.size() << " sub-folders, " << cartridgeFiles.size() << " cartridges (some may be unvalid)." << std::endl;
    
    // TEST chargement différé d'une cartouche
    Timer::callAfterDelay(2000, [&] () {
        this->shouldWriteCatridges = true;
        this->loadNextCartridge();
    } );
}

void CSVPresetsWriter::scanDirectory(std::string dirPath) {
    for (const auto & entry : std::filesystem::directory_iterator(dirPath)) {
        auto filename = std::string(entry.path().filename());
        if (filename[0] != '.') {  // We do not consider hidden files
            if (entry.is_directory()) {
                subFolders.push_back(entry);
                scanDirectory(std::string(entry.path()));
            }
            else if (entry.is_regular_file()) {
                auto extension = std::string(entry.path().extension()); // SYSEX files only
                if (extension.compare(std::string(".syx")) == 0 || extension.compare(std::string(".SYX")) == 0)
                    cartridgeFiles.push_back(entry);
            }
        }
    }
}

void CSVPresetsWriter::loadNextCartridge() {
    currentCartridgeFileIndex++;
    if (currentCartridgeFileIndex >= cartridgeFiles.size())  // TEMP POUR TEST, lire le tout
    {
        shouldWriteCatridges = false;  // after writing, the dexed can be used normally
        return;
    }
    // If a new cartridge can be loaded, the prepare the folder/name for the future CSV writing
    std::filesystem::directory_entry& entry = cartridgeFiles[currentCartridgeFileIndex];
    const std::filesystem::path& path = entry.path();
    //std::cout << "filename: " << path.filename() << std::endl;
    currentCartridgeName = path.filename();
    currentCartridgeName = currentCartridgeName.substr(0, currentCartridgeName.size() - 4);  // .syx or .SYX removal
    //std::cout << "cartridge name = " << currentCartridgeName << std::endl;
    currentSubFolder = std::string(path.parent_path());
    
    // Cartridge loading. If unvalid cartridge: we go to the next one
    if (! pluginEditor->loadCart(File(std::string(path)))) {
        std::cout << "========== Cartridge " << currentCartridgeName << " from folder " << currentSubFolder << " cannot be loaded. ==========" << std::endl;
        Timer::callAfterDelay(1, [&] () { this->loadNextCartridge(); } );
    }
}

void CSVPresetsWriter::WriteCurrentCartridge(DexedAudioProcessor *pluginProcessor) {
    if (! shouldWriteCatridges)
        return;
    
    char delimiter = ';';
    // CSV file: creation and column names
    std::ofstream csvFile;
    std::string csvFileName = currentSubFolder + "/" + currentCartridgeName + ".csv";
    std::cout << "========== Writing " << csvFileName << " ==========" << std::endl;
    csvFile.open(csvFileName);
    csvFile << "preset_index;preset_name";
    for (size_t i=0 ; i<pluginProcessor->getNumParameters() ; i++)
        csvFile << delimiter << i;
    csvFile << std::endl;
    // Actual reading of all parameters
    for (size_t preset_idx=0 ; preset_idx < pluginProcessor->getNumPrograms() ; preset_idx++) {
        pluginProcessor->setCurrentProgram(preset_idx);
        std::string presetName = pluginProcessor->getProgramName(preset_idx).toStdString();
        if (presetName.find(delimiter) != std::string::npos) {  // if the delimiter is found, we have a problem
            //assert(false);  // check the name and how to replace the delimiter-like char
            size_t delimiterPos = presetName.find(delimiter);
            presetName[delimiterPos] = ' ';
        }
        csvFile << preset_idx << delimiter << presetName;
        for (size_t i=0 ; i<pluginProcessor->getNumParameters() ; i++)
            csvFile << delimiter << std::to_string(pluginProcessor->getParameter(i));
        csvFile << std::endl;
    }
    
    // LOAD NEXT PRESET
    Timer::callAfterDelay(1, [&] () { this->loadNextCartridge(); } );
}
