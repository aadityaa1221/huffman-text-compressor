# Huffman Text Compressor 🗜️

A full-stack web application that carries out lossless compression and decompression of text files through the implementation of the **Huffman Coding Algorithm**. 

The compression logic is implemented from scratch using **C++**, ensuring optimal memory handling and manipulation at the bit level, whereas the user interface and API are powered using a **Node.js** backend.

## 🚀 Features
* **Lossless Compression:** Highly compresses common text files by substituting 8-bit ASCII characters with variable length bits based on their frequency.
* **Deterministic Decompression:** Makes use of deterministic ID-based tie-breaking for priority queue for mathematically guaranteed tree construction during both the compression and decompression stages.
* **Bit Packer:** Uses C++ bit-wise operators for packing the produced 1s and 0s into 8-bit bytes in order to produce a real `.huff` binary file.
* **Full Stack Integration:** A strong Node.js (with `Express` and `Multer`) integration serves as a reliable bridge for file streams from the browser to the compiled C++ program.

## 💡 Technologies Used
* **Engine:** C++ (STL)
* **Backend:** Node.js, Express.js, child_process
* **Middleware:** Multer (Multipart file handling)
* **Frontend:** HTML5, CSS3

## 🔰 How The Engine Works
1. **Frequency Calculation:** Takes the input file and counts how many times every individual character appears in the file.
2. **Building the Min-Heap Tree:** Inserts all characters into Custom Priority Queue (Min-Heap). It keeps combining the two nodes with lowest frequency till the tree gets only one root.
3. **Creating Prefix Codes:** Traversing the tree (DFS). If you go left – adding '0' to the prefix; if go right – adding '1'. So the more frequent characters will have short prefix codes (10), while less frequent - long ones (11010).
4. **Encoding and Bit Packing:** Encodes the text with the newly created binary strings using bitwise operations (bitwise shifts << and bitwise or |).
5. **Decoding:** Decodes the `.huff` file bit-by-bit, employing the bitwise `&` operation for picking up the bits and decoding the message using the binary tree structure.

## 💻 Running the Project Locally

### Requirements
* Node.js
* C++ Compiler (g++)

### Installation Steps
1. **Clone the repository:**
   ```bash
   git clone [https://github.com/aadityaa1221/huffman-text-compressor.git](https://github.com/aadityaa1221/huffman-text-compressor.git)
   cd huffman-text-compressor
