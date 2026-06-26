const express = require("express");
const multer  = require("multer");
const { execFile } = require("child_process");
const path    = require("path");
const fs      = require("fs");

const app = express();
const UPLOADS_DIR = path.join(__dirname, "uploads");

if (!fs.existsSync(UPLOADS_DIR)) fs.mkdirSync(UPLOADS_DIR, { recursive: true });

const upload = multer({ dest: UPLOADS_DIR });

app.use(express.static(path.join(__dirname, "public")));

function tryUnlink(...paths) {
  for (const p of paths) {
    try { if (fs.existsSync(p)) fs.unlinkSync(p); } catch (e) { console.error("unlink error:", e.message); }
  }
}

function runHuffman(mode, inputPath, outputPath) {
  const binaryPath = path.resolve(__dirname, "huffman");

  console.log(`[runHuffman] mode=${mode}`);
  console.log(`[runHuffman] binary=${binaryPath} exists=${fs.existsSync(binaryPath)}`);
  console.log(`[runHuffman] input=${inputPath}  exists=${fs.existsSync(inputPath)}`);
  console.log(`[runHuffman] output=${outputPath}`);

  if (!fs.existsSync(binaryPath)) {
    return Promise.reject(new Error(
      `huffman binary not found at ${binaryPath}. Compile with: g++ -O2 -o huffman huffman.cpp`
    ));
  }

  return new Promise((resolve, reject) => {
    execFile(binaryPath, [mode, inputPath, outputPath], (error, stdout, stderr) => {
      console.log(`[runHuffman] stdout: ${stdout}`);
      console.log(`[runHuffman] stderr: ${stderr}`);
      console.log(`[runHuffman] output exists after run: ${fs.existsSync(outputPath)}`);

      if (error) {
        return reject(new Error(`Process error: ${error.message}\nstderr: ${stderr}`));
      }
      if (!fs.existsSync(outputPath)) {
        return reject(new Error(`Binary ran OK but output file was not created.\nstdout: ${stdout}\nstderr: ${stderr}`));
      }
      resolve(stdout);
    });
  });
}

// Manual file send — bypasses res.download entirely
function sendFile(res, filePath, downloadName) {
  const stat = fs.statSync(filePath);
  console.log(`[sendFile] sending ${filePath} (${stat.size} bytes) as "${downloadName}"`);

  res.writeHead(200, {
    "Content-Type": "application/octet-stream",
    "Content-Disposition": `attachment; filename="${downloadName}"`,
    "Content-Length": stat.size,
  });

  const stream = fs.createReadStream(filePath);
  stream.on("error", (err) => console.error("[sendFile] stream error:", err.message));
  stream.pipe(res);
}

// ── COMPRESS ──────────────────────────────────────────────────────────────────
app.post("/compress", upload.single("file"), async (req, res) => {
  if (!req.file) return res.status(400).send("No file uploaded.");

  const inputPath  = path.resolve(req.file.path);
  const outputPath = inputPath + ".huff";

  console.log(`\n[/compress] uploaded: ${req.file.originalname}`);

  try {
    await runHuffman("compress", inputPath, outputPath);
    sendFile(res, outputPath, "compressed.huff");
    // Clean up after pipe finishes
    res.on("finish", () => tryUnlink(inputPath, outputPath));
  } catch (err) {
    console.error("[/compress] ERROR:", err.message);
    tryUnlink(inputPath, outputPath);
    if (!res.headersSent) res.status(500).send("Compression failed:\n" + err.message);
  }
});

// ── DECOMPRESS ────────────────────────────────────────────────────────────────
app.post("/decompress", upload.single("file"), async (req, res) => {
  if (!req.file) return res.status(400).send("No file uploaded.");

  const inputPath  = path.resolve(req.file.path);
  const outputPath = inputPath + ".restored.txt";

  console.log(`\n[/decompress] uploaded: ${req.file.originalname}`);

  try {
    await runHuffman("decompress", inputPath, outputPath);
    sendFile(res, outputPath, "restored.txt");
    res.on("finish", () => tryUnlink(inputPath, outputPath));
  } catch (err) {
    console.error("[/decompress] ERROR:", err.message);
    tryUnlink(inputPath, outputPath);
    if (!res.headersSent) res.status(500).send("Decompression failed:\n" + err.message);
  }
});

// ── START ─────────────────────────────────────────────────────────────────────
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`Server running at http://localhost:${PORT}`);
  const binaryPath = path.resolve(__dirname, "huffman");
  if (!fs.existsSync(binaryPath)) {
    console.warn(`\n⚠️  huffman binary NOT found. Run: g++ -O2 -o huffman huffman.cpp\n`);
  } else {
    console.log(`✓ huffman binary found at: ${binaryPath}`);
  }
});