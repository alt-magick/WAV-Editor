#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>
#include <sstream>
#include <iomanip> // For formatting the time

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

// Convert a number to an n-bit binary string
template <typename T>
std::string toBinary(T value, int bits) {
    return std::bitset<32>(value).to_string().substr(32 - bits);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input WAV file> <output CSV file>\n";
        return 1;
    }

    std::string inputFilename = argv[1];  // First command-line argument
    std::string outputFilename = argv[2]; // Second command-line argument

    std::ifstream file(inputFilename, std::ios::binary);
    std::ofstream outFile(outputFilename);

    if (!file) {
        std::cerr << "Error: Could not open WAV file!\n";
        return 1;
    }
    if (!outFile) {
        std::cerr << "Error: Could not create output CSV file!\n";
        return 1;
    }

    // Read WAV header
    WAVHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    // Validate WAV format
    if (std::string(header.riff, 4) != "RIFF" || std::string(header.wave, 4) != "WAVE") {
        std::cerr << "Error: Not a valid WAV file!\n";
        return 1;
    }

    std::cout << "Sample Rate: " << header.sampleRate << " Hz\n";
    std::cout << "Channels: " << header.numChannels << "\n";
    std::cout << "Bit Depth: " << header.bitsPerSample << "-bit\n";

    outFile << "Time,Left_Channel,Right_Channel\n"; // CSV header

    // Read audio samples
    int bytesPerSample = header.bitsPerSample / 8;
    int numSamples = header.dataSize / (bytesPerSample * header.numChannels);

    std::vector<char> sampleBuffer(bytesPerSample * header.numChannels);

    for (int i = 0; i < numSamples; i++) {
        file.read(sampleBuffer.data(), sampleBuffer.size());

        std::stringstream leftBinary, rightBinary;

        // Process left and right channels
        int32_t leftSample = 0, rightSample = 0;
        if (header.numChannels == 1) {
            // Mono: Only one sample
            leftSample = rightSample = 0;
            for (int j = 0; j < bytesPerSample; j++) {
                leftSample |= (static_cast<uint8_t>(sampleBuffer[j]) << (j * 8));
            }
            if (header.bitsPerSample == 16) {
                leftSample = static_cast<int16_t>(leftSample);
            }
            else if (header.bitsPerSample == 32) {
                leftSample = static_cast<int32_t>(leftSample);
            }
            rightSample = leftSample;
        }
        else if (header.numChannels == 2) {
            // Stereo: Separate left and right channels
            for (int j = 0; j < bytesPerSample; j++) {
                leftSample |= (static_cast<uint8_t>(sampleBuffer[j]) << (j * 8));
                rightSample |= (static_cast<uint8_t>(sampleBuffer[bytesPerSample + j]) << (j * 8));
            }
            if (header.bitsPerSample == 16) {
                leftSample = static_cast<int16_t>(leftSample);
                rightSample = static_cast<int16_t>(rightSample);
            }
            else if (header.bitsPerSample == 32) {
                leftSample = static_cast<int32_t>(leftSample);
                rightSample = static_cast<int32_t>(rightSample);
            }
        }

        // Convert to binary
        leftBinary << toBinary(leftSample, header.bitsPerSample);
        rightBinary << toBinary(rightSample, header.bitsPerSample);

        // Calculate time in seconds
        double timeInSeconds = static_cast<double>(i) / header.sampleRate;

        // Format time to display seconds with 6 decimal places
        outFile << std::fixed << std::setprecision(6) << timeInSeconds << ","
            << leftBinary.str() << "," << rightBinary.str() << "\n";
    }

    file.close();
    outFile.close();

    std::cout << "Audio sample extraction completed. Data saved to '" << outputFilename << "'.\n";
    return 0;
}
