const REPORT_SIZE = 64;
const CMD_GET_INFO = 0x01;
const CMD_GET_CONFIG_SUMMARY = 0x10;
const CMD_GET_BINDING = 0x11;
const CMD_SET_BINDING = 0x12;
const CMD_SAVE_CONFIG = 0x13;
const CMD_RESET_CONFIG = 0x14;
const CMD_SET_ACTIVE_LAYER = 0x30;
const CMD_GET_INPUT_EVENT = 0x33;
const CMD_SIMULATE_INPUT = 0x40;
const CMD_GET_OUTPUT_STATE = 0x41;
const CMD_RELEASE_ALL = 0x42;
const CMD_REBOOT_BOOTSEL = 0x7f;

const physicalKeys = [
  { id: "k01", label: "1", base: "KEY_1", baseCode: 0x1e, fn: "KEY_5", fnCode: 0x22 },
  { id: "k05", label: "2", base: "KEY_2", baseCode: 0x1f, fn: "KEY_6", fnCode: 0x23 },
  { id: "k06", label: "3", base: "KEY_3", baseCode: 0x20, fn: "KEY_7", fnCode: 0x24 },
  { id: "k04", label: "4", base: "KEY_4", baseCode: 0x21, fn: "KEY_8", fnCode: 0x25 },
  { id: "k09", label: "Esc", base: "KEY_ESC", baseCode: 0x29, fn: "KEY_ENTER", fnCode: 0x28 },
  { id: "k02", label: "Tab", base: "KEY_TAB", baseCode: 0x2b, fn: "KEY_F1", fnCode: 0x3a },
  { id: "k07", label: "B", base: "KEY_B", baseCode: 0x05, fn: "KEY_T", fnCode: 0x17 },
  { id: "k08", label: "G", base: "KEY_G", baseCode: 0x0a, fn: "KEY_H", fnCode: 0x0b },
  { id: "k10", label: "M", base: "KEY_M", baseCode: 0x10, fn: "KEY_F2", fnCode: 0x3b },
  { id: "k03", label: "Y", base: "KEY_Y", baseCode: 0x1c, fn: "KEY_N", fnCode: 0x11 },
];

const baseOnlyKeys = [
  { label: "Q", code: 0x14 },
  { label: "W", code: 0x1a },
  { label: "E", code: 0x08 },
  { label: "R", code: 0x15 },
  { label: "A", code: 0x04 },
  { label: "S", code: 0x16 },
  { label: "D", code: 0x07 },
  { label: "F", code: 0x09 },
  { label: "Z", code: 0x1d },
  { label: "X", code: 0x1b },
  { label: "C", code: 0x06 },
  { label: "V", code: 0x19 },
  { label: "Space", code: 0x2c },
  { label: "Ctrl", code: 0xe0 },
  { label: "Shift", code: 0xe1 },
  { label: "Alt", code: 0xe2 },
];

const mouseInputs = [
  { label: "Mouse 1", kind: 2, code: 1 },
  { label: "Mouse 2", kind: 2, code: 2 },
  { label: "Mouse 3", kind: 2, code: 3 },
  { label: "Mouse 4", kind: 2, code: 4 },
  { label: "Move X", kind: 3, code: 0 },
  { label: "Move Y", kind: 4, code: 0 },
  { label: "Wheel", kind: 5, code: 0 },
];

const editorInputs = [
  ...physicalKeys.map((key, index) => ({ slot: index, label: `${key.id} ${key.label} Base`, kind: 1, code: key.baseCode })),
  ...physicalKeys.map((key, index) => ({ slot: physicalKeys.length + index, label: `${key.id} ${key.label} Fn`, kind: 1, code: key.fnCode })),
  ...baseOnlyKeys.map((key, index) => ({ slot: physicalKeys.length * 2 + index, label: key.label, kind: 1, code: key.code })),
  ...mouseInputs.map((input, index) => ({ slot: physicalKeys.length * 2 + baseOnlyKeys.length + index, ...input })),
];

const keyboardOutputs = [
  ["A", 0x04], ["B", 0x05], ["C", 0x06], ["D", 0x07], ["E", 0x08], ["F", 0x09],
  ["G", 0x0a], ["H", 0x0b], ["I", 0x0c], ["J", 0x0d], ["K", 0x0e], ["L", 0x0f],
  ["M", 0x10], ["N", 0x11], ["O", 0x12], ["P", 0x13], ["Q", 0x14], ["R", 0x15],
  ["S", 0x16], ["T", 0x17], ["U", 0x18], ["V", 0x19], ["W", 0x1a], ["X", 0x1b],
  ["Y", 0x1c], ["Z", 0x1d],
  ["1", 0x1e], ["2", 0x1f], ["3", 0x20], ["4", 0x21], ["5", 0x22],
  ["6", 0x23], ["7", 0x24], ["8", 0x25], ["9", 0x26], ["0", 0x27],
  ["Enter", 0x28], ["Esc", 0x29], ["Backspace", 0x2a], ["Tab", 0x2b], ["Space", 0x2c],
  ["-", 0x2d], ["=", 0x2e], ["[", 0x2f], ["]", 0x30], ["\\\\", 0x31],
  [";", 0x33], ["'", 0x34], ["`", 0x35], [",", 0x36], [".", 0x37], ["/", 0x38],
  ["Caps Lock", 0x39],
  ["F1", 0x3a], ["F2", 0x3b], ["F3", 0x3c], ["F4", 0x3d], ["F5", 0x3e], ["F6", 0x3f],
  ["F7", 0x40], ["F8", 0x41], ["F9", 0x42], ["F10", 0x43], ["F11", 0x44], ["F12", 0x45],
  ["Print Screen", 0x46], ["Scroll Lock", 0x47], ["Pause", 0x48],
  ["Insert", 0x49], ["Home", 0x4a], ["Page Up", 0x4b], ["Delete", 0x4c], ["End", 0x4d], ["Page Down", 0x4e],
  ["Right", 0x4f], ["Left", 0x50], ["Down", 0x51], ["Up", 0x52],
  ["Num Lock", 0x53], ["KP /", 0x54], ["KP *", 0x55], ["KP -", 0x56], ["KP +", 0x57],
  ["KP Enter", 0x58], ["KP 1", 0x59], ["KP 2", 0x5a], ["KP 3", 0x5b], ["KP 4", 0x5c],
  ["KP 5", 0x5d], ["KP 6", 0x5e], ["KP 7", 0x5f], ["KP 8", 0x60], ["KP 9", 0x61],
  ["KP 0", 0x62], ["KP .", 0x63],
  ["Application", 0x65], ["Power", 0x66],
  ["KP =", 0x67],
  ["F13", 0x68], ["F14", 0x69], ["F15", 0x6a], ["F16", 0x6b], ["F17", 0x6c],
  ["F18", 0x6d], ["F19", 0x6e], ["F20", 0x6f], ["F21", 0x70], ["F22", 0x71],
  ["F23", 0x72], ["F24", 0x73],
  ["Left Ctrl", 0xe0], ["Left Shift", 0xe1], ["Left Alt", 0xe2], ["Left GUI", 0xe3],
  ["Right Ctrl", 0xe4], ["Right Shift", 0xe5], ["Right Alt", 0xe6], ["Right GUI", 0xe7],
];

const keyOutputs = [
  ["None", 0, 0],
  ...keyboardOutputs.map(([label, code]) => [label, 1, code]),
  ["Mouse 1", 2, 1],
  ["Mouse 2", 2, 2],
  ["Mouse 3", 2, 3],
  ["Mouse 4", 2, 4],
  ["Move X", 3, 0],
  ["Move Y", 4, 0],
  ["Wheel", 5, 0],
  ["Volume Up", 7, 0xe9],
  ["Volume Down", 7, 0xea],
  ["Mute", 7, 0xe2],
  ["Play/Pause", 7, 0xcd],
  ["Next Track", 7, 0xb5],
  ["Prev Track", 7, 0xb6],
  ["Layer 0 Base", 6, 0],
  ["Layer 1 Nav", 6, 1],
  ["Layer 2 Media", 6, 2],
  ["Layer 3 Game", 6, 3],
  ["Next Layer", 8, 0],
];

const state = {
  device: null,
  sequence: 1,
  pending: new Map(),
  inputPoll: null,
  lastInputCount: 0,
};

const keyboardNames = new Map(keyboardOutputs.map(([label, code]) => [code, label]));

const els = {
  connect: document.querySelector("#connect"),
  bootloader: document.querySelector("#bootloader"),
  status: document.querySelector("#status"),
  board: document.querySelector("#board"),
  firmware: document.querySelector("#firmware"),
  protocol: document.querySelector("#protocol"),
  layer: document.querySelector("#layer"),
  profiles: document.querySelector("#profiles"),
  bindings: document.querySelector("#bindings"),
  debug: document.querySelector("#debug"),
  keyGrid: document.querySelector("#key-grid"),
  baseKeyGrid: document.querySelector("#base-key-grid"),
  keySink: document.querySelector("#key-sink"),
  releaseAll: document.querySelector("#release-all"),
  mappingTable: document.querySelector("#mapping-table"),
  saveConfig: document.querySelector("#save-config"),
  resetConfig: document.querySelector("#reset-config"),
  inputEvent: document.querySelector("#input-event"),
  inputName: document.querySelector("#input-name"),
  inputRaw: document.querySelector("#input-raw"),
};

function setStatus(text) {
  els.status.textContent = text;
}

function nextSequence() {
  state.sequence = (state.sequence + 1) & 0xff;
  if (state.sequence === 0) {
    state.sequence = 1;
  }
  return state.sequence;
}

function parseCString(bytes) {
  const end = bytes.indexOf(0);
  return new TextDecoder().decode(bytes.slice(0, end >= 0 ? end : bytes.length));
}

function onInputReport(event) {
  const data = new Uint8Array(event.data.buffer);
  const command = data[0];
  const sequence = data[1];
  const length = data[2];
  const status = data[3];
  const payload = data.slice(4, 4 + length);
  const pending = state.pending.get(sequence);

  if (!pending) {
    return;
  }

  state.pending.delete(sequence);
  pending.resolve({ command, sequence, status, payload });
}

async function request(command, payload = new Uint8Array()) {
  if (!state.device?.opened) {
    throw new Error("Device is not open");
  }

  const sequence = nextSequence();
  const report = new Uint8Array(REPORT_SIZE);
  report[0] = command;
  report[1] = sequence;
  report[2] = payload.length;
  report.set(payload, 4);

  const response = new Promise((resolve, reject) => {
    const timer = setTimeout(() => {
      state.pending.delete(sequence);
      reject(new Error("Timed out waiting for device"));
    }, 1000);

    state.pending.set(sequence, {
      resolve: (value) => {
        clearTimeout(timer);
        resolve(value);
      },
    });
  });

  await state.device.sendReport(0, report);
  return response;
}

async function refreshInfo() {
  const response = await request(CMD_GET_INFO);
  if (response.status !== 0) {
    throw new Error(`Get info failed: ${response.status}`);
  }

  const p = response.payload;
  els.protocol.textContent = `${p[0]}.${p[1]}`;
  els.firmware.textContent = `${p[2]}.${p[3]}.${p[4]}`;
  els.layer.textContent = String(p[5]);
  els.board.textContent = parseCString(p.slice(7));

  await refreshConfig();
}

function setActiveLayerButton(layer) {
  document.querySelectorAll("[data-layer]").forEach((button) => {
    button.classList.toggle("active", Number(button.dataset.layer) === layer);
  });
}

async function refreshConfig() {
  const response = await request(CMD_GET_CONFIG_SUMMARY);
  if (response.status !== 0) {
    throw new Error(`Get config failed: ${response.status}`);
  }

  const p = response.payload;
  els.profiles.textContent = `${p[0]} profile, ${p[1]} layers`;
  els.bindings.textContent = `${p[2]} slots/layer`;
  els.layer.textContent = String(p[4]);
  setActiveLayerButton(p[4]);
  await loadBindings(p[4]);
}

async function getBinding(layer, slot) {
  const response = await request(CMD_GET_BINDING, new Uint8Array([layer, slot]));
  if (response.status !== 0) {
    throw new Error(`Get binding failed: ${response.status}`);
  }
  const p = response.payload;
  return {
    layer: p[0],
    slot: p[1],
    inputKind: p[2],
    inputCode: p[3],
    outputKind: p[4],
    outputCode: p[5],
    scale: new DataView(p.buffer, p.byteOffset, p.byteLength).getInt16(6, true),
  };
}

async function setBinding(layer, slot, inputKind, inputCode, outputKind, outputCode, scale = 1000) {
  const [scaleLo, scaleHi] = int16Bytes(scale);
  const response = await request(CMD_SET_BINDING, new Uint8Array([
    layer,
    slot,
    inputKind,
    inputCode,
    outputKind,
    outputCode,
    scaleLo,
    scaleHi,
  ]));
  if (response.status !== 0) {
    throw new Error(`Set binding failed: ${response.status}`);
  }
}

function outputValue(kind, code) {
  return `${kind}:${code}`;
}

function makeOutputSelect(row, binding, layer) {
  const select = document.createElement("select");
  keyOutputs.forEach(([label, kind, code]) => {
    const option = document.createElement("option");
    option.value = outputValue(kind, code);
    option.textContent = label;
    select.append(option);
  });
  select.value = outputValue(binding.outputKind, binding.outputCode);
  if (!select.value) {
    select.value = "0:0";
  }
  select.addEventListener("change", async () => {
    const [outputKind, outputCode] = select.value.split(":").map(Number);
    try {
      await setBinding(layer, row.slot, row.kind, row.code, outputKind, outputCode);
      setStatus("Changed; press Save to persist");
    } catch (error) {
      setStatus(error.message);
    }
  });
  return select;
}

async function loadBindings(layer) {
  const header = document.createElement("div");
  header.className = "mapping-row header";
  header.innerHTML = "<div>Input</div><div>Source</div><div>Output</div>";
  const rows = [header];

  for (const row of editorInputs) {
    const binding = await getBinding(layer, row.slot);
    const line = document.createElement("div");
    line.className = "mapping-row";
    const input = document.createElement("div");
    input.textContent = row.label;
    const source = document.createElement("div");
    source.textContent = `${row.kind}:${row.code}`;
    line.append(input, source, makeOutputSelect(row, binding, layer));
    rows.push(line);
  }

  els.mappingTable.replaceChildren(...rows);
}

function int16Bytes(value) {
  const normalized = value < 0 ? 0x10000 + value : value;
  return [normalized & 0xff, (normalized >> 8) & 0xff];
}

async function simulate(kind, code, value) {
  const payload = new Uint8Array([kind, code, ...int16Bytes(value)]);
  const response = await request(CMD_SIMULATE_INPUT, payload);
  if (response.status !== 0) {
    throw new Error(`Simulate failed: ${response.status}`);
  }
  await refreshOutput();
}

async function tapKey(code) {
  await simulate(1, code, 1);
  await new Promise((resolve) => setTimeout(resolve, 80));
  await simulate(1, code, 0);
}

async function tapMouse(button) {
  await simulate(2, button, 1);
  await new Promise((resolve) => setTimeout(resolve, 180));
  await simulate(2, button, 0);
  await new Promise((resolve) => setTimeout(resolve, 80));
  await releaseAll();
}

async function releaseAll() {
  const response = await request(CMD_RELEASE_ALL);
  if (response.status !== 0) {
    throw new Error(`Release failed: ${response.status}`);
  }
  await refreshOutput();
}

async function refreshOutput() {
  const response = await request(CMD_GET_OUTPUT_STATE);
  if (response.status !== 0) {
    throw new Error(`Get output failed: ${response.status}`);
  }

  const p = response.payload;
  const value = new DataView(p.buffer, p.byteOffset, p.byteLength).getInt16(2, true);
  const count = new DataView(p.buffer, p.byteOffset, p.byteLength).getUint32(4, true);
  els.debug.textContent = JSON.stringify({
    kind: p[0],
    code: p[1],
    value,
    count,
  }, null, 2);
}

function inputKindName(kind) {
  switch (kind) {
    case 1: return "Key";
    case 2: return "Mouse button";
    case 3: return "Move X";
    case 4: return "Move Y";
    case 5: return "Wheel";
    default: return "None";
  }
}

function inputName(kind, code) {
  if (kind === 1) {
    return keyboardNames.get(code) ?? `HID 0x${code.toString(16).padStart(2, "0")}`;
  }
  if (kind === 2) {
    return `Mouse ${code}`;
  }
  if (kind === 3) {
    return "Relative X";
  }
  if (kind === 4) {
    return "Relative Y";
  }
  if (kind === 5) {
    return "Wheel";
  }
  return "-";
}

async function refreshInputEvent() {
  const response = await request(CMD_GET_INPUT_EVENT);
  if (response.status !== 0) {
    throw new Error(`Get input failed: ${response.status}`);
  }

  const p = response.payload;
  const view = new DataView(p.buffer, p.byteOffset, p.byteLength);
  const value = view.getInt16(2, true);
  const count = view.getUint32(4, true);
  if (count === state.lastInputCount) {
    return;
  }

  state.lastInputCount = count;
  els.inputEvent.textContent = `${inputKindName(p[0])} ${value ? "down/move" : "up"}`;
  els.inputName.textContent = inputName(p[0], p[1]);
  els.inputRaw.textContent = `kind=${p[0]} code=${p[1]} value=${value} count=${count}`;
}

function startInputPolling() {
  if (state.inputPoll) {
    clearInterval(state.inputPoll);
  }

  state.inputPoll = setInterval(() => {
    if (!state.device?.opened || state.pending.size > 2) {
      return;
    }
    refreshInputEvent().catch((error) => setStatus(error.message));
  }, 120);
}

async function connect() {
  if (!("hid" in navigator)) {
    setStatus("WebHID is not available in this browser");
    return;
  }

  const [device] = await navigator.hid.requestDevice({
    filters: [{ vendorId: 0xcafe, productId: 0x4020, usagePage: 0xff00 }],
  });

  if (!device) {
    return;
  }

  state.device = device;
  state.device.addEventListener("inputreport", onInputReport);
  await state.device.open();
  els.bootloader.disabled = false;
  els.saveConfig.disabled = false;
  els.resetConfig.disabled = false;
  setStatus("Connected");
  await refreshInfo();
  startInputPolling();
}

els.connect.addEventListener("click", () => {
  connect().catch((error) => setStatus(error.message));
});

els.bootloader.addEventListener("click", async () => {
  try {
    await request(CMD_REBOOT_BOOTSEL);
    setStatus("Rebooting to BOOTSEL");
  } catch (error) {
    setStatus(error.message);
  }
});

els.saveConfig.addEventListener("click", async () => {
  try {
    const response = await request(CMD_SAVE_CONFIG);
    if (response.status !== 0) {
      throw new Error(`Save failed: ${response.status}`);
    }
    setStatus("Saved to device");
  } catch (error) {
    setStatus(error.message);
  }
});

els.resetConfig.addEventListener("click", async () => {
  try {
    const response = await request(CMD_RESET_CONFIG);
    if (response.status !== 0) {
      throw new Error(`Reset failed: ${response.status}`);
    }
    setStatus("Reset defaults");
    await refreshConfig();
  } catch (error) {
    setStatus(error.message);
  }
});

function renderPhysicalKeys() {
  const makeKeyButton = (key, showFn) => {
    const button = document.createElement("button");
    button.type = "button";
    button.dataset.keyCode = String(key.code);
    button.innerHTML = showFn ? key.html : `${key.label}<span>0x${key.code.toString(16)}</span>`;
    button.addEventListener("click", async () => {
      try {
        els.keySink.focus();
        await tapKey(key.code);
      } catch (error) {
        setStatus(error.message);
      }
    });
    return button;
  };

  els.keyGrid.replaceChildren(...physicalKeys.flatMap((key) => [
    makeKeyButton({ code: key.baseCode, html: `${key.id}: Base<span>${key.base}</span>` }, true),
    makeKeyButton({ code: key.fnCode, html: `${key.id}: Fn<span>${key.fn}</span>` }, true),
  ]));
  els.baseKeyGrid.replaceChildren(...baseOnlyKeys.map((key) => makeKeyButton(key, false)));
}

renderPhysicalKeys();

document.querySelectorAll("[data-layer]").forEach((button) => {
  button.addEventListener("click", async () => {
    const layer = Number(button.dataset.layer);
    try {
      const response = await request(CMD_SET_ACTIVE_LAYER, new Uint8Array([layer]));
      if (response.status !== 0) {
        throw new Error(`Set layer failed: ${response.status}`);
      }
      await refreshConfig();
    } catch (error) {
      setStatus(error.message);
    }
  });
});

document.querySelectorAll("[data-sim]").forEach((button) => {
  button.addEventListener("click", async () => {
    const [kind, code, value] = button.dataset.sim.split(",").map((item) => Number(item));
    try {
      await simulate(kind, code, value);
    } catch (error) {
      setStatus(error.message);
    }
  });
});

document.querySelectorAll("[data-mouse-tap]").forEach((button) => {
  button.addEventListener("click", async () => {
    try {
      await tapMouse(Number(button.dataset.mouseTap));
    } catch (error) {
      setStatus(error.message);
    }
  });
});

els.releaseAll.addEventListener("click", async () => {
  try {
    await releaseAll();
  } catch (error) {
    setStatus(error.message);
  }
});
