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

CSVPresetsWriter::CSVPresetsWriter() {
    presetsFolder = "/Users/gwendal/Music/DX7_AllTheWeb";
    
    // TEST CSV FILE
    shouldWriteCatridges = true;  // TEMP, TODO MODIFIER
    
    // Pre-process all available files in directory and sub-directories
    scanDirectory(presetsFolder);
    std::cout << "Presets scan results: " << subFolders.size() << " sub-folders, " << cartridgeFiles.size() << " cartridges (some may be unvalid)." << std::endl;
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

void CSVPresetsWriter::WriteCurrentCartridge(DexedAudioProcessor *processor) {
    if (! shouldWriteCatridges)
        return;
    
    // TODO checker le folder, cohérence etc...
    currentSubFolder = "";
    currentCartridgeName = "Dexed_01";
    // CSV file: creation and column names
    std::ofstream csvFile;
    char delimiter = ';';
    csvFile.open(getCurrentPath() + "/a_dummy_test.csv");
    csvFile << "preset_index;preset_name";
    for (size_t i=0 ; i<processor->getNumParameters() ; i++)
        csvFile << delimiter << i;
    csvFile << std::endl;
    // Actual reading of all parameters
    for (size_t preset_idx=0 ; preset_idx < processor->getNumPrograms() ; preset_idx++) {
        processor->setCurrentProgram(preset_idx);
        std::string presetName = processor->getProgramName(preset_idx).toStdString();
        if (presetName.find(delimiter) != std::string::npos) {  // if the delimiter is found, we have a problem
            assert(false);  // check the name and how to replace the delimiter-like char
            size_t delimiterPos = presetName.find(delimiter);
            presetName[delimiterPos] = ' ';
        }
        csvFile << preset_idx << delimiter << presetName;
        for (size_t i=0 ; i<processor->getNumParameters() ; i++)
            csvFile << delimiter << std::to_string(processor->getParameter(i));
        csvFile << std::endl;
    }
}
