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

CSVPresetsWriter::CSVPresetsWriter(DexedAudioProcessorEditor* _pluginEditor, DexedAudioProcessor* _pluginProcessor) :
pluginEditor(_pluginEditor), pluginProcessor(_pluginProcessor), delimiter(';')
{
    // ================== BASE FOLDER FOR SEARCHING SYSEX FILES ================
    // ================== BASE FOLDER FOR SEARCHING SYSEX FILES ================
    presetsFolder = "/Volumes/Gwendal_SSD/Datasets/DX7_AllTheWeb";
    paramsNamesFileName = "DEXED_PARAMETERS_NAMES.csv";
    // ================== BASE FOLDER FOR SEARCHING SYSEX FILES ================
    // ================== BASE FOLDER FOR SEARCHING SYSEX FILES ================
    shouldWriteCatridges = false;  // to prevent any .csv writing during startup
    
    // Pre-process all available files in directory and sub-directories
    scanDirectory(presetsFolder);
    std::cout << "Presets scan results: " << subFolders.size() << " sub-folders, " << cartridgeFiles.size() << " cartridges (some may be unvalid)." << std::endl;
    
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

void CSVPresetsWriter::writeParamsNames() {
    std::ofstream csvFile;
    std::string csvFileName = presetsFolder + "/" + paramsNamesFileName;
    std::cout << "========== Writing parameters' names to " << paramsNamesFileName << " ==========" << std::endl;
    csvFile.open(csvFileName);
    // Columns labels are parameters indexes
    for (size_t i=0 ; i<pluginProcessor->getNumParameters() ; i++) {
        if (i > 0)
            csvFile << delimiter;
        csvFile << i;
    }
    csvFile << std::endl;
    // parameters names
    for (size_t i=0 ; i<pluginProcessor->getNumParameters() ; i++) {
        if (i > 0)
            csvFile << delimiter;
        csvFile << pluginProcessor->getParameterName(i).toStdString();
    }
    csvFile << std::endl;
}

void CSVPresetsWriter::loadNextCartridge() {
    currentCartridgeFileIndex++;
    // First cartridge: plugin params names written to yet another csv file
    if (currentCartridgeFileIndex == 0)
        writeParamsNames();
    // If the last cartridge already written, stop and return
    if (currentCartridgeFileIndex >= cartridgeFiles.size())
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

void CSVPresetsWriter::WriteCurrentCartridge() {
    if (! shouldWriteCatridges)
        return;
    
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
        // We replace any potential harmful special ASCII code by ' '
        for (size_t i=0 ; i<presetName.size() ; i++) {
            if (presetName[i] < 0x20 || 0x7E < presetName[i] || presetName[i] == '"')
                presetName[i] = ' ';
        }
        csvFile << preset_idx << delimiter << presetName;
        for (size_t i=0 ; i<pluginProcessor->getNumParameters() ; i++)
            csvFile << delimiter << std::to_string(pluginProcessor->getParameter(i));
        csvFile << std::endl;
    }
    
    // LOAD NEXT PRESET
    Timer::callAfterDelay(1, [&] () { this->loadNextCartridge(); } );
}
