#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>
#include <sstream>
#include <iomanip>
#include <string>

#pragma pack(push, 1)
struct WAVHeader {
    char riff[4];        // "RIFF"
    uint32_t fileSize;   // File size - 8 bytes
    char wave[4];        // "WAVE"
    char fmt[4];         // "fmt "
    uint32_t fmtSize;    // Size of fmt chunk
    uint16_t audioFormat;// 1 = PCM
    uint16_t numChannels;// 1 = Mono, 2 = Stereo
    uint32_t sampleRate; // Samples per second
    uint32_t byteRate;   // SampleRate * NumChannels * BitsPerSample/8
    uint16_t blockAlign; // NumChannels * BitsPerSample/8
    uint16_t bitsPerSample; // Bits per sample (8, 16, 24, 32)
    char data[4];        // "data"
    uint32_t dataSize;   // Size of audio data
};
#pragma pack(pop)

// Convert a binary string back to a number
template <typename T>
T fromBinary(const std::string& binaryStr) {
    T value = 0;
    for (size_t i = 0; i < binaryStr.size(); i++) {
        value |= (binaryStr[i] == '1' ? 1 : 0) << (binaryStr.size() - i - 1);
    }
    return value;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input CSV file> <output WAV file>\n";
        return 1;
    }

    std::string inputFilename = argv[1];  // First command-line argument
    std::string outputFilename = argv[2]; // Second command-line argument

    std::ifstream inFile(inputFilename);
    std::ofstream outFile(outputFilename, std::ios::binary);

    if (!inFile) {
        std::cerr << "Error: Could not open CSV file!\n";
        return 1;
    }
    if (!outFile) {
        std::cerr << "Error: Could not create output WAV file!\n";
        return 1;
    }

    // Define WAV header
    WAVHeader header = {};
    header.riff[0] = 'R'; header.riff[1] = 'I'; header.riff[2] = 'F'; header.riff[3] = 'F';
    header.wave[0] = 'W'; header.wave[1] = 'A'; header.wave[2] = 'V'; header.wave[3] = 'E';
    header.fmt[0] = 'f'; header.fmt[1] = 'm'; header.fmt[2] = 't'; header.fmt[3] = ' ';
    header.audioFormat = 1; // PCM
    header.numChannels = 2; // Stereo
    header.sampleRate = 44100; // Assuming 44.1 kHz sample rate
    header.bitsPerSample = 16; // 16-bit depth
    header.blockAlign = (header.numChannels * header.bitsPerSample) / 8;
    header.byteRate = header.sampleRate * header.blockAlign;
    header.data[0] = 'd'; header.data[1] = 'a'; header.data[2] = 't'; header.data[3] = 'a';

    // Skip CSV header line
    std::string line;
    std::getline(inFile, line);

    // Prepare to write the WAV file
    std::vector<int16_t> audioData;

    while (std::getline(inFile, line)) {
        std::stringstream ss(line);
        std::string time, leftChannel, rightChannel;

        // Read CSV columns (time, left channel, right channel)
        std::getline(ss, time, ',');
        std::getline(ss, leftChannel, ',');
        std::getline(ss, rightChannel);

        // Convert binary strings to 16-bit samples
        int16_t leftSample = fromBinary<int16_t>(leftChannel);
        int16_t rightSample = fromBinary<int16_t>(rightChannel);

        // Store the samples in audio data
        audioData.push_back(leftSample);
        audioData.push_back(rightSample);
    }

    // Calculate the total data size and update the WAV header
    header.dataSize = audioData.size() * sizeof(int16_t);
    header.fileSize = sizeof(WAVHeader) - 8 + header.dataSize;

    // Write the WAV header
    outFile.write(reinterpret_cast<char*>(&header), sizeof(WAVHeader));

    // Write the audio data
    outFile.write(reinterpret_cast<char*>(audioData.data()), audioData.size() * sizeof(int16_t));

    inFile.close();
    outFile.close();

    std::cout << "CSV file converted to WAV successfully. Data saved to '" << outputFilename << "'.\n";
    return 0;
}
