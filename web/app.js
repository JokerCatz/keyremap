const REPORT_SIZE = 64;
const PROTOCOL_MAJOR = 2;

const CMD = {
  GET_INFO: 0x01,
  GET_CONFIG_SUMMARY: 0x10,
  GET_BINDING: 0x11,
  SET_BINDING: 0x12,
  SAVE_CONFIG: 0x13,
  RESET_CONFIG: 0x14,
  GET_RAW_REPORTS: 0x20,
  GET_HID_INTERFACES: 0x21,
  GET_REPORT_DESCRIPTOR: 0x22,
  SET_ACTIVE_LAYER: 0x30,
  GET_LAYER_STATE: 0x31,
  GET_HOST_STATUS: 0x32,
  GET_EVENT_LOG: 0x34,
  SIMULATE_INPUT: 0x40,
  RELEASE_ALL: 0x42,
  REBOOT_BOOTSEL: 0x7f,
};

const IN = { KEY: 1, MOUSE_BUTTON: 2, REL_X: 3, REL_Y: 4, WHEEL: 5, CONSUMER: 6 };
const OUT = {
  NONE: 0, KEY: 1, MOUSE_BUTTON: 2, REL_X: 3, REL_Y: 4, WHEEL: 5,
  LAYER: 6, CONSUMER: 7, NEXT_LAYER: 8, LAYER_HOLD: 9, BLOCK: 10,
};

const LAYER_COUNT = 4;
const BINDING_COUNT = 48;
const LAYER_NAMES = ["Base", "Layer 1", "Layer 2", "Layer 3"];

// Physical keys of the JD-DZ.COM handle (see config/handle-layout.json):
// each one sends a different HID key in the handle's built-in Fn mode.
const PHYSICAL_KEYS = [
  ["k01", 0x1e, 0x22], ["k05", 0x1f, 0x23], ["k06", 0x20, 0x24], ["k04", 0x21, 0x25],
  ["k09", 0x29, 0x28], ["k02", 0x2b, 0x3a], ["k07", 0x05, 0x17], ["k08", 0x0a, 0x0b],
  ["k10", 0x10, 0x3b], ["k03", 0x1c, 0x11],
];
const PHYSICAL_BY_CODE = new Map();
for (const [id, base, fn] of PHYSICAL_KEYS) {
  PHYSICAL_BY_CODE.set(base, `${id}`);
  PHYSICAL_BY_CODE.set(fn, `${id} Fn`);
}
const HANDLE_KEYS = [
  0x1e, 0x1f, 0x20, 0x21, 0x29, 0x2b, 0x05, 0x0a, 0x10, 0x1c,
  0x22, 0x23, 0x24, 0x25, 0x28, 0x3a, 0x17, 0x0b, 0x3b, 0x11,
  0x14, 0x1a, 0x08, 0x15, 0x04, 0x16, 0x07, 0x09, 0x1d, 0x1b, 0x06, 0x19, 0x2c, 0xe0, 0xe1, 0xe2,
];

const KEY_NAMES = new Map([
  ...Array.from({ length: 26 }, (_, i) => [0x04 + i, String.fromCharCode(65 + i)]),
  [0x1e, "1"], [0x1f, "2"], [0x20, "3"], [0x21, "4"], [0x22, "5"],
  [0x23, "6"], [0x24, "7"], [0x25, "8"], [0x26, "9"], [0x27, "0"],
  [0x28, "Enter"], [0x29, "Esc"], [0x2a, "Backspace"], [0x2b, "Tab"], [0x2c, "Space"],
  [0x2d, "-"], [0x2e, "="], [0x2f, "["], [0x30, "]"], [0x31, "\\"],
  [0x33, ";"], [0x34, "'"], [0x35, "`"], [0x36, ","], [0x37, "."], [0x38, "/"],
  [0x39, "Caps Lock"],
  ...Array.from({ length: 12 }, (_, i) => [0x3a + i, `F${i + 1}`]),
  [0x46, "Print Screen"], [0x47, "Scroll Lock"], [0x48, "Pause"],
  [0x49, "Insert"], [0x4a, "Home"], [0x4b, "Page Up"], [0x4c, "Delete"], [0x4d, "End"], [0x4e, "Page Down"],
  [0x4f, "→"], [0x50, "←"], [0x51, "↓"], [0x52, "↑"],
  [0x53, "Num Lock"], [0x54, "KP /"], [0x55, "KP *"], [0x56, "KP -"], [0x57, "KP +"], [0x58, "KP Enter"],
  ...Array.from({ length: 9 }, (_, i) => [0x59 + i, `KP ${i + 1}`]),
  [0x62, "KP 0"], [0x63, "KP ."], [0x65, "Menu"],
  ...Array.from({ length: 12 }, (_, i) => [0x68 + i, `F${i + 13}`]),
  [0xe0, "Left Ctrl"], [0xe1, "Left Shift"], [0xe2, "Left Alt"], [0xe3, "Left Win"],
  [0xe4, "Right Ctrl"], [0xe5, "Right Shift"], [0xe6, "Right Alt"], [0xe7, "Right Win"],
]);

const CONSUMER_NAMES = new Map([
  [0xe9, "音量 +"], [0xea, "音量 -"], [0xe2, "靜音"], [0xcd, "播放/暫停"],
  [0xb5, "下一首"], [0xb6, "上一首"], [0xb7, "停止"], [0x6f, "亮度 +"], [0x70, "亮度 -"],
]);

const MOUSE_BUTTON_NAMES = ["", "左鍵", "右鍵", "中鍵", "側鍵 (上一頁)", "側鍵 (下一頁)", "按鍵 6", "按鍵 7", "按鍵 8"];

const els = Object.fromEntries(
  [
    "connect", "bootloader", "status", "active-layers", "edit-layers", "layer-rule",
    "last-input", "last-output", "last-layer", "event-log", "event-hide-motion", "event-pause", "event-clear",
    "raw-log", "raw-filter", "raw-hide-motion", "raw-pause", "raw-clear", "release-all", "key-sink",
    "save-config", "reset-config", "learn", "learn-hint", "add-input", "add-manual", "binding-list",
    "board", "firmware", "protocol", "refresh-host", "host-status", "host-vidpid", "host-line", "interfaces",
  ].map((id) => [id.replace(/-(\w)/g, (_, c) => c.toUpperCase()), document.getElementById(id)]),
);

const state = {
  device: null,
  sequence: 0,
  pending: new Map(),
  queue: Promise.resolve(),
  polling: false,
  rawAfter: 0,
  eventAfter: 0,
  rawPaused: false,
  eventPaused: false,
  activeLayer: 0,
  editLayer: 0,
  bindings: [],
  dirty: false,
  learning: false,
  tab: "monitor",
  interfaces: [],
};

// ---------------------------------------------------------------- naming

function keyName(code) {
  const name = KEY_NAMES.get(code) ?? `Key 0x${hex(code)}`;
  const physical = PHYSICAL_BY_CODE.get(code);
  return physical ? `${name} [${physical}]` : name;
}

function inputName(kind, code) {
  switch (kind) {
    case IN.KEY: return keyName(code);
    case IN.MOUSE_BUTTON: return `滑鼠 ${MOUSE_BUTTON_NAMES[code] ?? code}`;
    case IN.REL_X: return "搖桿 / 滑鼠 X";
    case IN.REL_Y: return "搖桿 / 滑鼠 Y";
    case IN.WHEEL: return "滾輪";
    case IN.CONSUMER: return `多媒體 ${CONSUMER_NAMES.get(code) ?? `0x${hex(code)}`}`;
    default: return `未知 ${kind}:${code}`;
  }
}

function outputName(kind, code) {
  switch (kind) {
    case OUT.NONE: return "（無輸出）";
    case OUT.KEY: return KEY_NAMES.get(code) ?? `Key 0x${hex(code)}`;
    case OUT.MOUSE_BUTTON: return `滑鼠 ${MOUSE_BUTTON_NAMES[code] ?? code}`;
    case OUT.REL_X: return "滑鼠 X";
    case OUT.REL_Y: return "滑鼠 Y";
    case OUT.WHEEL: return "滾輪";
    case OUT.CONSUMER: return CONSUMER_NAMES.get(code) ?? `多媒體 0x${hex(code)}`;
    case OUT.LAYER: return `切到 ${LAYER_NAMES[code] ?? code}（常駐）`;
    case OUT.LAYER_HOLD: return `按住時 ${LAYER_NAMES[code] ?? code}`;
    case OUT.NEXT_LAYER: return "切到下一個 Layer（常駐）";
    case OUT.BLOCK: return "停用";
    default: return `未知 ${kind}:${code}`;
  }
}

const DEFAULT_TAP_HOLD_MS = 500;
const MAX_TAP_HOLD_MS = 5000;

function isLayerOutput(kind) {
  return kind === OUT.LAYER || kind === OUT.NEXT_LAYER || kind === OUT.LAYER_HOLD;
}

function isMotion(kind) {
  return kind === IN.REL_X || kind === IN.REL_Y || kind === IN.WHEEL;
}

function hex(value, width = 2) {
  return value.toString(16).padStart(width, "0");
}

function setStatus(text) {
  els.status.textContent = text;
}

// ---------------------------------------------------------------- transport

function onInputReport(event) {
  const data = new Uint8Array(event.data.buffer);
  const pending = state.pending.get(data[1]);
  if (!pending) {
    return;
  }
  state.pending.delete(data[1]);
  pending({ command: data[0], status: data[3], payload: data.slice(4, 4 + data[2]) });
}

function sendRequest(command, payload) {
  state.sequence = (state.sequence % 255) + 1;
  const sequence = state.sequence;
  const report = new Uint8Array(REPORT_SIZE);
  report[0] = command;
  report[1] = sequence;
  report[2] = payload.length;
  report.set(payload, 4);

  return new Promise((resolve, reject) => {
    const timer = setTimeout(() => {
      state.pending.delete(sequence);
      reject(new Error("裝置沒有回應"));
    }, 1000);
    state.pending.set(sequence, (value) => {
      clearTimeout(timer);
      resolve(value);
    });
    state.device.sendReport(0, report).catch((error) => {
      clearTimeout(timer);
      state.pending.delete(sequence);
      reject(error);
    });
  });
}

// Requests are serialized so polling and edits never interleave.
function request(command, payload = []) {
  if (!state.device?.opened) {
    return Promise.reject(new Error("尚未連線"));
  }
  const run = state.queue.then(() => sendRequest(command, Uint8Array.from(payload)));
  state.queue = run.catch(() => {});
  return run.then((response) => {
    if (response.status !== 0) {
      throw new Error(`指令 0x${hex(command)} 失敗（status ${response.status}）`);
    }
    return response.payload;
  });
}

const u16 = (p, i) => p[i] | (p[i + 1] << 8);
const s16 = (p, i) => (u16(p, i) << 16) >> 16;
const u32 = (p, i) => (p[i] | (p[i + 1] << 8) | (p[i + 2] << 16) | (p[i + 3] << 24)) >>> 0;
const le32 = (v) => [v & 0xff, (v >> 8) & 0xff, (v >> 16) & 0xff, (v >>> 24) & 0xff];
const le16 = (v) => [v & 0xff, (v >> 8) & 0xff];

// ---------------------------------------------------------------- layers

function renderLayerButtons(container, onClick) {
  container.replaceChildren(
    ...LAYER_NAMES.map((name, layer) => {
      const button = document.createElement("button");
      button.type = "button";
      button.dataset.layer = layer;
      button.textContent = name;
      button.addEventListener("click", () => onClick(layer));
      return button;
    }),
  );
}

function markLayer(container, layer) {
  container.querySelectorAll("button").forEach((button) => {
    button.classList.toggle("active", Number(button.dataset.layer) === layer);
  });
}

function showActiveLayer(layer) {
  state.activeLayer = layer;
  markLayer(els.activeLayers, layer);
}

async function setActiveLayer(layer) {
  try {
    const p = await request(CMD.SET_ACTIVE_LAYER, [layer]);
    showActiveLayer(p[0]);
  } catch (error) {
    setStatus(error.message);
  }
}

// ---------------------------------------------------------------- monitor

const MAX_LOG_ROWS = 500;

// Newest rows go on top. While the user has scrolled down to read, the view is
// frozen and new rows wait in a buffer, so the list does not move under them.
const logBuffers = new Map();

function appendLog(container, row) {
  if (container.scrollTop > 4) {
    const buffer = logBuffers.get(container) ?? [];
    buffer.push(row);
    if (buffer.length > MAX_LOG_ROWS) buffer.shift();
    logBuffers.set(container, buffer);
    container.dataset.frozen = String(buffer.length);
    return;
  }
  container.prepend(row);
  while (container.childElementCount > MAX_LOG_ROWS) {
    container.lastElementChild.remove();
  }
}

function flushLog(container) {
  const buffer = logBuffers.get(container);
  if (!buffer?.length || container.scrollTop > 4) return;
  logBuffers.delete(container);
  delete container.dataset.frozen;
  for (const row of buffer) appendLog(container, row);
}

function timeStamp() {
  const now = new Date();
  return `${now.toLocaleTimeString("en-GB")}.${String(now.getMilliseconds()).padStart(3, "0")}`;
}

function logRow(...cells) {
  const row = document.createElement("div");
  row.className = "log-row";
  for (const [text, className] of cells) {
    const cell = document.createElement("span");
    cell.textContent = text;
    if (className) {
      cell.className = className;
    }
    row.append(cell);
  }
  return row;
}

function describeRaw(instance, bytes) {
  const itf = state.interfaces[instance];
  if (!itf?.parsed?.usesReportId) {
    return "";
  }
  const kind = itf.parsed.reportKinds.get(bytes[0]);
  return kind ? `ID ${bytes[0]} ${kind}` : `ID ${bytes[0]}`;
}

const lastRawReport = new Map();

// True when a report differs from the previous one with the same report ID
// only inside relative-axis fields (stick / wheel movement, no button change).
function isMotionOnlyReport(instance, bytes) {
  const parsed = state.interfaces[instance]?.parsed;
  if (!parsed) return false;
  const reportId = parsed.usesReportId ? bytes[0] : 0;
  const skip = parsed.usesReportId ? 8 : 0;
  const ranges = parsed.relativeBits.get(reportId);
  const key = `${instance}:${reportId}`;
  const previous = lastRawReport.get(key);
  lastRawReport.set(key, bytes);
  if (!ranges?.length || !previous || previous.length !== bytes.length) return false;

  for (let bit = skip; bit < bytes.length * 8; bit++) {
    if (ranges.some(([start, end]) => bit - skip >= start && bit - skip < end)) continue;
    const mask = 1 << (bit % 8);
    if ((bytes[bit >> 3] & mask) !== (previous[bit >> 3] & mask)) return false;
  }
  return true;
}

async function pollRaw() {
  const p = await request(CMD.GET_RAW_REPORTS, le32(state.rawAfter));
  let pos = 6;
  if (p[1] & 1 && !state.rawPaused) {
    appendLog(els.rawLog, logRow([timeStamp(), "muted"], ["…有封包來不及讀取而遺失", "warn"]));
  }
  for (let n = 0; n < p[0]; n++) {
    const seq = u32(p, pos);
    const instance = p[pos + 4] & 0x7f;
    const truncated = p[pos + 4] & 0x80;
    const len = p[pos + 5];
    const bytes = p.slice(pos + 6, pos + 6 + len);
    pos += 6 + len;
    state.rawAfter = seq;

    const filter = els.rawFilter.value;
    const motionOnly = isMotionOnlyReport(instance, bytes);
    if (state.rawPaused || (filter !== "all" && Number(filter) !== instance) || (motionOnly && els.rawHideMotion.checked)) {
      continue;
    }
    const hexText = Array.from(bytes, (b) => hex(b)).join(" ") + (truncated ? " …" : "");
    appendLog(els.rawLog, logRow(
      [timeStamp(), "muted"],
      [`介面 ${instance}`, "tag"],
      [describeRaw(instance, bytes), "muted"],
      [hexText, "mono"],
    ));
  }
  if (!p[0]) {
    state.rawAfter = Math.max(state.rawAfter, u32(p, 2));
  }
}

async function pollEvents() {
  const p = await request(CMD.GET_EVENT_LOG, le32(state.eventAfter));
  let pos = 6;
  for (let n = 0; n < p[0]; n++) {
    const event = {
      seq: u32(p, pos),
      inKind: p[pos + 4], inCode: p[pos + 5], inValue: s16(p, pos + 6),
      outKind: p[pos + 8], outCode: p[pos + 9], outValue: s16(p, pos + 10),
      layer: p[pos + 12] & 0x7f, simulated: Boolean(p[pos + 12] & 0x80),
    };
    pos += 13;
    state.eventAfter = event.seq;
    handleEvent(event);
  }
  if (!p[0]) {
    state.eventAfter = Math.max(state.eventAfter, u32(p, 2));
  }
}

function handleEvent(event) {
  const motion = isMotion(event.inKind);
  const inText = motion
    ? `${inputName(event.inKind, event.inCode)} ${event.inValue > 0 ? "+" : ""}${event.inValue}`
    : `${inputName(event.inKind, event.inCode)} ${event.inValue ? "按下" : "放開"}`;
  const outText = event.outKind === OUT.NONE ? "（無輸出）" : `${outputName(event.outKind, event.outCode)}${motion ? ` ${event.outValue}` : ""}`;

  if (state.learning && !event.simulated && (motion || event.inValue)) {
    finishLearn(event.inKind, motion ? 0 : event.inCode);
  }

  if (!motion || !els.eventHideMotion.checked) {
    els.lastInput.textContent = inText + (event.simulated ? "（模擬）" : "");
    els.lastOutput.textContent = outText;
    els.lastLayer.textContent = LAYER_NAMES[event.layer] ?? event.layer;
  }

  if (state.eventPaused || (motion && els.eventHideMotion.checked)) {
    return;
  }
  appendLog(els.eventLog, logRow(
    [timeStamp(), "muted"],
    [inText, event.simulated ? "muted" : ""],
    ["→", "muted"],
    [outText, event.outKind === OUT.BLOCK ? "warn" : ""],
    [LAYER_NAMES[event.layer] ?? String(event.layer), `tag layer-${event.layer}`],
  ));
}

async function pollLoop() {
  let tick = 0;
  while (state.polling && state.device?.opened) {
    try {
      if (state.tab === "monitor" || state.learning) {
        await pollEvents();
        if (state.tab === "monitor") {
          await pollRaw();
        }
      }
      if (tick++ % 10 === 0) {
        const p = await request(CMD.GET_LAYER_STATE);
        if (p[0] !== state.activeLayer) {
          showActiveLayer(p[0]);
        }
      }
    } catch (error) {
      setStatus(error.message);
    }
    await new Promise((resolve) => setTimeout(resolve, 25));
  }
}

async function simulate(kind, code, value) {
  await request(CMD.SIMULATE_INPUT, [kind, code, ...le16(value)]);
}

// ---------------------------------------------------------------- bindings

function bindingPayload(layer, slot, b) {
  return [layer, slot, b.inKind, b.inCode, b.outKind, b.outCode, ...le16(b.scale)];
}

async function loadBindings(layer) {
  const bindings = [];
  for (let slot = 0; slot < BINDING_COUNT; slot++) {
    const p = await request(CMD.GET_BINDING, [layer, slot]);
    bindings.push({ slot, inKind: p[2], inCode: p[3], outKind: p[4], outCode: p[5], scale: s16(p, 6) });
  }
  state.bindings = bindings;
  renderBindings();
}

async function writeBinding(binding) {
  await request(CMD.SET_BINDING, bindingPayload(state.editLayer, binding.slot, binding));
  state.dirty = true;
  updateDirty();
}

function updateDirty() {
  els.saveConfig.textContent = state.dirty ? "儲存到裝置 *" : "儲存到裝置";
  if (state.dirty) {
    setStatus("已套用到裝置，但還沒儲存；拔掉後會遺失");
  }
}

function outputOptions(inKind) {
  const groups = [];
  const motionIn = isMotion(inKind);
  groups.push(["", [
    [OUT.NONE, 0, state.editLayer === 0 ? "原樣輸出（預設）" : "沿用 Base（預設）"],
    [OUT.BLOCK, 0, "停用（不輸出）"],
  ]]);
  groups.push(["滑鼠", [
    [OUT.REL_X, 0, "滑鼠 X"], [OUT.REL_Y, 0, "滑鼠 Y"], [OUT.WHEEL, 0, "滾輪"],
    ...(motionIn ? [] : [1, 2, 3, 4, 5].map((n) => [OUT.MOUSE_BUTTON, n, `滑鼠 ${MOUSE_BUTTON_NAMES[n]}`])),
  ]]);
  if (!motionIn) {
    groups.push(["Layer", [
      ...LAYER_NAMES.map((name, i) => [OUT.LAYER_HOLD, i, `按住時切到 ${name}`]),
      ...LAYER_NAMES.map((name, i) => [OUT.LAYER, i, `切到 ${name}（常駐）`]),
      [OUT.NEXT_LAYER, 0, "切到下一個 Layer（常駐）"],
    ]]);
    groups.push(["多媒體", [...CONSUMER_NAMES].map(([code, name]) => [OUT.CONSUMER, code, name])]);
    groups.push(["鍵盤", [...KEY_NAMES].map(([code, name]) => [OUT.KEY, code, name])]);
  }
  return groups;
}

function makeOutputSelect(binding, onChange) {
  const select = document.createElement("select");
  for (const [label, options] of outputOptions(binding.inKind)) {
    const parent = label ? document.createElement("optgroup") : select;
    if (label) {
      parent.label = label;
      select.append(parent);
    }
    for (const [kind, code, name] of options) {
      const option = document.createElement("option");
      option.value = `${kind}:${code}`;
      option.textContent = name;
      parent.append(option);
    }
  }
  select.value = `${binding.outKind}:${binding.outCode}`;
  if (select.selectedIndex < 0) {
    const option = document.createElement("option");
    option.value = select.value = `${binding.outKind}:${binding.outCode}`;
    option.textContent = outputName(binding.outKind, binding.outCode);
    select.append(option);
    select.value = option.value;
  }
  select.addEventListener("change", () => {
    const [kind, code] = select.value.split(":").map(Number);
    onChange(kind, code);
  });
  return select;
}

function renderBindings() {
  const rows = state.bindings.filter((b) => b.inKind !== 0);
  if (!rows.length) {
    const empty = document.createElement("p");
    empty.className = "hint";
    empty.textContent = state.editLayer === 0
      ? "Base 沒有任何綁定：所有按鍵、滑鼠、搖桿都原樣輸出。"
      : `${LAYER_NAMES[state.editLayer]} 沒有任何綁定：全部沿用 Base。`;
    els.bindingList.replaceChildren(empty);
    return;
  }

  els.bindingList.replaceChildren(...rows.map((binding) => {
    const row = document.createElement("div");
    row.className = "binding-row";

    const input = document.createElement("div");
    input.className = "binding-input";
    input.textContent = inputName(binding.inKind, binding.inCode);

    const select = makeOutputSelect(binding, async (kind, code) => {
      // Layer bindings store their tap/long-press threshold (ms) in the scale field.
      if (isLayerOutput(kind) !== isLayerOutput(binding.outKind)) {
        binding.scale = isLayerOutput(kind) ? DEFAULT_TAP_HOLD_MS : 1000;
      }
      binding.outKind = kind;
      binding.outCode = code;
      await writeBinding(binding).catch((error) => setStatus(error.message));
      renderBindings();
    });

    const scale = document.createElement("label");
    scale.className = "scale";
    if (isLayerOutput(binding.outKind)) {
      const field = document.createElement("input");
      field.type = "number";
      field.min = "0";
      field.max = String(MAX_TAP_HOLD_MS);
      field.step = "100";
      field.value = Math.max(0, binding.scale);
      field.title = binding.outKind === OUT.LAYER_HOLD
        ? "按住多久才算切換 layer；在這之前放開＝送出這顆鍵原本的功能。0＝立即切換"
        : "長按多久才切換 layer；在這之前放開＝送出這顆鍵原本的功能。0＝按下立即切換";
      field.addEventListener("change", async () => {
        binding.scale = Math.max(0, Math.min(MAX_TAP_HOLD_MS, Math.round(Number(field.value) || 0)));
        field.value = binding.scale;
        await writeBinding(binding).catch((error) => setStatus(error.message));
      });
      scale.append(field, " ms");
      scale.title = field.title;
    } else if (isMotion(binding.inKind) || isMotion(binding.outKind)) {
      const field = document.createElement("input");
      field.type = "number";
      field.step = "10";
      field.value = binding.scale / 10;
      field.title = "倍率 %，負數代表反向";
      field.addEventListener("change", async () => {
        binding.scale = Math.max(-32000, Math.min(32000, Math.round(Number(field.value) * 10)));
        await writeBinding(binding).catch((error) => setStatus(error.message));
      });
      scale.append(field, " %");
    }

    const remove = document.createElement("button");
    remove.type = "button";
    remove.textContent = "移除";
    remove.addEventListener("click", async () => {
      Object.assign(binding, { inKind: 0, inCode: 0, outKind: 0, outCode: 0, scale: 0 });
      await writeBinding(binding).catch((error) => setStatus(error.message));
      renderBindings();
    });

    row.append(input, select, scale, remove);
    return row;
  }));
}

async function addBinding(inKind, inCode) {
  const existing = state.bindings.find((b) => b.inKind === inKind && b.inCode === inCode);
  if (existing) {
    setStatus(`${inputName(inKind, inCode)} 已經在清單中`);
    renderBindings();
    return;
  }
  const free = state.bindings.find((b) => b.inKind === 0);
  if (!free) {
    setStatus(`這個 layer 已經用滿 ${BINDING_COUNT} 個綁定`);
    return;
  }
  // A new row starts as an identity mapping so nothing changes until the user picks an output.
  const identity = { [IN.KEY]: OUT.KEY, [IN.MOUSE_BUTTON]: OUT.MOUSE_BUTTON, [IN.REL_X]: OUT.REL_X, [IN.REL_Y]: OUT.REL_Y, [IN.WHEEL]: OUT.WHEEL, [IN.CONSUMER]: OUT.CONSUMER };
  Object.assign(free, { inKind, inCode, outKind: identity[inKind], outCode: inCode, scale: 1000 });
  await writeBinding(free);
  renderBindings();
  setStatus(`已新增 ${inputName(inKind, inCode)}，請選擇輸出`);
}

function startLearn() {
  state.learning = true;
  els.learn.textContent = "取消";
  els.learnHint.hidden = false;
}

function stopLearn() {
  state.learning = false;
  els.learn.textContent = "按手把新增";
  els.learnHint.hidden = true;
}

function finishLearn(kind, code) {
  stopLearn();
  addBinding(kind, code).catch((error) => setStatus(error.message));
}

function renderManualInputs() {
  const options = [
    ...HANDLE_KEYS.map((code) => [IN.KEY, code]),
    [IN.MOUSE_BUTTON, 1], [IN.MOUSE_BUTTON, 2], [IN.MOUSE_BUTTON, 3], [IN.MOUSE_BUTTON, 4], [IN.MOUSE_BUTTON, 5],
    [IN.REL_X, 0], [IN.REL_Y, 0], [IN.WHEEL, 0],
  ];
  els.addInput.replaceChildren(...options.map(([kind, code]) => {
    const option = document.createElement("option");
    option.value = `${kind}:${code}`;
    option.textContent = inputName(kind, code);
    return option;
  }));
}

async function selectEditLayer(layer) {
  state.editLayer = layer;
  markLayer(els.editLayers, layer);
  els.layerRule.textContent = layer === 0
    ? "Base：沒有綁定的輸入會原樣送出。"
    : `${LAYER_NAMES[layer]}：沒有綁定的輸入會沿用 Base 的設定（所以搖桿和其他鍵照常可用）。`;
  if (state.device?.opened) {
    els.bindingList.replaceChildren(Object.assign(document.createElement("p"), { className: "hint", textContent: "讀取中…" }));
    await loadBindings(layer).catch((error) => setStatus(error.message));
  }
}

// ---------------------------------------------------------------- device tab

const HOST_STATUS = ["等待手把", "已連接目標手把", "已連接其他 HID 裝置", "手把已拔除", "接收錯誤", "USB 裝置已連接，但沒有 HID 介面"];
const LINE_STATE = ["SE0（未連接）", "Full-speed idle", "Low-speed idle", "SE1"];

// Minimal HID report descriptor decoder, for display only.
function parseDescriptor(bytes) {
  const lines = [];
  const reportKinds = new Map();
  const relativeBits = new Map();
  const bitOffsets = new Map();
  let reportSize = 0;
  let reportCount = 0;
  let usagePage = 0;
  let reportId = 0;
  let usesReportId = false;
  let indent = 0;
  const pageNames = { 0x01: "Generic Desktop", 0x07: "Keyboard", 0x08: "LED", 0x09: "Button", 0x0c: "Consumer" };
  const collectionUsage = { "1:2": "滑鼠", "1:6": "鍵盤", "1:128": "系統", "12:1": "多媒體" };
  let lastUsage = 0;

  for (let i = 0; i < bytes.length;) {
    const prefix = bytes[i++];
    let size = prefix & 3;
    if (size === 3) size = 4;
    let value = 0;
    for (let b = 0; b < size; b++) value |= bytes[i + b] << (8 * b);
    const raw = Array.from(bytes.slice(i - 1, i + size), (b) => hex(b)).join(" ");
    i += size;
    const type = (prefix >> 2) & 3;
    const tag = prefix >> 4;
    let text = `item ${hex(prefix)}`;

    if (type === 0) {
      const flags = value;
      const desc = `${flags & 1 ? "Const" : "Data"}, ${flags & 2 ? "Var" : "Array"}, ${flags & 4 ? "Rel" : "Abs"}`;
      if (tag === 0x8) {
        text = `Input (${desc})`;
        const start = bitOffsets.get(reportId) ?? 0;
        const end = start + reportSize * reportCount;
        bitOffsets.set(reportId, end);
        if ((flags & 7) === 6) {
          relativeBits.set(reportId, [...(relativeBits.get(reportId) ?? []), [start, end]]);
        }
      }
      else if (tag === 0x9) text = `Output (${desc})`;
      else if (tag === 0xb) text = `Feature (${desc})`;
      else if (tag === 0xa) {
        text = `Collection (${["Physical", "Application", "Logical"][value] ?? value})`;
        const kind = collectionUsage[`${usagePage}:${lastUsage}`];
        if (kind && reportId) reportKinds.set(reportId, kind);
      } else if (tag === 0xc) {
        indent = Math.max(0, indent - 1);
        text = "End Collection";
      }
    } else if (type === 1) {
      const names = { 0: "Usage Page", 1: "Logical Min", 2: "Logical Max", 7: "Report Size", 8: "Report ID", 9: "Report Count" };
      if (tag === 0) usagePage = value;
      if (tag === 7) reportSize = value;
      if (tag === 9) reportCount = value;
      if (tag === 8) {
        reportId = value;
        usesReportId = true;
        const kind = collectionUsage[`${usagePage}:${lastUsage}`];
        if (kind) reportKinds.set(reportId, kind);
      }
      const shown = tag === 0 ? `${pageNames[value] ?? "0x" + hex(value, 2)}` : String(tag === 1 && size === 1 ? (value << 24) >> 24 : value);
      text = `${names[tag] ?? `Global ${tag}`} (${shown})`;
    } else if (type === 2) {
      const names = { 0: "Usage", 1: "Usage Min", 2: "Usage Max" };
      if (tag === 0) lastUsage = value;
      text = `${names[tag] ?? `Local ${tag}`} (0x${hex(value)})`;
    }
    lines.push(`${raw.padEnd(15)} ${"  ".repeat(indent)}${text}`);
    if (type === 0 && tag === 0xa) indent++;
  }
  return { lines, usesReportId, reportKinds, relativeBits };
}

async function readDescriptor(instance, length) {
  const bytes = [];
  while (bytes.length < length) {
    const p = await request(CMD.GET_REPORT_DESCRIPTOR, [instance, ...le16(bytes.length)]);
    const chunk = p.slice(5);
    if (!chunk.length) break;
    bytes.push(...chunk);
  }
  return Uint8Array.from(bytes);
}

async function refreshHost() {
  const host = await request(CMD.GET_HOST_STATUS);
  els.hostStatus.textContent = HOST_STATUS[host[0]] ?? host[0];
  els.hostVidpid.textContent = host[6] || host[7] ? `${hex(u16(host, 6), 4)}:${hex(u16(host, 8), 4)}` : "-";
  els.hostLine.textContent = LINE_STATE[host[1]] ?? host[1];

  const p = await request(CMD.GET_HID_INTERFACES);
  const interfaces = [];
  for (let i = 0; i < p[0]; i++) {
    const e = p.slice(1 + i * 6, 7 + i * 6);
    const itf = { instance: i, mounted: e[0], itfProtocol: e[1], descLen: u16(e, 3), flags: e[5] };
    if (itf.mounted && itf.descLen) {
      itf.descriptor = await readDescriptor(i, itf.descLen);
      itf.parsed = parseDescriptor(itf.descriptor);
    }
    interfaces.push(itf);
  }
  state.interfaces = interfaces;
  renderInterfaces();
}

function renderInterfaces() {
  const caps = (flags) => [
    flags & 0x01 && "鍵盤鍵", flags & 0x02 && "滑鼠鍵", flags & 0x04 && "移動軸", flags & 0x08 && "多媒體鍵",
  ].filter(Boolean).join("、") || "（沒有可用欄位）";

  els.interfaces.replaceChildren(...state.interfaces.filter((itf) => itf.mounted).map((itf) => {
    const box = document.createElement("details");
    box.className = "interface";
    const summary = document.createElement("summary");
    const type = itf.flags & 0x80 ? "boot 模式" : "report 模式（依 descriptor 解讀）";
    summary.textContent = `介面 ${itf.instance}：${caps(itf.flags)} · ${type} · descriptor ${itf.descLen} bytes`;
    const pre = document.createElement("pre");
    pre.textContent = itf.parsed ? itf.parsed.lines.join("\n") : "（沒有 descriptor）";
    box.append(summary, pre);
    return box;
  }));
}

// ---------------------------------------------------------------- connection

async function connect() {
  if (!("hid" in navigator)) {
    setStatus("這個瀏覽器不支援 WebHID，請用 Chrome 或 Edge");
    return;
  }
  const [device] = await navigator.hid.requestDevice({
    filters: [{ vendorId: 0xcafe, productId: 0x4020, usagePage: 0xff00 }],
  });
  if (!device) {
    return;
  }

  state.device = device;
  device.addEventListener("inputreport", onInputReport);
  await device.open();

  const info = await request(CMD.GET_INFO);
  els.protocol.textContent = `${info[0]}.${info[1]}`;
  els.firmware.textContent = `${info[2]}.${info[3]}.${info[4]}`;
  els.board.textContent = new TextDecoder().decode(info.slice(7, info.indexOf(0, 7)));
  if (info[0] !== PROTOCOL_MAJOR) {
    setStatus(`韌體協定版本 ${info[0]}.${info[1]} 與網頁不相容，請先燒錄新韌體（make flash）`);
    els.bootloader.disabled = false;
    return;
  }

  for (const el of [els.bootloader, els.saveConfig, els.resetConfig, els.learn, els.addManual, els.refreshHost]) {
    el.disabled = false;
  }
  els.connect.textContent = "已連線";
  els.connect.disabled = true;
  showActiveLayer(info[5]);
  setStatus("已連線");

  // Skip anything that happened before connecting.
  state.rawAfter = u32(await request(CMD.GET_RAW_REPORTS, le32(0xffffffff)), 2);
  state.eventAfter = u32(await request(CMD.GET_EVENT_LOG, le32(0xffffffff)), 2);
  await refreshHost();
  await selectEditLayer(state.editLayer);

  state.polling = true;
  pollLoop();
}

function disconnected() {
  state.polling = false;
  state.device = null;
  stopLearn();
  els.connect.textContent = "連線";
  els.connect.disabled = false;
  for (const el of [els.bootloader, els.saveConfig, els.resetConfig, els.learn, els.addManual, els.refreshHost]) {
    el.disabled = true;
  }
  setStatus("裝置已斷線");
}

// ---------------------------------------------------------------- wiring

function showTab(tab) {
  state.tab = tab;
  document.querySelectorAll("[data-tab]").forEach((button) => button.classList.toggle("active", button.dataset.tab === tab));
  document.querySelectorAll(".tab-page").forEach((page) => { page.hidden = page.id !== `tab-${tab}`; });
  if (tab !== "remap") {
    stopLearn();
  }
}

document.querySelectorAll("[data-tab]").forEach((button) => button.addEventListener("click", () => showTab(button.dataset.tab)));

els.connect.addEventListener("click", () => connect().catch((error) => setStatus(error.message)));

els.bootloader.addEventListener("click", async () => {
  if (!confirm("讓 RP2040 重開進入 BOOTSEL（燒錄模式）？")) return;
  await request(CMD.REBOOT_BOOTSEL).catch(() => {});
  setStatus("已重開進 BOOTSEL，請執行 make flash");
});

els.saveConfig.addEventListener("click", async () => {
  try {
    await request(CMD.SAVE_CONFIG);
    state.dirty = false;
    updateDirty();
    setStatus("已儲存到裝置 flash");
  } catch (error) {
    setStatus(error.message);
  }
});

els.resetConfig.addEventListener("click", async () => {
  if (!confirm("清空所有 layer 的綁定並儲存？所有輸入會恢復成原樣輸出。")) return;
  try {
    await request(CMD.RESET_CONFIG);
    state.dirty = false;
    updateDirty();
    await loadBindings(state.editLayer);
    setStatus("已清空並儲存");
  } catch (error) {
    setStatus(error.message);
  }
});

els.learn.addEventListener("click", () => (state.learning ? stopLearn() : startLearn()));
els.addManual.addEventListener("click", () => {
  const [kind, code] = els.addInput.value.split(":").map(Number);
  addBinding(kind, code).catch((error) => setStatus(error.message));
});

els.refreshHost.addEventListener("click", () => refreshHost().catch((error) => setStatus(error.message)));

els.eventPause.addEventListener("click", () => {
  state.eventPaused = !state.eventPaused;
  els.eventPause.textContent = state.eventPaused ? "繼續" : "暫停";
});
els.rawPause.addEventListener("click", () => {
  state.rawPaused = !state.rawPaused;
  els.rawPause.textContent = state.rawPaused ? "繼續" : "暫停";
});
for (const log of [els.eventLog, els.rawLog]) {
  log.addEventListener("scroll", () => flushLog(log));
}
els.eventClear.addEventListener("click", () => {
  logBuffers.delete(els.eventLog);
  els.eventLog.replaceChildren();
});
els.rawClear.addEventListener("click", () => {
  logBuffers.delete(els.rawLog);
  els.rawLog.replaceChildren();
});

document.querySelectorAll("[data-sim]").forEach((button) => button.addEventListener("click", () => {
  const [kind, code, value] = button.dataset.sim.split(",").map(Number);
  simulate(kind, code, value).catch((error) => setStatus(error.message));
}));
document.querySelectorAll("[data-sim-tap]").forEach((button) => button.addEventListener("click", async () => {
  const [kind, code] = button.dataset.simTap.split(",").map(Number);
  if (kind === IN.KEY) els.keySink.focus();
  try {
    await simulate(kind, code, 1);
    await new Promise((resolve) => setTimeout(resolve, 60));
    await simulate(kind, code, 0);
  } catch (error) {
    setStatus(error.message);
  }
}));
els.releaseAll.addEventListener("click", () => request(CMD.RELEASE_ALL).catch((error) => setStatus(error.message)));

if ("hid" in navigator) {
  navigator.hid.addEventListener("disconnect", (event) => {
    if (event.device === state.device) disconnected();
  });
}

renderLayerButtons(els.activeLayers, setActiveLayer);
renderLayerButtons(els.editLayers, selectEditLayer);
markLayer(els.editLayers, 0);
renderManualInputs();
selectEditLayer(0);
