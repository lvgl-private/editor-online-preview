const fs = require("fs");
const path = require("path");

const ALLOWED_EXTENSIONS = [".c", ".lvml"];

function isAllowedFile(filename) {
  return ALLOWED_EXTENSIONS.some((ext) => filename.endsWith(ext));
}

function createFileNode(basePath, itemPath) {
  const relativePath = "/project/" + path.relative(basePath, itemPath);
  const stats = fs.statSync(itemPath);
  const isDirectory = stats.isDirectory();

  // Skip files that aren't in allowed extensions
  if (!isDirectory && !isAllowedFile(itemPath)) {
    return null;
  }

  const node = {
    name: path.basename(itemPath),
    path: relativePath,
    type: isDirectory ? "directory" : "file",
  };

  if (node.type === "file" && node.name.endsWith(".lvml")) {
    const content = fs.readFileSync(itemPath, "utf-8");
    if (content.trimStart().startsWith("<tests")) {
      node.isTest = true;
    }
  }

  if (isDirectory) {
    const items = fs.readdirSync(itemPath);
    const children = items
      .map((item) => createFileNode(basePath, path.join(itemPath, item)))
      .filter(Boolean) // Remove null entries
      .sort((a, b) => {
        if (a.type !== b.type) return a.type === "directory" ? -1 : 1;
        return a.name.localeCompare(b.name);
      });

    // Only include directory if it has matching files
    if (children.length > 0) {
      node.children = children;
      return node;
    }
    return null;
  }

  return node;
}

function generateManifest(directoryPath) {
  const manifest = createFileNode(directoryPath, directoryPath);
  fs.writeFileSync("project-manifest.json", JSON.stringify(manifest, null, 2));
}

// Get directory from command line args
const targetDir = process.argv[2];
if (!targetDir) {
  console.error("Please provide a directory path");
  console.error("Usage: node generateManifest.js <directory-path>");
  process.exit(1);
}

const fullPath = path.resolve(targetDir);
if (!fs.existsSync(fullPath)) {
  console.error(`Directory not found: ${fullPath}`);
  process.exit(1);
}

try {
  generateManifest(fullPath);
  console.log(`Manifest generated for: ${fullPath}`);
} catch (error) {
  console.error("Error generating manifest:", error);
  process.exit(1);
}
