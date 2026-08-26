"use strict";

const fs = require("node:fs");
const path = require("node:path");
const esbuild = require("esbuild");

const root = __dirname;
const outputPath = path.join(root, "game.js");
const result = esbuild.buildSync({
  entryPoints: [path.join(root, "game-3d.source.js")],
  bundle: true,
  format: "iife",
  platform: "browser",
  target: "es2020",
  minify: true,
  legalComments: "inline",
  metafile: true,
  outfile: outputPath,
  write: false,
});

const externalImports = Object.values(result.metafile.outputs)
  .flatMap((output) => output.imports || [])
  .filter((entry) => entry.external);

if (externalImports.length > 0) {
  throw new Error(`Browser bundle still has external imports: ${externalImports.map((entry) => entry.path).join(", ")}`);
}

const bundle = result.outputFiles[0].text;
const networkPrimitives = [
  /\bfetch\s*\(/,
  /\bXMLHttpRequest\b/,
  /\bWebSocket\b/,
  /\bEventSource\b/,
  /\bsendBeacon\b/,
];
if (networkPrimitives.some((pattern) => pattern.test(bundle))) {
  throw new Error("Browser bundle contains a runtime network primitive; this game must remain offline-only.");
}
fs.writeFileSync(outputPath, bundle, "utf8");

const inputs = Object.keys(result.metafile.inputs).length;
const bytes = Object.values(result.metafile.outputs)
  .reduce((total, output) => total + (output.bytes || 0), 0);
console.log(`Built ${path.basename(outputPath)} from ${inputs} modules (${(bytes / 1024).toFixed(1)} KiB).`);
