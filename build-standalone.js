"use strict";

const fs = require("node:fs");
const path = require("node:path");

const webDir = __dirname;
const outputPath = path.join(webDir, "gorilla-golden-eye-standalone.html");

const textInputs = {
  index: "index.html",
  styles: "styles.css",
  audio: "audio.js",
  game: "game.js",
};

const assetInputs = [];

function fail(message) {
  throw new Error(message);
}

function assertInputsExist(relativePaths) {
  const missing = relativePaths.filter(
    (relativePath) => !fs.existsSync(path.join(webDir, relativePath)),
  );

  if (missing.length > 0) {
    fail(`Missing required input${missing.length === 1 ? "" : "s"}: ${missing.join(", ")}`);
  }
}

function readText(relativePath) {
  return fs.readFileSync(path.join(webDir, relativePath), "utf8");
}

function getAttribute(tag, name) {
  const attributePattern = new RegExp(
    `\\b${name}\\s*=\\s*(?:"([^"]*)"|'([^']*)'|([^\\s"'=<>\u0060]+))`,
    "i",
  );
  const match = tag.match(attributePattern);
  return match ? match[1] ?? match[2] ?? match[3] : null;
}

function normalizeLocalReference(reference) {
  if (!reference || /^(?:[a-z][a-z\d+.-]*:|\/\/|#)/i.test(reference)) {
    return null;
  }

  const withoutQuery = reference.split(/[?#]/, 1)[0].replaceAll("\\", "/");
  return path.posix.normalize(withoutQuery.replace(/^\/+/, "").replace(/^\.\//, ""));
}

function replaceReferencedTag(html, tagPattern, attribute, target, replacement) {
  let replacementCount = 0;
  const result = html.replace(tagPattern, (tag) => {
    if (normalizeLocalReference(getAttribute(tag, attribute)) !== target) {
      return tag;
    }

    replacementCount += 1;
    return replacement;
  });

  if (replacementCount !== 1) {
    fail(
      `Expected exactly one ${attribute} reference to ${target}, found ${replacementCount}`,
    );
  }

  return result;
}

function escapeClosingScriptTags(source) {
  return source.replace(/<\/script/gi, "<\\/script");
}

function escapeRegExp(value) {
  return value.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

function inlineAsset(html, assetPath, dataUrl) {
  const escapedPath = escapeRegExp(assetPath);
  const referencePattern = new RegExp(
    `(^|[\\s"'\u0060(=,:])((?:\\.\\/|\\/)?${escapedPath})(?:[?#][^\\s"'\u0060()<>]*)?(?=$|[\\s"'\u0060)>,;])`,
    "gm",
  );
  let replacementCount = 0;

  const result = html.replace(referencePattern, (_match, prefix) => {
    replacementCount += 1;
    return `${prefix}${dataUrl}`;
  });

  if (replacementCount === 0) {
    fail(`Required asset is not referenced by the source files: ${assetPath}`);
  }

  return result;
}

function isUnresolvedLocalFile(reference) {
  const value = reference.trim();
  if (!value || /^(?:data:|blob:|https?:|\/\/|#|mailto:|tel:)/i.test(value)) {
    return false;
  }

  const pathname = value.split(/[?#]/, 1)[0].replaceAll("\\", "/");
  const basename = path.posix.basename(pathname);
  return (
    basename.lastIndexOf(".") > 0 &&
    /\.(?:css|js|png|jpe?g|gif|webp|svg|ico)$/i.test(basename)
  );
}

function findUnresolvedReferences(html) {
  const references = new Set();
  const quotedReferencePattern = /(["'\u0060])([^"'\u0060\r\n\s]+)\1/g;
  const cssUrlPattern = /url\(\s*([^)]+?)\s*\)/gi;
  const resourceAttributePattern =
    /\b(?:src|href|poster)\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s"'=<>\u0060]+))/gi;

  for (const match of html.matchAll(quotedReferencePattern)) {
    if (isUnresolvedLocalFile(match[2])) {
      references.add(match[2]);
    }
  }

  for (const match of html.matchAll(cssUrlPattern)) {
    const reference = match[1].trim().replace(/^(?:"([\s\S]*)"|'([\s\S]*)')$/, "$1$2");
    if (isUnresolvedLocalFile(reference)) {
      references.add(reference);
    }
  }

  for (const match of html.matchAll(resourceAttributePattern)) {
    const reference = match[1] ?? match[2] ?? match[3];
    if (isUnresolvedLocalFile(reference)) {
      references.add(reference);
    }
  }

  return [...references].sort();
}

function findExternalResources(html) {
  const references = new Set();
  const resourceAttributePattern =
    /\b(?:src|href|poster)\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s"'=<>`]+))/gi;
  const cssUrlPattern = /url\(\s*(?:"([^"]*)"|'([^']*)'|([^\s)'"`]+))\s*\)/gi;
  const cssImportPattern = /@import\s+(?:url\(\s*)?(?:"([^"]*)"|'([^']*)'|([^\s)'";`]+))/gi;

  for (const pattern of [resourceAttributePattern, cssUrlPattern, cssImportPattern]) {
    for (const match of html.matchAll(pattern)) {
      const reference = match[1] ?? match[2] ?? match[3] ?? "";
      if (/^(?:https?:)?\/\//i.test(reference.trim())) references.add(reference.trim());
    }
  }
  return [...references].sort();
}

function formatSize(bytes) {
  const mebibytes = bytes / (1024 * 1024);
  return `${bytes} bytes (${mebibytes.toFixed(2)} MiB)`;
}

function assertOfflineOnly(source) {
  const networkPrimitives = [
    /\bfetch\s*\(/,
    /\bXMLHttpRequest\b/,
    /\bWebSocket\b/,
    /\bEventSource\b/,
    /\bsendBeacon\b/,
  ];
  if (networkPrimitives.some((pattern) => pattern.test(source))) {
    fail("Standalone build contains a runtime network primitive; this game must remain offline-only.");
  }
}

function build() {
  const requiredInputs = [...Object.values(textInputs), ...assetInputs.map((asset) => asset.path)];
  assertInputsExist(requiredInputs);

  const sources = Object.fromEntries(
    Object.entries(textInputs).map(([name, relativePath]) => [name, readText(relativePath)]),
  );

  let html = sources.index;
  html = replaceReferencedTag(
    html,
    /<link\b[^>]*>/gi,
    "href",
    textInputs.styles,
    `<style data-standalone="styles">\n${sources.styles}\n</style>`,
  );
  html = replaceReferencedTag(
    html,
    /<script\b[^>]*>[\s\S]*?<\/script\s*>/gi,
    "src",
    textInputs.audio,
    `<script data-standalone="audio">\n${escapeClosingScriptTags(sources.audio)}\n</script>`,
  );
  html = replaceReferencedTag(
    html,
    /<script\b[^>]*>[\s\S]*?<\/script\s*>/gi,
    "src",
    textInputs.game,
    `<script data-standalone="game">\n${escapeClosingScriptTags(sources.game)}\n</script>`,
  );

  for (const asset of assetInputs) {
    const contents = fs.readFileSync(path.join(webDir, asset.path));
    const dataUrl = `data:${asset.mime};base64,${contents.toString("base64")}`;
    html = inlineAsset(html, asset.path, dataUrl);
  }

  const externalResources = findExternalResources(html);
  if (externalResources.length > 0) {
    fail(`External resources are not allowed: ${externalResources.join(", ")}`);
  }

  const unresolvedReferences = findUnresolvedReferences(html);
  if (unresolvedReferences.length > 0) {
    fail(`Unresolved local references: ${unresolvedReferences.join(", ")}`);
  }

  const banner = "<!-- Self-contained build generated by web-gorilla/build-standalone.js -->\n";
  const output = `${banner}${html}`;
  assertOfflineOnly(output);
  fs.writeFileSync(outputPath, output, "utf8");

  const outputSize = fs.statSync(outputPath).size;
  console.log(`Built ${outputPath}`);
  console.log(`Output size: ${formatSize(outputSize)}`);
}

try {
  build();
} catch (error) {
  console.error(`Standalone build failed: ${error.message}`);
  process.exitCode = 1;
}
