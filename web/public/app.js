const pipelineEl = document.querySelector("#pipeline");
const sampleSelect = document.querySelector("#sampleSelect");
const runButton = document.querySelector("#runButton");
const resetButton = document.querySelector("#resetButton");
const sourceInput = document.querySelector("#sourceInput");
const lineGutter = document.querySelector("#lineGutter");
const lineHighlight = document.querySelector("#lineHighlight");
const tabsEl = document.querySelector("#tabs");
const stageSummary = document.querySelector("#stageSummary");
const tabPanel = document.querySelector("#tabPanel");

const tabDefs = [
  { id: "tokens", label: "Tokens", stageKey: "tokens" },
  { id: "ast", label: "AST", stageKey: "ast" },
  { id: "semantic", label: "Semantic", stageKey: "semantic" },
  { id: "mips", label: "MIPS", stageKey: "mips" },
  { id: "diagnostics", label: "Diagnostics" },
];

const pipelineDefs = [
  { key: "tokens", label: "Lexical" },
  { key: "ast", label: "Syntax" },
  { key: "semantic", label: "Semantic" },
  { key: "mips", label: "Codegen" },
];

const state = {
  samples: [],
  selectedSample: null,
  activeTab: "tokens",
  result: null,
  running: false,
};

init();

async function init() {
  renderPipeline();
  renderTabs();
  renderStageSummary();
  renderPanel();
  bindEvents();
  updateLineGutter();
  await loadSamples();
}

function bindEvents() {
  sampleSelect.addEventListener("change", () => {
    const sample = state.samples.find((item) => item.id === sampleSelect.value);
    if (sample) {
      state.selectedSample = sample;
      sourceInput.value = sample.content;
      state.result = null;
      updateLineGutter();
      clearHighlight();
      if (!isCodegenEnabled()) {
        state.activeTab = state.activeTab === "mips" ? "tokens" : state.activeTab;
      }
      renderPipeline();
      renderTabs();
      renderStageSummary();
      renderPanel();
    }
  });

  runButton.addEventListener("click", () => {
    compileCurrentSource();
  });

  resetButton.addEventListener("click", () => {
    if (state.selectedSample) {
      sourceInput.value = state.selectedSample.content;
      updateLineGutter();
      clearHighlight();
    }
  });

  sourceInput.addEventListener("input", () => {
    updateLineGutter();
    clearHighlight();
  });

  sourceInput.addEventListener("scroll", () => {
    lineGutter.scrollTop = sourceInput.scrollTop;
    if (lineHighlight.dataset.line) {
      positionHighlight(Number(lineHighlight.dataset.line));
    }
  });
}

async function loadSamples() {
  try {
    const response = await fetch("/api/samples");
    const payload = await response.json();
    const groups = payload.groups || [];
    state.samples = groups.flatMap((group) =>
      group.samples.map((sample) => ({
        ...sample,
        groupId: group.id,
        groupLabel: group.label,
      })),
    );
    renderSampleSelect(groups);

    const defaultSample =
      state.samples.find((item) => item.id === "mips/while_sum.snl") ||
      state.samples.find((item) => item.id === "valid/full_syntax.snl") ||
      state.samples[0];

    if (defaultSample) {
      state.selectedSample = defaultSample;
      sampleSelect.value = defaultSample.id;
      sourceInput.value = defaultSample.content;
      updateLineGutter();
    }
  } catch (error) {
    tabPanel.innerHTML = `<div class="empty-state">示例加载失败：${escapeHtml(error.message)}</div>`;
  }
}

function renderSampleSelect(groups) {
  sampleSelect.innerHTML = "";
  for (const group of groups) {
    const optgroup = document.createElement("optgroup");
    optgroup.label = group.label;
    for (const sample of group.samples) {
      const option = document.createElement("option");
      option.value = sample.id;
      option.textContent = `${group.id}/${sample.name}`;
      optgroup.append(option);
    }
    sampleSelect.append(optgroup);
  }
}

async function compileCurrentSource() {
  state.running = true;
  state.result = null;
  clearHighlight();
  setRunState(true);
  renderPipeline();
  renderStageSummary();
  renderPanel("编译运行中...");

  try {
    const response = await fetch("/api/compile", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        source: sourceInput.value,
        enableCodegen: isCodegenEnabled(),
      }),
    });
    state.result = await response.json();
    const firstDiagnostic = state.result.diagnostics?.find((item) => item.line);
    if (firstDiagnostic) {
      highlightLine(firstDiagnostic.line);
    }
    if (firstDiagnostic) {
      state.activeTab = "diagnostics";
    } else {
      state.activeTab = state.result.mips && isCodegenEnabled() ? "mips" : "semantic";
    }
  } catch (error) {
    state.result = {
      stages: [],
      diagnostics: [{ stage: "client", line: null, column: null, message: error.message }],
      mips: "",
    };
    state.activeTab = "diagnostics";
  } finally {
    state.running = false;
    setRunState(false);
    renderPipeline();
    renderTabs();
    renderStageSummary();
    renderPanel();
  }
}

function setRunState(isRunning) {
  runButton.disabled = isRunning;
  runButton.querySelector("span").textContent = isRunning ? "Running" : "Run";
}

function renderPipeline() {
  const stages = state.result?.stages || [];
  pipelineEl.innerHTML = visiblePipelineDefs()
    .map((def) => {
      const stage = stages.find((item) => item.key === def.key);
      const status = stage?.status || (state.running ? "queued" : "idle");
      return `
        <div class="pipeline-step ${escapeHtml(status)}">
          <span class="pipeline-name">${escapeHtml(def.label)}</span>
          <span class="pipeline-state">${escapeHtml(status)}</span>
        </div>
      `;
    })
    .join("");
}

function renderTabs() {
  tabsEl.innerHTML = visibleTabDefs()
    .map((tab) => `
      <button class="tab-button ${tab.id === state.activeTab ? "active" : ""}" type="button" data-tab="${tab.id}">
        ${escapeHtml(tab.label)}
      </button>
    `)
    .join("");

  tabsEl.querySelectorAll(".tab-button").forEach((button) => {
    button.addEventListener("click", () => {
      state.activeTab = button.dataset.tab;
      renderTabs();
      renderPanel();
    });
  });
}

function renderStageSummary() {
  const stages = state.result?.stages || [];
  if (!stages.length) {
    stageSummary.innerHTML = `<span class="summary-chip">等待运行</span>`;
    return;
  }

  stageSummary.innerHTML = stages
    .map((stage) => `
      <span class="summary-chip ${escapeHtml(stage.status)}">
        ${escapeHtml(stage.name)} · ${escapeHtml(stage.status)}
      </span>
    `)
    .join("");
}

function renderPanel(message) {
  if (message) {
    tabPanel.innerHTML = `<div class="empty-state">${escapeHtml(message)}</div>`;
    return;
  }

  if (!state.result) {
    tabPanel.innerHTML = `<div class="empty-state">选择示例或输入源码后点击 Run</div>`;
    return;
  }

  if (state.activeTab === "diagnostics") {
    renderDiagnostics();
    return;
  }

  if (state.activeTab === "mips" && !isCodegenEnabled()) {
    state.activeTab = "semantic";
  }

  const tab = visibleTabDefs().find((item) => item.id === state.activeTab);
  const stage = state.result.stages.find((item) => item.key === tab?.stageKey);
  if (!stage) {
    tabPanel.innerHTML = `<div class="empty-state">暂无输出</div>`;
    return;
  }

  const output = [stage.stdout, stage.stderr].filter(Boolean).join("\n");
  tabPanel.innerHTML = output
    ? `<pre class="output-block">${escapeHtml(output)}</pre>`
    : `<div class="empty-state">${escapeHtml(stage.name)} 没有文本输出</div>`;
}

function renderDiagnostics() {
  const diagnostics = state.result?.diagnostics || [];

  if (!diagnostics.length) {
    tabPanel.innerHTML = `<div class="empty-state">本次编译没有诊断信息</div>`;
    return;
  }

  tabPanel.innerHTML = `
    <div class="diagnostics">
      ${diagnostics.map((diagnostic) => renderDiagnosticRow(diagnostic)).join("")}
    </div>
  `;

  tabPanel.querySelectorAll(".diagnostic-row").forEach((row) => {
    row.addEventListener("click", () => {
      const line = Number(row.dataset.line);
      if (line) {
        highlightLine(line);
      }
    });
  });
}

function visibleTabDefs() {
  return isCodegenEnabled()
    ? tabDefs
    : tabDefs.filter((tab) => tab.id !== "mips");
}

function visiblePipelineDefs() {
  return isCodegenEnabled()
    ? pipelineDefs
    : pipelineDefs.filter((stage) => stage.key !== "mips");
}

function isCodegenEnabled() {
  return state.selectedSample?.groupId === "mips";
}

function renderDiagnosticRow(diagnostic) {
  return `
    <article class="diagnostic-row" data-line="${diagnostic.line || ""}">
      <div>
        <div class="diagnostic-label">Stage</div>
        <div class="diagnostic-value diagnostic-stage">${escapeHtml(diagnostic.stage)}</div>
      </div>
      <div>
        <div class="diagnostic-label">Line</div>
        <div class="diagnostic-value">${escapeHtml(diagnostic.line ?? "-")}</div>
      </div>
      <div>
        <div class="diagnostic-label">Column</div>
        <div class="diagnostic-value">${escapeHtml(diagnostic.column ?? "-")}</div>
      </div>
      <div>
        <div class="diagnostic-label">Message</div>
        <div class="diagnostic-value">${escapeHtml(diagnostic.message)}</div>
      </div>
    </article>
  `;
}

function updateLineGutter() {
  const lineCount = sourceInput.value.split("\n").length;
  lineGutter.textContent = Array.from({ length: lineCount }, (_, index) => index + 1).join("\n");
}

function highlightLine(line) {
  lineHighlight.dataset.line = String(line);
  positionHighlight(line);
}

function positionHighlight(line) {
  const lineHeight = 21;
  const topPadding = 16;
  lineHighlight.style.top = `${topPadding + (line - 1) * lineHeight - sourceInput.scrollTop}px`;
}

function clearHighlight() {
  delete lineHighlight.dataset.line;
  lineHighlight.style.top = "-100px";
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}
