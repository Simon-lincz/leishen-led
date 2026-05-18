const state = {
  mode: "static",
  color: "#b200ff",
  brightness: 70,
  time: 0,
};

const modeMap = {
  off: 0,
  static: 1,
  breathing: 2,
  flow: 3,
  flash: 4,
  successively: 5,
  marquee: 6,
  scanning: 7,
  meteor: 8,
};

const modeName = Object.fromEntries(Object.entries(modeMap).map(([name, value]) => [value, name]));
modeName[3] = "flow";

const preview = document.querySelector("#preview");
const modeInput = document.querySelector("#mode");
const colorInput = document.querySelector("#color");
const brightnessInput = document.querySelector("#brightness");
const timeInput = document.querySelector("#time");
const brightnessValue = document.querySelector("#brightnessValue");
const timeValue = document.querySelector("#timeValue");
const summary = document.querySelector("#summary");
const statusText = document.querySelector("#statusText");
const buttons = [...document.querySelectorAll("button")];

function hexToRgb(hex) {
  const value = Number.parseInt(hex.slice(1), 16);
  return [(value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff];
}

function rgbToHex([r, g, b]) {
  return `#${[r, g, b].map((value) => value.toString(16).padStart(2, "0")).join("")}`;
}

function setBusy(busy) {
  buttons.forEach((button) => {
    button.disabled = busy;
  });
}

function payload() {
  return {
    mode: state.mode,
    brightness: state.brightness,
    color: hexToRgb(state.color),
    time: state.time,
  };
}

function setState(next) {
  Object.assign(state, next);
  render();
}

function render() {
  const [r, g, b] = hexToRgb(state.color);
  const duration = Math.max(state.time, 120);

  document.documentElement.style.setProperty("--led", `${r}, ${g}, ${b}`);
  document.documentElement.style.setProperty("--brightness", String(state.brightness / 100));
  document.documentElement.style.setProperty("--duration", `${duration}ms`);

  preview.dataset.mode = state.mode;
  preview.dataset.timeActive = String(state.mode === "static" && state.time > 0);

  modeInput.value = state.mode;
  colorInput.value = state.color;
  brightnessInput.value = state.brightness;
  timeInput.value = state.time;
  brightnessValue.value = state.brightness;
  timeValue.value = state.time;

  summary.textContent = `${state.mode} · RGB ${r} ${g} ${b} · ${state.brightness}% · time ${state.time}`;
  statusText.textContent =
    `EC 0x95=${modeMap[state.mode]} 0x98=${state.brightness}\n` +
    `EC 0x9A=${r} 0x9B=${g} 0x9C=${b} 0x9D=${state.time >> 8} 0x9E=${state.time & 0xff}`;
}

async function requestJson(path, options = {}) {
  const response = await fetch(path, {
    headers: { "Content-Type": "application/json" },
    ...options,
  });
  const text = await response.text();
  const data = text ? JSON.parse(text) : {};

  if (!response.ok) {
    throw new Error(data.error || `HTTP ${response.status}`);
  }
  return data;
}

function absorbServerState(data) {
  setState({
    mode: data.modeName || modeName[data.mode] || "static",
    brightness: Number(data.brightness),
    color: rgbToHex(data.color || [178, 0, 255]),
    time: Number(data.time),
  });
}

async function loadStatus() {
  try {
    const data = await requestJson("/api/status");
    absorbServerState(data);
    statusText.textContent += "\n\n服务已连接，状态来自 EC。";
  } catch (error) {
    statusText.textContent = `服务未连接或 EC 读取失败：${error.message}`;
    render();
  }
}

async function applyState(next = {}) {
  setBusy(true);
  try {
    setState(next);
    const data = await requestJson("/api/apply", {
      method: "POST",
      body: JSON.stringify(payload()),
    });
    absorbServerState(data);
    statusText.textContent += "\n\n已写入 EC，并保存为开机恢复状态。";
  } catch (error) {
    statusText.textContent = `写入失败：${error.message}`;
  } finally {
    setBusy(false);
  }
}

modeInput.addEventListener("change", () => setState({ mode: modeInput.value }));
colorInput.addEventListener("input", () => setState({ color: colorInput.value }));
brightnessInput.addEventListener("input", () => setState({ brightness: Number(brightnessInput.value) }));
timeInput.addEventListener("input", () => setState({ time: Number(timeInput.value) }));

document.querySelector("#apply").addEventListener("click", () => applyState());
document.querySelector("#staticPurple").addEventListener("click", () => {
  applyState({ mode: "static", color: "#b200ff", brightness: 70, time: 0 });
});
document.querySelector("#flowPreset").addEventListener("click", () => {
  applyState({ mode: "flow", color: "#ff0000", brightness: 70, time: 1000 });
});
document.querySelector("#offPreset").addEventListener("click", async () => {
  setBusy(true);
  try {
    const data = await requestJson("/api/off", { method: "POST" });
    absorbServerState(data);
    statusText.textContent += "\n\n已关灯，并保存为开机恢复状态。";
  } catch (error) {
    statusText.textContent = `关灯失败：${error.message}`;
  } finally {
    setBusy(false);
  }
});

render();
loadStatus();
