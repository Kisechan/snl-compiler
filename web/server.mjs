import { spawn } from "node:child_process";
import http from "node:http";
import { access, mkdtemp, readFile, readdir, rm, stat, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const projectRoot = path.resolve(__dirname, "..");
const publicRoot = path.join(__dirname, "public");
const testsRoot = path.join(projectRoot, "tests");
const defaultCompilerPath = process.platform === "win32"
  ? path.join(projectRoot, "build", "Debug", "snlc.exe")
  : path.join(projectRoot, "build", "snlc");
const compilerPath = process.env.SNLC_BIN || defaultCompilerPath;
const host = process.env.HOST || "127.0.0.1";
const port = Number(process.env.PORT || 5173);
const timeoutMs = 5000;

const sampleGroups = [
  { id: "valid", label: "正确样例" },
  { id: "lexical_errors", label: "词法错误" },
  { id: "syntax_errors", label: "语法错误" },
  { id: "semantic_errors", label: "语义错误" },
  { id: "mips", label: "MIPS 生成" },
];

const stageOrder = [
  { key: "tokens", name: "Lexical", args: (sourcePath) => ["--tokens", sourcePath] },
  { key: "ast", name: "Syntax", args: (sourcePath) => ["--ast", sourcePath] },
  { key: "semantic", name: "Semantic", args: (sourcePath) => ["--semantic", sourcePath] },
  { key: "mips", name: "Codegen", args: (sourcePath, asmPath) => ["--mips", sourcePath, "-o", asmPath] },
];

const mimeTypes = {
  ".html": "text/html; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".json": "application/json; charset=utf-8",
  ".svg": "image/svg+xml",
};

async function main() {
  try {
    await access(compilerPath);
  } catch {
    console.error(`snlc executable was not found at ${compilerPath}`);
    console.error("Build it with:");
    console.error("  cmake -S . -B build");
    console.error("  cmake --build build");
    process.exit(1);
  }

  const server = http.createServer((req, res) => {
    handleRequest(req, res).catch((error) => {
      sendJson(res, 500, {
        error: "internal_server_error",
        message: error instanceof Error ? error.message : String(error),
      });
    });
  });

  server.listen(port, host, () => {
    console.log(`SNL Compiler Workbench: http://${host}:${port}`);
  });
}

async function handleRequest(req, res) {
  const requestUrl = new URL(req.url || "/", `http://${host}:${port}`);

  if (req.method === "GET" && requestUrl.pathname === "/api/samples") {
    return sendJson(res, 200, await loadSamples());
  }

  if (req.method === "POST" && requestUrl.pathname === "/api/compile") {
    const body = await readJsonBody(req);
    if (!body || typeof body.source !== "string") {
      return sendJson(res, 400, {
        error: "bad_request",
        message: "Expected JSON body: { source: string }",
      });
    }
    return sendJson(res, 200, await compileSource(body.source, body.enableCodegen === true));
  }

  if (req.method !== "GET" && req.method !== "HEAD") {
    return sendJson(res, 405, {
      error: "method_not_allowed",
      message: "Only GET and POST are supported.",
    });
  }

  return serveStatic(requestUrl.pathname, req, res);
}

async function loadSamples() {
  const groups = [];

  for (const group of sampleGroups) {
    const groupDir = path.join(testsRoot, group.id);
    const samples = [];
    let entries = [];
    try {
      entries = await readdir(groupDir, { withFileTypes: true });
    } catch {
      groups.push({ ...group, samples });
      continue;
    }

    for (const entry of entries.filter((item) => item.isFile() && item.name.endsWith(".snl"))) {
      const filePath = path.join(groupDir, entry.name);
      const content = await readFile(filePath, "utf8");
      samples.push({
        id: `${group.id}/${entry.name}`,
        name: entry.name.replace(/\.snl$/u, ""),
        path: path.relative(projectRoot, filePath),
        content,
      });
    }

    samples.sort((a, b) => a.name.localeCompare(b.name));
    groups.push({ ...group, samples });
  }

  return { groups };
}

async function compileSource(source, enableCodegen) {
  const tempRoot = await mkdtemp(path.join(os.tmpdir(), "snl-web-"));
  const sourcePath = path.join(tempRoot, "input.snl");
  const asmPath = path.join(tempRoot, "output.asm");
  const stages = [];
  const diagnostics = [];
  let mips = "";

  try {
    await writeFile(sourcePath, source, "utf8");

    const enabledStages = enableCodegen
      ? stageOrder
      : stageOrder.filter((stage) => stage.key !== "mips");

    for (let index = 0; index < enabledStages.length; index += 1) {
      const stage = enabledStages[index];
      const result = await runCompiler(stage.args(sourcePath, asmPath));
      const stageDiagnostics = parseDiagnostics(result.stderr);
      diagnostics.push(...stageDiagnostics);

      let stdout = result.stdout;
      if (stage.key === "mips" && result.exitCode === 0) {
        mips = await readFile(asmPath, "utf8");
        stdout = mips;
      }

      stages.push({
        key: stage.key,
        name: stage.name,
        status: result.exitCode === 0 ? "success" : "error",
        stdout,
        stderr: result.stderr,
        exitCode: result.exitCode,
        timedOut: result.timedOut,
      });

      if (result.exitCode !== 0) {
        for (const skipped of enabledStages.slice(index + 1)) {
          stages.push({
            key: skipped.key,
            name: skipped.name,
            status: "skipped",
            stdout: "",
            stderr: "Skipped because a previous stage failed.",
            exitCode: null,
            timedOut: false,
          });
        }
        break;
      }
    }

    return { stages, diagnostics, mips };
  } finally {
    await rm(tempRoot, { recursive: true, force: true });
  }
}

function runCompiler(args) {
  return new Promise((resolve) => {
    const child = spawn(compilerPath, args, {
      cwd: projectRoot,
      stdio: ["ignore", "pipe", "pipe"],
    });

    let stdout = "";
    let stderr = "";
    let timedOut = false;
    const timer = setTimeout(() => {
      timedOut = true;
      child.kill("SIGKILL");
    }, timeoutMs);

    child.stdout.setEncoding("utf8");
    child.stderr.setEncoding("utf8");
    child.stdout.on("data", (chunk) => {
      stdout += chunk;
    });
    child.stderr.on("data", (chunk) => {
      stderr += chunk;
    });
    child.on("error", (error) => {
      clearTimeout(timer);
      resolve({
        stdout,
        stderr: `${stderr}${error.message}\n`,
        exitCode: 127,
        timedOut,
      });
    });
    child.on("close", (code) => {
      clearTimeout(timer);
      resolve({
        stdout,
        stderr: timedOut ? `${stderr}command-line error: compiler timed out\n` : stderr,
        exitCode: timedOut ? 124 : code ?? 1,
        timedOut,
      });
    });
  });
}

function parseDiagnostics(text) {
  const diagnostics = [];
  const pattern = /^(lexical|syntax|semantic|codegen|command-line|file) error(?: at (\d+):(\d+))?: (.*)$/gim;
  let match;

  while ((match = pattern.exec(text)) !== null) {
    diagnostics.push({
      stage: match[1].toLowerCase(),
      line: match[2] ? Number(match[2]) : null,
      column: match[3] ? Number(match[3]) : null,
      message: match[4],
      raw: match[0],
    });
  }

  return diagnostics;
}

async function readJsonBody(req) {
  const chunks = [];
  let total = 0;

  for await (const chunk of req) {
    total += chunk.length;
    if (total > 1024 * 1024) {
      throw new Error("Request body is too large.");
    }
    chunks.push(chunk);
  }

  const text = Buffer.concat(chunks).toString("utf8");
  return text ? JSON.parse(text) : null;
}

async function serveStatic(pathname, req, res) {
  const requested = pathname === "/" ? "/index.html" : pathname;
  const decoded = decodeURIComponent(requested);
  const filePath = path.normalize(path.join(publicRoot, decoded));

  if (!filePath.startsWith(publicRoot)) {
    return sendJson(res, 403, {
      error: "forbidden",
      message: "Requested path is outside the public directory.",
    });
  }

  try {
    const fileStat = await stat(filePath);
    if (!fileStat.isFile()) {
      return sendJson(res, 404, {
        error: "not_found",
        message: "File not found.",
      });
    }
    const content = req.method === "HEAD" ? "" : await readFile(filePath);
    res.writeHead(200, {
      "Content-Type": mimeTypes[path.extname(filePath)] || "application/octet-stream",
      "Cache-Control": "no-store",
    });
    return res.end(content);
  } catch {
    return sendJson(res, 404, {
      error: "not_found",
      message: "File not found.",
    });
  }
}

function sendJson(res, statusCode, payload) {
  res.writeHead(statusCode, {
    "Content-Type": "application/json; charset=utf-8",
    "Cache-Control": "no-store",
  });
  res.end(JSON.stringify(payload));
}

main();
