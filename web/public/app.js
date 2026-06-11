const pipelineEl = document.querySelector("#pipeline");
const sampleSelect = document.querySelector("#sampleSelect");
const parserSelect = document.querySelector("#parserSelect");
const marsPathInput = document.querySelector("#marsPathInput");
const assemblyInput = document.querySelector("#assemblyInput");
const runAssemblyInput = document.querySelector("#runAssemblyInput");
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
  { id: "run", label: "Run", stageKey: "run" },
  { id: "diagnostics", label: "Diagnostics" },
];

const pipelineDefs = [
  { key: "tokens", label: "Lexical" },
  { key: "ast", label: "Syntax" },
  { key: "semantic", label: "Semantic" },
  { key: "mips", label: "Codegen" },
  { key: "run", label: "Run" },
];

const state = {
  samples: [],
  selectedSample: null,
  activeTab: "tokens",
  result: null,
  running: false,
  parserMode: "recursive",
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
      assemblyInput.value = defaultAssemblyInputForSample(sample);
      updateLineGutter();
      clearHighlight();
      if (!isCodegenEnabled()) {
        state.activeTab = state.activeTab === "mips" || state.activeTab === "run"
          ? "tokens"
          : state.activeTab;
      }
      renderPipeline();
      renderTabs();
      renderStageSummary();
      renderPanel();
      updateMipsOptionsVisibility();
    }
  });

  runButton.addEventListener("click", () => {
    compileCurrentSource();
  });

  parserSelect.addEventListener("change", () => {
    state.parserMode = parserSelect.value === "ll1" ? "ll1" : "recursive";
    state.result = null;
    clearHighlight();
    renderPipeline();
    renderStageSummary();
    renderPanel();
  });

  runAssemblyInput.addEventListener("change", () => {
    state.result = null;
    clearHighlight();
    if (!isAssemblyRunEnabled() && state.activeTab === "run") {
      state.activeTab = "mips";
    }
    renderPipeline();
    renderTabs();
    renderStageSummary();
    renderPanel();
  });

  marsPathInput.addEventListener("input", () => {
    if (state.result) {
      state.result = null;
      renderPipeline();
      renderStageSummary();
      renderPanel();
    }
  });

  assemblyInput.addEventListener("input", () => {
    if (state.result) {
      state.result = null;
      renderPipeline();
      renderStageSummary();
      renderPanel();
    }
  });

  resetButton.addEventListener("click", () => {
    if (state.selectedSample) {
      sourceInput.value = state.selectedSample.content;
      assemblyInput.value = defaultAssemblyInputForSample(state.selectedSample);
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
      state.samples.find((item) => item.id === "valid/basic_tokens.snl") ||
      state.samples.find((item) => item.id === "mips/while_sum.snl") ||
      state.samples.find((item) => item.id === "valid/full_syntax.snl") ||
      state.samples[0];

    if (defaultSample) {
      state.selectedSample = defaultSample;
      state.activeTab = "tokens";
      sampleSelect.value = defaultSample.id;
      sourceInput.value = defaultSample.content;
      assemblyInput.value = defaultAssemblyInputForSample(defaultSample);
      updateLineGutter();
      updateMipsOptionsVisibility();
      renderPipeline();
      renderTabs();
      renderStageSummary();
      renderPanel();
    }
  } catch (error) {
    tabPanel.innerHTML = `<div class="empty-state">Failed to load samples: ${escapeHtml(error.message)}</div>`;
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
  renderPanel("Running compile pipeline...");

  try {
    const response = await fetch("/api/compile", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        source: sourceInput.value,
        enableCodegen: isCodegenEnabled(),
        parserMode: state.parserMode,
        runAssembly: isAssemblyRunEnabled(),
        marsPath: marsPathInput.value,
        assemblyInput: assemblyInput.value,
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
      const runStage = state.result.stages?.find((item) => item.key === "run");
      state.activeTab = runStage ? "run" : (state.result.mips && isCodegenEnabled() ? "mips" : "semantic");
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
    stageSummary.innerHTML = `
      <span class="summary-chip parser-mode">${escapeHtml(parserModeLabel())}</span>
      <span class="summary-chip">Ready</span>
    `;
    return;
  }

  stageSummary.innerHTML = `
    <span class="summary-chip parser-mode">${escapeHtml(parserModeLabel())}</span>
    ${stages
    .map((stage) => `
      <span class="summary-chip ${escapeHtml(stage.status)}">
        ${escapeHtml(stage.name)} · ${escapeHtml(stage.status)}
      </span>
    `)
    .join("")}
  `;
}

function renderPanel(message) {
  if (message) {
    tabPanel.innerHTML = `<div class="empty-state">${escapeHtml(message)}</div>`;
    return;
  }

  if (!state.result) {
    tabPanel.innerHTML = `<div class="empty-state">Select a sample or edit source, then click Run.</div>`;
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
  if (state.activeTab === "run" && !stage && state.result.runOutput) {
    tabPanel.innerHTML = `<pre class="output-block">${escapeHtml(state.result.runOutput)}</pre>`;
    return;
  }
  if (!stage) {
    tabPanel.innerHTML = `<div class="empty-state">No output available.</div>`;
    return;
  }

  const output = state.activeTab === "run" && state.result.runOutput
    ? state.result.runOutput
    : [stage.stdout, stage.stderr].filter(Boolean).join("\n");
  tabPanel.innerHTML = output
    ? `<pre class="output-block">${escapeHtml(output)}</pre>`
    : `<div class="empty-state">${escapeHtml(stage.name)} produced no text output.</div>`;
}

function renderDiagnostics() {
  const diagnostics = state.result?.diagnostics || [];

  if (!diagnostics.length) {
    tabPanel.innerHTML = `<div class="empty-state">No diagnostics for this run.</div>`;
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
  return tabDefs.filter((tab) => {
    if (tab.id === "mips") {
      return isCodegenEnabled();
    }
    if (tab.id === "run") {
      return isAssemblyRunEnabled();
    }
    return true;
  });
}

function visiblePipelineDefs() {
  return pipelineDefs.filter((stage) => {
    if (stage.key === "mips") {
      return isCodegenEnabled();
    }
    if (stage.key === "run") {
      return isAssemblyRunEnabled();
    }
    return true;
  });
}

function isCodegenEnabled() {
  return state.selectedSample?.groupId === "mips";
}

function isAssemblyRunEnabled() {
  return isCodegenEnabled() && runAssemblyInput.checked;
}

function parserModeLabel() {
  return state.parserMode === "ll1" ? "Parser · LL(1)" : "Parser · Recursive";
}

function defaultAssemblyInputForSample(sample) {
  return sample?.id === "mips/read_add.snl" ? "7" : "";
}

function updateMipsOptionsVisibility() {
  const showMipsOptions = isCodegenEnabled();
  document.querySelectorAll(".mips-option").forEach((element) => {
    element.hidden = !showMipsOptions;
  });
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
