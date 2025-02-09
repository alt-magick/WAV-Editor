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
    uint32_t sampleRate;    // e.g., 22222 Hz (computed from CSV times)
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
    // Read the CSV header line (e.g., "Time,Channel1,Channel2")
    if (!std::getline(inFile, line)) {
        std::cerr << "Error: CSV file is empty!\n";
        return 1;
    }
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
        std::cerr << "Error: CSV header must contain at least two columns (Time and one channel)!\n";
        return 1;
    }
    // The first column is assumed to be "Time", so the number of channels is:
    int numChannels = headerColumns.size() - 1;

    // Prepare containers for time values and channel samples.
    std::vector<double> times;
    // Create a vector for each channel.
    std::vector< std::vector<int16_t> > channelSamples(numChannels);

    // Read each data line.
    while (std::getline(inFile, line)) {
        if (line.empty()) continue;
        std::istringstream lineStream(line);
        std::string token;

        // Read the time value.
        if (!std::getline(lineStream, token, ',')) continue;
        // Trim whitespace.
        token.erase(token.begin(), std::find_if(token.begin(), token.end(), [](unsigned char c) { return !std::isspace(c); }));
        token.erase(std::find_if(token.rbegin(), token.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), token.end());
        double t = std::stod(token);
        times.push_back(t);

        // For each channel, read the sample.
        for (int ch = 0; ch < numChannels; ch++) {
            if (!std::getline(lineStream, token, ',')) {
                std::cerr << "Error: Missing sample data in line: " << line << "\n";
                break;
            }
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

    // Compute the sample rate from the CSV time values.
    // (For example, if times[1] - times[0] is ~0.000045 s, sampleRate will be ~22222 Hz.)
    uint32_t sampleRate = 44100; // fallback default.
    if (times.size() >= 2) {
        double dt = times[1] - times[0];
        if (dt > 0)
            sampleRate = static_cast<uint32_t>(std::round(1.0 / dt));
    }

    // Interleave samples (if multiple channels) into one vector.
    size_t numFrames = channelSamples[0].size();  // assuming all channels have the same number of samples.
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
    wavHeader.sampleRate = sampleRate;
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
        << "Computed sample rate: " << sampleRate << " Hz\n"
        << "Number of channels: " << numChannels << "\n";
    return 0;
}
