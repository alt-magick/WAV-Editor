#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <string>
#include <cstdint>

#pragma pack(push, 1)
struct WAVHeader {
    char riff[4];         // "RIFF"
    uint32_t fileSize;    // File size - 8 bytes
    char wave[4];         // "WAVE"
    char fmt[4];          // "fmt "
    uint32_t fmtSize;     // Size of fmt chunk
    uint16_t audioFormat; // 1 = PCM
    uint16_t numChannels; // 1 = Mono, 2 = Stereo, etc.
    uint32_t sampleRate;  // Samples per second
    uint32_t byteRate;    // SampleRate * NumChannels * BitsPerSample/8
    uint16_t blockAlign;  // NumChannels * BitsPerSample/8
    uint16_t bitsPerSample; // Should be 8 for our case
    char data[4];         // "data"
    uint32_t dataSize;    // Size of audio data
};
#pragma pack(pop)

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input WAV file> <output CSV file>\n";
        return 1;
    }

    // Open the input WAV file in binary mode.
    std::ifstream inFile(argv[1], std::ios::binary);
    if (!inFile) {
        std::cerr << "Error: Could not open WAV file '" << argv[1] << "'!\n";
        return 1;
    }

    // Open the output CSV file.
    std::ofstream outFile(argv[2]);
    if (!outFile) {
        std::cerr << "Error: Could not create CSV file '" << argv[2] << "'!\n";
        return 1;
    }

    // Read the WAV header.
    WAVHeader header;
    inFile.read(reinterpret_cast<char*>(&header), sizeof(WAVHeader));

    // Verify that the file is 8-bit PCM.
    if (header.audioFormat != 1 || header.bitsPerSample != 8) {
        std::cerr << "Error: Only 8-bit PCM WAV files are supported!\n";
        return 1;
    }

    // Read the audio data into a vector.
    std::vector<uint8_t> audioData(header.dataSize);
    inFile.read(reinterpret_cast<char*>(audioData.data()), header.dataSize);
    inFile.close();

    // Calculate the number of frames (each frame contains one sample per channel).
    size_t numFrames = audioData.size() / header.numChannels;
    double timeStep = 1.0 / header.sampleRate; // time difference between samples

    // === Write a comment line with the original sample rate ===
    outFile << "# SampleRate: " << header.sampleRate << "\n";

    // Write the CSV header line.
    // Now the CSV has: Sample,Time,Channel1[,Channel2,...]
    outFile << "Sample,Time";
    for (int ch = 1; ch <= header.numChannels; ++ch) {
        outFile << ",Channel" << ch;
    }
    outFile << "\n";

    // Write each frame: sample index, exact time, and the sample(s) for that frame.
    for (size_t frame = 0; frame < numFrames; ++frame) {
        double currentTime = frame * timeStep;
        outFile << frame << "," << std::fixed << std::setprecision(6) << currentTime;
        // Write one sample for each channel.
        for (int ch = 0; ch < header.numChannels; ++ch) {
            size_t index = frame * header.numChannels + ch;
            outFile << "," << static_cast<int>(audioData[index]);
        }
        outFile << "\n";
    }

    outFile.close();
    std::cout << "WAV file converted to CSV successfully. Data saved to '" << argv[2] << "'.\n";
    return 0;
}
