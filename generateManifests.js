import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

function createFileNode(basePath, itemPath, prefix) {
  const relativePath = prefix + path.relative(basePath, itemPath);
  const stats = fs.statSync(itemPath);
  const isDirectory = stats.isDirectory();

  const isAllowedFile = (filePath) => {
    const filesToIgnore = ["generateManifests.js", "project-manifest.json", "resources-manifest.json", ".DS_Store"];
    if (filesToIgnore.includes(path.basename(filePath))) {
      return false;
    }
    const allowedExtensions = [".c", ".h", ".xml", ".ttf", ".bmp", ".jpg", ".jpeg", ".svg", ".png"];
    return allowedExtensions.some((ext) => filePath.endsWith(ext));
  };

  // Skip files that aren't in allowed extensions
  if (!isDirectory && !isAllowedFile(itemPath)) {
    return null;
  }

  const node = {
    name: path.basename(itemPath),
    path: relativePath,
    type: isDirectory ? "directory" : "file",
    xmlType: "test" | "component" | "screen" | "widget",
  };

  if (node.type === "file") {
    const content = fs.readFileSync(itemPath, "utf-8");
    if (content.trimStart().startsWith("<tests")) {
      node.xmlType = "test";
    }

    if (content.trimStart().startsWith("<component")) {
      node.xmlType = "component";
    }

    if (content.trimStart().startsWith("<screen")) {
      node.xmlType = "screen";
    }

    if (content.trimStart().startsWith("<widget")) {
      node.xmlType = "widget";
    }
  }

  if (isDirectory) {
    const items = fs.readdirSync(itemPath);
    const children = items
      .map((item) => createFileNode(basePath, path.join(itemPath, item), prefix))
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

// Only run as a script if called directly
if (import.meta.url === `file://${process.argv[1]}`) {
  // Get directory from command line args
  const targetDir = process.argv[2];
  if (!targetDir) {
    console.error("Please provide a directory path");
    console.error("Usage: node generateManifests.js <directory-path>");
    process.exit(1);
  }

  const prefix = process.argv[3] || "";

  const fullPath = path.resolve(targetDir);
  if (!fs.existsSync(fullPath)) {
    console.error(`Directory not found: ${fullPath}`);
    process.exit(1);
  }

  try {
    generateProjectManifest(fullPath, targetDir, true);
    generateResourcesManifest("resources/lvgl/xmls/", "resources/lvgl/xmls/", true);
    console.log(`Manifests generated for: ${fullPath}`);
  } catch (error) {
    console.error("Error generating manifest:", error);
    process.exit(1);
  }
}

export function generateProjectManifest(directoryPath, prefix, writeToFile = false) {
  const manifest = createFileNode(directoryPath, directoryPath, prefix);

  if (writeToFile) {
    fs.writeFileSync("project-manifest.json", JSON.stringify(manifest, null, 2));
  }

  return manifest;
}

export function generateResourcesManifest(directoryPath, prefix, writeToFile = false) {
  const manifest = createFileNode(directoryPath, directoryPath, prefix);

  if (writeToFile) {
    fs.writeFileSync("resources-manifest.json", JSON.stringify(manifest, null, 2));
  }

  return manifest;
}
