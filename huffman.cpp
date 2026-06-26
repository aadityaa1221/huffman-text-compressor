#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <queue>
#include <vector>
#include <cstdint>
#include <algorithm> // Required for std::sort

using namespace std;

// ==========================================
// PHASE 1: FREQUENCY COUNTER
// ==========================================
unordered_map<unsigned char, int> countFrequencies(const string &filename)
{
    unordered_map<unsigned char, int> freqMap;
    ifstream file(filename, ios::binary);

    if (!file.is_open())
    {
        cerr << "Error: Could not open file -> " << filename << "\n";
        return freqMap;
    }

    unsigned char ch;
    int raw;
    while ((raw = file.get()) != EOF)
    {
        freqMap[static_cast<unsigned char>(raw)]++;
    }

    file.close();
    return freqMap;
}

// ==========================================
// PHASE 2: BUILDING THE HUFFMAN TREE (DETERMINISTIC)
// ==========================================
struct Node
{
    unsigned char ch;
    int freq;
    int id; // SDE FIX: A unique ID to strictly enforce tie-breaking
    Node *left;
    Node *right;

    Node(unsigned char character, int frequency, int nodeId)
        : ch(character), freq(frequency), id(nodeId), left(nullptr), right(nullptr) {}

    Node(int frequency, Node *leftChild, Node *rightChild, int nodeId)
        : ch(0), freq(frequency), id(nodeId), left(leftChild), right(rightChild) {}
};

struct Compare
{
    bool operator()(Node *a, Node *b)
    {
        // Primary: higher frequency = lower priority (min-heap)
        if (a->freq != b->freq) return a->freq > b->freq;
        
        // Secondary (STRICT DETERMINISM): If frequencies are tied, 
        // the node created first (lower ID) gets prioritized.
        return a->id > b->id;
    }
};

Node *buildHuffmanTree(unordered_map<unsigned char, int> &freqMap)
{
    priority_queue<Node *, vector<Node *>, Compare> pq;

    // 1. Sort the map entries alphabetically to guarantee identical initial insertion order
    vector<pair<unsigned char, int>> sortedFreqs(freqMap.begin(), freqMap.end());
    sort(sortedFreqs.begin(), sortedFreqs.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    int idCounter = 0; // Tracks the exact order nodes are created

    // 2. Push all leaf nodes deterministically
    for (auto &pair : sortedFreqs) {
        pq.push(new Node(pair.first, pair.second, idCounter++));
    }

    // Handle edge case: File contains only one unique character repeatedly
    if (pq.size() == 1)
    {
        Node *only = pq.top(); pq.pop();
        // Give it a real left child and null right child to prevent double-free errors
        Node *parent = new Node(only->freq, only, nullptr, idCounter++);
        pq.push(parent);
    }

    // 3. Build the tree deterministically
    while (pq.size() > 1)
    {
        Node *left  = pq.top(); pq.pop();
        Node *right = pq.top(); pq.pop();
        
        // Create new parent node with combined frequency
        pq.push(new Node(left->freq + right->freq, left, right, idCounter++));
    }

    return pq.empty() ? nullptr : pq.top();
}

// ==========================================
// PHASE 3: GENERATING THE CODES
// ==========================================
void generateCodes(Node *root, string currentCode,
                   unordered_map<unsigned char, string> &huffmanCodes)
{
    if (!root) return;

    if (!root->left && !root->right)
    {
        huffmanCodes[root->ch] = currentCode.empty() ? "0" : currentCode;
        return;
    }

    generateCodes(root->left,  currentCode + "0", huffmanCodes);
    generateCodes(root->right, currentCode + "1", huffmanCodes);
}

// ==========================================
// PHASE 4: COMPRESS
// ==========================================
void compressFile(const string &inputFile, const string &outputFile,
                  unordered_map<unsigned char, string> &huffmanCodes,
                  unordered_map<unsigned char, int>    &freqMap)
{
    ifstream inFile(inputFile, ios::binary);
    ofstream outFile(outputFile, ios::binary | ios::trunc);

    if (!inFile.is_open() || !outFile.is_open())
    {
        cerr << "Error: Could not open files for compression.\n";
        return;
    }

    // --- Header: frequency table ---
    uint32_t numChars = static_cast<uint32_t>(freqMap.size());
    outFile.write(reinterpret_cast<const char *>(&numChars), sizeof(numChars));

    for (auto &pair : freqMap)
    {
        unsigned char c = pair.first;
        uint32_t freq   = static_cast<uint32_t>(pair.second);
        outFile.write(reinterpret_cast<const char *>(&c), 1);
        outFile.write(reinterpret_cast<const char *>(&freq), sizeof(freq));
    }

    // --- Header: total byte count so decompressor knows when to stop ---
    uint64_t totalBytes = 0;
    for (auto &pair : freqMap) totalBytes += pair.second;
    outFile.write(reinterpret_cast<const char *>(&totalBytes), sizeof(totalBytes));

    // --- Bitstream ---
    unsigned char buffer = 0;
    int bitCount = 0;
    int raw;

    while ((raw = inFile.get()) != EOF)
    {
        unsigned char ch = static_cast<unsigned char>(raw);
        const string &code = huffmanCodes[ch];

        for (char bit : code)
        {
            buffer = (buffer << 1) | (bit == '1' ? 1 : 0);
            if (++bitCount == 8)
            {
                outFile.write(reinterpret_cast<const char *>(&buffer), 1);
                buffer   = 0;
                bitCount = 0;
            }
        }
    }

    // Flush remaining bits (left-aligned padding)
    if (bitCount > 0)
    {
        buffer <<= (8 - bitCount);
        outFile.write(reinterpret_cast<const char *>(&buffer), 1);
    }

    inFile.close();
    outFile.close();
    cout << "Compression complete! File saved as: " << outputFile << "\n";
}

// ==========================================
// PHASE 5: DECOMPRESS
// ==========================================
void decompressFile(const string &compressedFile, const string &outputFile)
{
    ifstream inFile(compressedFile, ios::binary);
    ofstream outFile(outputFile, ios::binary);

    if (!inFile.is_open() || !outFile.is_open())
    {
        cerr << "Error: Could not open files for decompression.\n";
        return;
    }

    // --- Read frequency table ---
    uint32_t numChars = 0;
    inFile.read(reinterpret_cast<char *>(&numChars), sizeof(numChars));

    unordered_map<unsigned char, int> freqMap;
    for (uint32_t i = 0; i < numChars; i++)
    {
        unsigned char c;
        uint32_t freq;
        inFile.read(reinterpret_cast<char *>(&c), 1);
        inFile.read(reinterpret_cast<char *>(&freq), sizeof(freq));
        freqMap[c] = static_cast<int>(freq);
    }

    // --- Read total byte count ---
    uint64_t totalBytes = 0;
    inFile.read(reinterpret_cast<char *>(&totalBytes), sizeof(totalBytes));

    // --- Rebuild tree ---
    Node *root = buildHuffmanTree(freqMap);
    if (!root)
    {
        cerr << "Error: Empty or corrupt frequency table.\n";
        return;
    }

    // --- Decode ---
    Node *current       = root;
    uint64_t written    = 0;
    int raw;

    while (written < totalBytes && (raw = inFile.get()) != EOF)
    {
        unsigned char byte = static_cast<unsigned char>(raw);

        for (int i = 7; i >= 0 && written < totalBytes; i--)
        {
            // Isolate bit i
            current = ((byte >> i) & 1) ? current->right : current->left;

            if (!current)
            {
                cerr << "Error: Corrupt data (null node).\n";
                goto done;
            }

            // Leaf node = decoded one character
            if (!current->left && !current->right)
            {
                outFile.write(reinterpret_cast<const char *>(&current->ch), 1);
                written++;
                current = root;
            }
        }
    }

done:
    inFile.close();
    outFile.close();
    cout << "Decompression complete! File restored as: " << outputFile << "\n";
}

// ==========================================
// MAIN
// ==========================================
int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        cerr << "Usage: " << argv[0] << " <compress|decompress> <input> <output>\n";
        return 1;
    }

    string mode       = argv[1];
    string inputFile  = argv[2];
    string outputFile = argv[3];

    if (mode == "compress")
    {
        auto frequencies = countFrequencies(inputFile);
        if (frequencies.empty())
        {
            cerr << "Error: Input file is empty or unreadable.\n";
            return 1;
        }
        Node *root = buildHuffmanTree(frequencies);
        unordered_map<unsigned char, string> huffmanCodes;
        generateCodes(root, "", huffmanCodes);
        compressFile(inputFile, outputFile, huffmanCodes, frequencies);
    }
    else if (mode == "decompress")
    {
        decompressFile(inputFile, outputFile);
    }
    else
    {
        cerr << "Unknown mode: " << mode << ". Use 'compress' or 'decompress'.\n";
        return 1;
    }

    return 0;
}