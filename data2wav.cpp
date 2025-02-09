#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <cmath>  // for std::round

#pragma pack(push, 1)
struct WAVHeader {
    char riff[4];           // "RIFF"
    uint32_t fileSize;      // File size - 8 bytes
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmtSize;       // Size of fmt chunk (16 for PCM)
    uint16_t audioFormat;   // PCM = 1
    uint16_t numChannels;   // 1 (mono) or 2 (stereo), etc.
    uint32_t sampleRate;    // e.g., 44100 Hz (will be read from CSV)
    uint32_t byteRate;      // SampleRate * NumChannels * BitsPerSample/8
    uint16_t blockAlign;    // NumChannels * BitsPerSample/8
    uint16_t bitsPerSample; // Should be 16 for our output
    char data[4];           // "data"
    uint32_t dataSize;      // Size of audio data in bytes
};
#pragma pack(pop)

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input CSV file> <output WAV file>\n";
        return 1;
    }

    std::ifstream inFile(argv[1]);
    if (!inFile) {
        std::cerr << "Error: Could not open CSV file!\n";
        return 1;
    }

    std::ofstream outFile(argv[2], std::ios::binary);
    if (!outFile) {
        std::cerr << "Error: Could not create WAV file!\n";
        return 1;
    }

    std::string line;
    uint32_t sampleRate = 44100; // fallback default

    // === Check for a comment line containing the sample rate ===
    if (std::getline(inFile, line)) {
        if (!line.empty() && line[0] == '#') {
            // Expected format: "# SampleRate: <number>"
            std::istringstream iss(line);
            std::string hash, token;
            iss >> hash >> token; // token should be "SampleRate:"
            if (token == "SampleRate:" && (iss >> sampleRate)) {
                // Successfully parsed sample rate.
            }
            // Read the next line, which should be the header line.
            if (!std::getline(inFile, line)) {
                std::cerr << "Error: CSV file does not contain a header line after the sample rate comment!\n";
                return 1;
            }
        }
    }
    else {
        std::cerr << "Error: CSV file is empty!\n";
        return 1;
    }

    // Now 'line' holds the CSV header line.
    std::istringstream headerStream(line);
    std::vector<std::string> headerColumns;
    std::string col;
    while (std::getline(headerStream, col, ',')) {
        // Trim leading and trailing whitespace.
        col.erase(col.begin(), std::find_if(col.begin(), col.end(), [](unsigned char c) { return !std::isspace(c); }));
        col.erase(std::find_if(col.rbegin(), col.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), col.end());
        headerColumns.push_back(col);
    }
    if (headerColumns.size() < 2) {
        std::cerr << "Error: CSV header must contain at least two columns (e.g., Sample and one channel)!\n";
        return 1;
    }

    // Determine how many columns to skip:
    // If the second column is "Time", then skip two columns ("Sample" and "Time").
    int headerOffset = 1;
    if (headerColumns.size() >= 2 && headerColumns[1] == "Time")
        headerOffset = 2;

    // The remaining columns represent channels.
    int numChannels = static_cast<int>(headerColumns.size() - headerOffset);

    // Prepare containers for channel samples.
    // Each channel will have its own vector of samples.
    std::vector< std::vector<int16_t> > channelSamples(numChannels);

    // Read each data line.
    while (std::getline(inFile, line)) {
        if (line.empty())
            continue;
        std::istringstream lineStream(line);
        std::string token;

        // Read and ignore the first column (sample index).
        if (!std::getline(lineStream, token, ',')) continue;
        // If there is a "Time" column, skip it too.
        if (headerOffset == 2) {
            if (!std::getline(lineStream, token, ',')) {
                std::cerr << "Error: Missing time data in line: " << line << "\n";
                continue;
            }
        }

        // For each channel, read the sample.
        for (int ch = 0; ch < numChannels; ch++) {
            if (!std::getline(lineStream, token, ',')) {
                std::cerr << "Error: Missing sample data in line: " << line << "\n";
                break;
            }
            // Trim whitespace.
            token.erase(token.begin(), std::find_if(token.begin(), token.end(), [](unsigned char c) { return !std::isspace(c); }));
            token.erase(std::find_if(token.rbegin(), token.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), token.end());
            try {
                // The CSV values are from the original 8-bit file (0–255).
                int sample8 = std::stoi(token);
                // Convert to 16-bit: recenter (subtract 128) and scale up by 256.
                int sample16 = (sample8 - 128) << 8;
                channelSamples[ch].push_back(static_cast<int16_t>(sample16));
            }
            catch (const std::exception& e) {
                std::cerr << "Error converting sample '" << token << "': " << e.what() << "\n";
            }
        }
    }
    inFile.close();

    // Interleave samples (if multiple channels) into one vector.
    // Assumes all channels have the same number of samples.
    size_t numFrames = channelSamples[0].size();
    std::vector<int16_t> interleaved;
    interleaved.reserve(numFrames * numChannels);
    for (size_t i = 0; i < numFrames; i++) {
        for (int ch = 0; ch < numChannels; ch++) {
            interleaved.push_back(channelSamples[ch][i]);
        }
    }

    // Build the WAV header.
    WAVHeader wavHeader;
    std::memcpy(wavHeader.riff, "RIFF", 4);
    std::memcpy(wavHeader.wave, "WAVE", 4);
    std::memcpy(wavHeader.fmt, "fmt ", 4);
    wavHeader.fmtSize = 16;
    wavHeader.audioFormat = 1;
    wavHeader.numChannels = numChannels;
    wavHeader.sampleRate = sampleRate;  // Use the sample rate from the CSV comment.
    wavHeader.bitsPerSample = 16;
    wavHeader.blockAlign = wavHeader.numChannels * (wavHeader.bitsPerSample / 8);
    wavHeader.byteRate = wavHeader.sampleRate * wavHeader.blockAlign;
    wavHeader.dataSize = static_cast<uint32_t>(interleaved.size() * sizeof(int16_t));
    std::memcpy(wavHeader.data, "data", 4);
    wavHeader.fileSize = 36 + wavHeader.dataSize;

    // Write the header and the audio data.
    outFile.write(reinterpret_cast<char*>(&wavHeader), sizeof(WAVHeader));
    outFile.write(reinterpret_cast<char*>(interleaved.data()), wavHeader.dataSize);
    outFile.close();

    std::cout << "CSV file converted to WAV successfully.\n"
        << "Using sample rate: " << sampleRate << " Hz\n"
        << "Number of channels: " << numChannels << "\n";
    return 0;
}
