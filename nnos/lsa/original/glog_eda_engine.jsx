// -*- coding: utf-8 -*-
/**
 * GLOG EDA ENGINE - Production Dashboard
 * Author: Jason M Jarmacz | Evolution Strategist
 * Co-Author: Claude by Anthropic
 * Human/AI Innovative Collaboration
 *
 * API Target: https://glogalhosts.com
 * OpenAPI: 3.1.0 | Multimodal Agent Builder v0.1.0
 *
 * NeuroDivergent AI Evolution - Omega State Equation:
 *
 *   Omega_ActualState =
 *     ( alpha * (grounding_score + reference_integrity) * omega * (threat_load / sqrt_risk) )
 *     / ( iota * (reference_integrity + grounding_score) + iota * (intervention_bandwidth * env_delta) )
 *
 *   Parameters:
 *     grounding_score        = sleep + routine + environment_stability
 *     threat_load            = conflict + uncertainty + time_pressure
 *     reference_integrity    = truth_signals + trusted_inputs
 *     intervention_bandwidth = tools * allies * protocols
 *
 * Quantum Migration Threshold:
 *   X + Y > Z => Immediate Migration Required
 *   X = Data confidentiality shelf-life (years)
 *   Y = System migration duration (years)
 *   Z = Threat horizon to Q-Day (years <= 10)
 *
 * EDA Primitive Chain (Knockout Sequence):
 *   P001 LOAD -> P010 INSPECT_SHAPE -> P011 INSPECT_TYPES -> P014 INSPECT_MISSING
 *   -> P040 AGG_MEAN -> P042 AGG_SD -> P049 AGG_IQR -> P093 DETECT_OUTLIER
 *   -> P050 GROUP_AGG -> P052 CROSS_TAB -> P047 AGG_QUANTILE -> G015 EMIT_ARTIFACT
 */

import { useState, useEffect, useCallback, useRef } from "react";

// ─── CONFIGURATION ────────────────────────────────────────────────────────────

const API_BASE = "https://glogalhosts.com";

const OMEGA_DEFAULTS = {
  grounding_score: 0.82,
  threat_load: 0.31,
  reference_integrity: 0.91,
  intervention_bandwidth: 0.75,
  env_delta: 0.12,
  sqrt_risk: 0.55,
};

const EDA_CHAIN = [
  { id: "P001", label: "LOAD",            desc: "Fetch live agent corpus from API" },
  { id: "P010", label: "INSPECT_SHAPE",   desc: "Cardinality: agents x fields x depth" },
  { id: "P011", label: "INSPECT_TYPES",   desc: "Field type classification per agent" },
  { id: "P014", label: "INSPECT_MISSING", desc: "Null / absent field detection" },
  { id: "P040", label: "AGG_MEAN",        desc: "Mean token usage across agents" },
  { id: "P042", label: "AGG_SD",          desc: "Standard deviation of token usage" },
  { id: "P049", label: "AGG_IQR",         desc: "IQR outlier boundary computation" },
  { id: "P093", label: "DETECT_OUTLIER",  desc: "Flag statistical anomalies (1.5x IQR)" },
  { id: "P050", label: "GROUP_AGG",       desc: "Segment aggregation by provider + type" },
  { id: "P052", label: "CROSS_TAB",       desc: "Provider x Type contingency table" },
  { id: "P047", label: "AGG_QUANTILE",    desc: "Decision threshold quantiles [Q1,Q2,Q3]" },
  { id: "G015", label: "EMIT_ARTIFACT",   desc: "Export validated analysis artifact" },
];

// ─── MATH ENGINE: NEURODIVERGENT OMEGA STATE ──────────────────────────────────

function computeOmega(p) {
  const num =
    1.0 *
    (p.grounding_score + p.reference_integrity) *
    (p.threat_load / Math.max(p.sqrt_risk, 0.0001));
  const den =
    (p.reference_integrity + p.grounding_score) +
    (p.intervention_bandwidth * p.env_delta);
  return den === 0 ? 0 : parseFloat((num / den).toFixed(6));
}

function quantumGate(X, Y, Z) {
  return { sum: parseFloat((X + Y).toFixed(3)), Z, migrate: X + Y > Z };
}

// ─── STATISTICAL PRIMITIVES ───────────────────────────────────────────────────

function mean(arr) {
  if (!arr.length) return 0;
  return arr.reduce((a, b) => a + b, 0) / arr.length;
}

function stddev(arr) {
  if (arr.length < 2) return 0;
  const m = mean(arr);
  return Math.sqrt(arr.reduce((a, b) => a + (b - m) ** 2, 0) / arr.length);
}

function quantiles(arr) {
  if (!arr.length) return { q1: 0, median: 0, q3: 0, iqr: 0 };
  const s = [...arr].sort((a, b) => a - b);
  const q1 = s[Math.floor(s.length * 0.25)];
  const median = s[Math.floor(s.length * 0.5)];
  const q3 = s[Math.floor(s.length * 0.75)];
  return { q1, median, q3, iqr: q3 - q1 };
}

function detectOutliers(arr) {
  const { q1, q3, iqr } = quantiles(arr);
  const lo = q1 - 1.5 * iqr;
  const hi = q3 + 1.5 * iqr;
  return arr.filter(v => v < lo || v > hi);
}

// ─── EDA PIPELINE EXECUTOR ────────────────────────────────────────────────────

function runEDA(agents) {
  const log = [];
  const ts = () => new Date().toISOString();

  log.push({ id: "P001", label: "LOAD", ts: ts(),
    result: { count: agents.length, source: API_BASE + "/agents" } });

  const fields = agents.length ? Object.keys(agents[0]) : [];
  log.push({ id: "P010", label: "INSPECT_SHAPE", ts: ts(),
    result: { rows: agents.length, columns: fields.length, fields } });

  const typeMap = {};
  fields.forEach(f => {
    const sample = agents[0][f];
    typeMap[f] = sample === null ? "null" : typeof sample;
  });
  log.push({ id: "P011", label: "INSPECT_TYPES", ts: ts(), result: typeMap });

  const missing = {};
  fields.forEach(f => {
    missing[f] = agents.filter(a => a[f] === null || a[f] === undefined || a[f] === "").length;
  });
  log.push({ id: "P014", label: "INSPECT_MISSING", ts: ts(), result: missing });

  const stateScore = (a) => {
    const m = { idle: 100, active: 500, processing: 800, error: 50 };
    return m[a.state] || 200;
  };
  const scores = agents.map(stateScore);

  const mu = mean(scores);
  log.push({ id: "P040", label: "AGG_MEAN", ts: ts(),
    result: { mean: parseFloat(mu.toFixed(4)), field: "state_score_proxy" } });

  const sd = stddev(scores);
  log.push({ id: "P042", label: "AGG_SD", ts: ts(),
    result: { stddev: parseFloat(sd.toFixed(4)) } });

  const q = quantiles(scores);
  log.push({ id: "P049", label: "AGG_IQR", ts: ts(), result: q });

  const outliers = detectOutliers(scores);
  log.push({ id: "P093", label: "DETECT_OUTLIER", ts: ts(),
    result: { outlier_count: outliers.length, values: outliers,
              fence_lo: q.q1 - 1.5 * q.iqr, fence_hi: q.q3 + 1.5 * q.iqr } });

  const byProvider = {};
  agents.forEach(a => {
    const p = a.provider || "unknown";
    if (!byProvider[p]) byProvider[p] = { count: 0, score_sum: 0, memory: 0, tools: 0 };
    byProvider[p].count++;
    byProvider[p].score_sum += stateScore(a);
    if (a.memory_enabled) byProvider[p].memory++;
    if (a.tools_enabled) byProvider[p].tools++;
  });
  Object.keys(byProvider).forEach(p => {
    byProvider[p].mean_score = parseFloat(
      (byProvider[p].score_sum / byProvider[p].count).toFixed(2));
  });
  log.push({ id: "P050", label: "GROUP_AGG", ts: ts(), result: byProvider });

  const crossTab = {};
  agents.forEach(a => {
    const p = a.provider || "unknown";
    const t = a.type || "unknown";
    if (!crossTab[p]) crossTab[p] = {};
    crossTab[p][t] = (crossTab[p][t] || 0) + 1;
  });
  log.push({ id: "P052", label: "CROSS_TAB", ts: ts(), result: crossTab });

  log.push({ id: "P047", label: "AGG_QUANTILE", ts: ts(),
    result: { Q1_threshold: q.q1, Q2_median: q.median, Q3_threshold: q.q3,
              decision_boundary: q.q3 + 1.5 * q.iqr } });

  const artifact = {
    generated_at: ts(),
    author: "Jason M Jarmacz | Evolution Strategist",
    coauthor: "Claude by Anthropic",
    collaboration: "Human/AI Innovative Collaboration",
    framework: "NeuroDivergent AI Evolution",
    api: API_BASE,
    primitive_count: log.length + 1,
    chain: [...log.map(l => l.id), "G015"],
    summary: {
      total_agents: agents.length,
      mean_score: parseFloat(mu.toFixed(4)),
      stddev: parseFloat(sd.toFixed(4)),
      iqr: q.iqr,
      outliers: outliers.length,
      providers: Object.keys(byProvider),
    },
    primitives: log,
  };
  log.push({ id: "G015", label: "EMIT_ARTIFACT", ts: ts(), result: artifact });

  return { log, artifact };
}

// ─── API CLIENT ───────────────────────────────────────────────────────────────

async function apiFetch(path, opts = {}) {
  const res = await fetch(API_BASE + path, {
    headers: { "Content-Type": "application/json", ...opts.headers },
    ...opts,
  });
  if (!res.ok) throw new Error(`HTTP ${res.status}: ${res.statusText} [${path}]`);
  return res.json();
}

async function apiPost(path, body) {
  return apiFetch(path, { method: "POST", body: JSON.stringify(body) });
}

// ─── DESIGN TOKENS ───────────────────────────────────────────────────────────

const C = {
  bg0: "#080b0f",
  bg1: "#0d1117",
  bg2: "#161b22",
  bg3: "#1c2128",
  border: "#21262d",
  border2: "#30363d",
  text: "#c8d6e5",
  muted: "#8b949e",
  dim: "#6e7681",
  blue: "#58a6ff",
  green: "#3fb950",
  orange: "#f0883e",
  red: "#ff7b72",
  purple: "#d2a8ff",
  teal: "#79c0ff",
};

const S = {
  root: {
    fontFamily: "'JetBrains Mono', 'Fira Code', 'Courier New', monospace",
    background: C.bg0,
    color: C.text,
    minHeight: "100vh",
    fontSize: "12px",
    margin: 0,
    padding: 0,
  },
  header: {
    background: `linear-gradient(135deg, ${C.bg1} 0%, ${C.bg2} 100%)`,
    borderBottom: `1px solid ${C.border}`,
    padding: "16px 24px",
    display: "flex",
    alignItems: "center",
    justifyContent: "space-between",
    position: "sticky",
    top: 0,
    zIndex: 100,
  },
  hTitle: { fontSize: "13px", fontWeight: 700, color: C.blue,
    letterSpacing: "0.1em", textTransform: "uppercase" },
  hSub: { fontSize: "10px", color: C.dim, marginTop: 2 },
  body: { display: "flex", height: "calc(100vh - 58px)" },
  sidebar: { width: 220, background: C.bg1, borderRight: `1px solid ${C.border}`,
    overflowY: "auto", padding: "10px 0", flexShrink: 0 },
  sSection: { padding: "8px 14px 3px", fontSize: "10px", color: C.dim,
    textTransform: "uppercase", letterSpacing: "0.1em" },
  sItem: (active) => ({
    padding: "6px 14px",
    cursor: "pointer",
    display: "flex", alignItems: "center", gap: 7,
    background: active ? C.bg3 : "transparent",
    borderLeft: `2px solid ${active ? C.blue : "transparent"}`,
    transition: "all 0.12s",
  }),
  sDot: (on) => ({
    width: 6, height: 6, borderRadius: "50%", flexShrink: 0,
    background: on ? C.green : C.border2,
    boxShadow: on ? `0 0 5px ${C.green}88` : "none",
  }),
  sLabel: (active) => ({
    fontSize: "11px", fontWeight: 500,
    color: active ? C.blue : C.muted,
  }),
  sId: { fontSize: "10px", color: C.dim },
  main: { flex: 1, overflowY: "auto", padding: "18px 22px" },
  panel: {
    background: C.bg1, border: `1px solid ${C.border}`,
    borderRadius: 6, marginBottom: 14, overflow: "hidden",
  },
  ph: {
    background: C.bg2, borderBottom: `1px solid ${C.border}`,
    padding: "9px 14px", display: "flex",
    alignItems: "center", justifyContent: "space-between",
  },
  pt: { fontSize: "11px", fontWeight: 700, color: "#f0f6fc",
    textTransform: "uppercase", letterSpacing: "0.07em" },
  pb: { padding: 14 },
  grid2: { display: "grid", gridTemplateColumns: "1fr 1fr", gap: 12 },
  grid3: { display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 12 },
  grid4: { display: "grid", gridTemplateColumns: "1fr 1fr 1fr 1fr", gap: 10 },
  card: {
    background: C.bg2, border: `1px solid ${C.border}`,
    borderRadius: 5, padding: 12,
  },
  cardLabel: { fontSize: "10px", color: C.dim, textTransform: "uppercase",
    letterSpacing: "0.08em", marginBottom: 5 },
  cardVal: { fontSize: "20px", fontWeight: 700, color: C.blue },
  cardSub: { fontSize: "10px", color: C.muted, marginTop: 3 },
  btn: (v = "default") => ({
    padding: "6px 14px", borderRadius: 4,
    border: v === "default" ? `1px solid ${C.border2}` : "none",
    background: v === "primary" ? "#238636" : v === "danger" ? "#da3633" :
                v === "warn" ? "#bb6d0c" : C.bg3,
    color: "#f0f6fc", cursor: "pointer", fontSize: "11px",
    fontWeight: 600, letterSpacing: "0.04em", fontFamily: "inherit",
    transition: "opacity 0.12s",
    whiteSpace: "nowrap",
  }),
  row: { display: "flex", alignItems: "center", gap: 8, flexWrap: "wrap" },
  input: {
    background: C.bg1, border: `1px solid ${C.border2}`,
    borderRadius: 4, color: C.text,
    padding: "5px 9px", fontSize: "11px",
    fontFamily: "inherit", width: "100%",
    boxSizing: "border-box",
  },
  lbl: { fontSize: "10px", color: C.muted, marginBottom: 3,
    display: "block", textTransform: "uppercase", letterSpacing: "0.06em" },
  code: {
    background: C.bg2, border: `1px solid ${C.border}`,
    borderRadius: 4, padding: 10, fontSize: "10px",
    overflowX: "auto", whiteSpace: "pre-wrap", wordBreak: "break-all",
    color: C.teal, maxHeight: 280, overflowY: "auto",
    lineHeight: 1.5,
  },
  tag: (c = C.blue) => ({
    display: "inline-block", padding: "2px 7px",
    borderRadius: 10, fontSize: "10px", fontWeight: 600,
    background: c + "22", color: c,
    border: `1px solid ${c}44`,
    marginRight: 3, marginBottom: 2,
  }),
  table: { width: "100%", borderCollapse: "collapse", fontSize: "11px" },
  th: { padding: "6px 9px", background: C.bg2, color: C.muted,
    textAlign: "left", borderBottom: `1px solid ${C.border}`,
    textTransform: "uppercase", letterSpacing: "0.06em", fontSize: "10px" },
  td: { padding: "6px 9px", borderBottom: `1px solid ${C.bg2}`, color: C.text },
  notice: (t = "info") => ({
    padding: "9px 12px", borderRadius: 4, fontSize: "11px", marginBottom: 10,
    background: t === "warn" ? "#f0883e18" : t === "error" ? "#da363318" : "#58a6ff0f",
    border: `1px solid ${t === "warn" ? "#f0883e44" : t === "error" ? "#da363344" : "#58a6ff44"}`,
    color: t === "warn" ? C.orange : t === "error" ? C.red : C.teal,
    fontFamily: "inherit", lineHeight: 1.6,
  }),
  prog: { width: "100%", height: 3, background: C.border, borderRadius: 2,
    overflow: "hidden", margin: "4px 0" },
  progBar: (pct, c = C.green) => ({
    height: "100%", width: `${Math.min(pct, 100)}%`,
    background: c, transition: "width 0.35s ease",
  }),
  spacer: (h = 8) => ({ height: h }),
};

// ─── OMEGA PANEL ─────────────────────────────────────────────────────────────

function OmegaPanel({ params, setParams }) {
  const omega = computeOmega(params);
  const gate = quantumGate(7, 2, 10);
  const health = omega > 1.2 ? "CRITICAL" : omega > 0.8 ? "ELEVATED" : "STABLE";
  const hc = omega > 1.2 ? C.red : omega > 0.8 ? C.orange : C.green;

  const Slider = ({ k, label }) => (
    <div style={{ marginBottom: 10 }}>
      <div style={S.row}>
        <span style={S.lbl}>{label}</span>
        <span style={{ ...S.lbl, marginLeft: "auto", color: C.blue }}>
          {params[k].toFixed(2)}
        </span>
      </div>
      <input type="range" min="0" max="1" step="0.01"
        value={params[k]}
        onChange={e => setParams(p => ({ ...p, [k]: parseFloat(e.target.value) }))}
        style={{ width: "100%", accentColor: C.blue }} />
    </div>
  );

  return (
    <div style={S.panel}>
      <div style={S.ph}>
        <span style={S.pt}>NeuroDivergent AI Evolution - Omega State Equation</span>
        <span style={S.tag(hc)}>{health}</span>
      </div>
      <div style={S.pb}>
        <div style={{ ...S.notice("info"), whiteSpace: "pre" }}>
{`  Omega_ActualState =
    ( alpha * (G + R) * omega * (T / K) )
    / ( iota * (R + G) + iota * (B * D) )

  G = grounding_score       | R = reference_integrity
  T = threat_load           | K = sqrt_risk
  B = intervention_bandwidth | D = env_delta`}
        </div>

        <div style={S.grid2}>
          <div>
            <Slider k="grounding_score"       label="G: Grounding Score" />
            <Slider k="threat_load"           label="T: Threat Load" />
            <Slider k="reference_integrity"   label="R: Reference Integrity" />
          </div>
          <div>
            <Slider k="intervention_bandwidth" label="B: Intervention Bandwidth" />
            <Slider k="env_delta"             label="D: Environment Delta" />
            <Slider k="sqrt_risk"             label="K: Sqrt Risk" />
          </div>
        </div>

        <div style={S.grid4}>
          <div style={S.card}>
            <div style={S.cardLabel}>Omega Result</div>
            <div style={{ ...S.cardVal, color: hc }}>{omega.toFixed(6)}</div>
            <div style={S.cardSub}>Actual State Score</div>
          </div>
          <div style={S.card}>
            <div style={S.cardLabel}>State Assessment</div>
            <div style={{ ...S.cardVal, fontSize: 15, color: hc }}>{health}</div>
            <div style={S.prog}><div style={S.progBar(Math.min(omega * 50, 100), hc)} /></div>
          </div>
          <div style={S.card}>
            <div style={S.cardLabel}>Quantum Gate</div>
            <div style={{ ...S.cardVal, fontSize: 14,
              color: gate.migrate ? C.red : C.green }}>
              {gate.migrate ? "MIGRATE NOW" : "DEFERRED"}
            </div>
            <div style={S.cardSub}>X+Y={gate.sum} vs Z={gate.Z}</div>
          </div>
          <div style={S.card}>
            <div style={S.cardLabel}>Q-Day Horizon</div>
            <div style={{ ...S.cardVal, fontSize: 20, color: C.orange }}>
              Z &lt;= 10
            </div>
            <div style={S.cardSub}>Years to cryptographic risk</div>
          </div>
        </div>
      </div>
    </div>
  );
}

// ─── AGENT PANEL ─────────────────────────────────────────────────────────────

function AgentPanel({ agents, loading, onLoad, health }) {
  const [form, setForm] = useState({
    name: "", type: "multimodal", provider: "anthropic",
    model: "", description: "", system_prompt: "",
    temperature: 0.7, max_tokens: 4096,
    enable_memory: true, enable_tools: true,
    enable_vision: true, enable_audio: true,
  });
  const [creating, setCreating] = useState(false);
  const [notice, setNotice] = useState(null);

  const handleCreate = async () => {
    if (!form.name.trim()) { setNotice({ t: "error", m: "Agent name required." }); return; }
    setCreating(true); setNotice(null);
    try {
      const body = { ...form };
      if (!body.model) delete body.model;
      if (!body.system_prompt) delete body.system_prompt;
      const r = await apiPost("/agents", body);
      setNotice({ t: "info", m: `Created: ${r.name} [${r.id}]` });
      setForm(f => ({ ...f, name: "" }));
      await onLoad();
    } catch (e) { setNotice({ t: "error", m: e.message }); }
    finally { setCreating(false); }
  };

  const handleDelete = async (id, name) => {
    try {
      await apiFetch("/agents/" + id, { method: "DELETE" });
      setNotice({ t: "warn", m: `Deleted: ${name}` });
      await onLoad();
    } catch (e) { setNotice({ t: "error", m: e.message }); }
  };

  return (
    <div>
      <div style={S.panel}>
        <div style={S.ph}>
          <span style={S.pt}>Create Agent</span>
          <div style={S.row}>
            {health && <span style={S.tag(health.status === "healthy" ? C.green : C.red)}>
              {health.status} v{health.version}
            </span>}
          </div>
        </div>
        <div style={S.pb}>
          {notice && <div style={S.notice(notice.t)}>{notice.m}</div>}
          <div style={S.grid3}>
            <div>
              <label style={S.lbl}>Name *</label>
              <input style={S.input} value={form.name}
                onChange={e => setForm(f => ({ ...f, name: e.target.value }))}
                placeholder="ndae-agent-01" />
            </div>
            <div>
              <label style={S.lbl}>Type</label>
              <select style={S.input} value={form.type}
                onChange={e => setForm(f => ({ ...f, type: e.target.value }))}>
                <option>multimodal</option><option>chat</option><option>specialist</option>
              </select>
            </div>
            <div>
              <label style={S.lbl}>Provider</label>
              <select style={S.input} value={form.provider}
                onChange={e => setForm(f => ({ ...f, provider: e.target.value }))}>
                <option>anthropic</option><option>openai</option><option>google</option>
              </select>
            </div>
          </div>
          <div style={S.grid2}>
            <div>
              <label style={S.lbl}>Model (optional)</label>
              <input style={S.input} value={form.model}
                onChange={e => setForm(f => ({ ...f, model: e.target.value }))}
                placeholder="claude-sonnet-4-6" />
            </div>
            <div>
              <label style={S.lbl}>Temperature [{form.temperature}]</label>
              <input type="range" min="0" max="2" step="0.05" value={form.temperature}
                onChange={e => setForm(f => ({ ...f, temperature: parseFloat(e.target.value) }))}
                style={{ width: "100%", accentColor: C.blue, marginTop: 6 }} />
            </div>
          </div>
          <label style={S.lbl}>System Prompt</label>
          <textarea style={{ ...S.input, height: 55, resize: "vertical" }}
            value={form.system_prompt}
            onChange={e => setForm(f => ({ ...f, system_prompt: e.target.value }))}
            placeholder="You are a NeuroDivergent AI Evolution agent..." />
          <div style={{ ...S.spacer(6) }} />
          <div style={S.row}>
            {["enable_memory","enable_tools","enable_vision","enable_audio"].map(k => (
              <label key={k} style={{ ...S.row, cursor: "pointer", fontSize: "10px",
                color: C.muted, gap: 4 }}>
                <input type="checkbox" checked={form[k]}
                  onChange={e => setForm(f => ({ ...f, [k]: e.target.checked }))}
                  style={{ accentColor: C.green }} />
                {k.replace("enable_", "").toUpperCase()}
              </label>
            ))}
            <button style={{ ...S.btn("primary"), marginLeft: "auto" }}
              onClick={handleCreate} disabled={creating}>
              {creating ? "Creating..." : "CREATE AGENT"}
            </button>
          </div>
        </div>
      </div>

      <div style={S.panel}>
        <div style={S.ph}>
          <span style={S.pt}>Agent Registry</span>
          <div style={S.row}>
            <span style={S.tag(C.blue)}>{agents.length} registered</span>
            <button style={S.btn()} onClick={onLoad} disabled={loading}>
              {loading ? "Syncing..." : "REFRESH"}
            </button>
          </div>
        </div>
        <div style={S.pb}>
          {loading && <div style={S.notice("info")}>Loading agent registry...</div>}
          {!loading && agents.length === 0 && (
            <div style={S.notice("warn")}>No agents found. Create one above.</div>
          )}
          {agents.length > 0 && (
            <table style={S.table}>
              <thead>
                <tr>
                  {["ID","Name","Type","Provider","State","Memory","Tools","Vision","Actions"].map(h => (
                    <th key={h} style={S.th}>{h}</th>
                  ))}
                </tr>
              </thead>
              <tbody>
                {agents.map(a => (
                  <tr key={a.id}>
                    <td style={{ ...S.td, color: C.dim, fontSize: "10px" }}>
                      {a.id.slice(0, 10)}...
                    </td>
                    <td style={{ ...S.td, color: C.teal, fontWeight: 600 }}>{a.name}</td>
                    <td style={S.td}><span style={S.tag(C.teal)}>{a.type}</span></td>
                    <td style={S.td}><span style={S.tag(C.orange)}>{a.provider}</span></td>
                    <td style={S.td}>
                      <span style={S.tag(a.state === "idle" ? C.green : C.orange)}>
                        {a.state}
                      </span>
                    </td>
                    <td style={S.td}>
                      <span style={S.tag(a.memory_enabled ? C.green : C.dim)}>
                        {a.memory_enabled ? "ON" : "OFF"}
                      </span>
                    </td>
                    <td style={S.td}>
                      <span style={S.tag(a.tools_enabled ? C.green : C.dim)}>
                        {a.tools_enabled ? "ON" : "OFF"}
                      </span>
                    </td>
                    <td style={S.td}>
                      <span style={S.tag(a.capabilities?.vision ? C.green : C.dim)}>
                        {a.capabilities?.vision ? "ON" : "OFF"}
                      </span>
                    </td>
                    <td style={S.td}>
                      <button
                        style={{ ...S.btn("danger"), padding: "3px 7px", fontSize: "10px" }}
                        onClick={() => handleDelete(a.id, a.name)}>
                        DEL
                      </button>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          )}
        </div>
      </div>
    </div>
  );
}

// ─── EDA PANEL ───────────────────────────────────────────────────────────────

function EDAPanel({ agents, activeStep, setActiveStep }) {
  const [result, setResult] = useState(null);
  const [running, setRunning] = useState(false);

  const run = () => {
    if (!agents.length) return;
    setRunning(true); setResult(null); setActiveStep(0);
    let step = 0;
    const tick = setInterval(() => {
      step++;
      setActiveStep(step);
      if (step >= EDA_CHAIN.length - 1) {
        clearInterval(tick);
        setResult(runEDA(agents));
        setRunning(false);
      }
    }, 180);
  };

  const download = () => {
    if (!result) return;
    const blob = new Blob(
      [JSON.stringify(result.artifact, null, 2)],
      { type: "application/json" }
    );
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = `ndae_eda_${Date.now()}.json`;
    a.click();
    URL.revokeObjectURL(url);
  };

  return (
    <div>
      <div style={S.panel}>
        <div style={S.ph}>
          <span style={S.pt}>Knockout EDA Chain — Full Recon Blitz</span>
          <div style={S.row}>
            {result && <button style={S.btn()} onClick={download}>EXPORT G015</button>}
            <button style={S.btn("primary")} onClick={run}
              disabled={running || agents.length === 0}>
              {running ? "EXECUTING..." : "RUN FULL CHAIN"}
            </button>
          </div>
        </div>
        <div style={S.pb}>
          {agents.length === 0 && (
            <div style={S.notice("warn")}>
              P001 LOAD requires live agents. Load registry first via AGENTS tab.
            </div>
          )}
          {EDA_CHAIN.map((step, i) => {
            const done = result ? true : i < activeStep;
            const active = i === activeStep && running;
            return (
              <div key={step.id} style={{
                display: "flex", alignItems: "center", gap: 10,
                padding: "5px 0", borderBottom: `1px solid ${C.bg2}`,
              }}>
                <span style={S.sDot(done)} />
                <span style={{ ...S.sId, width: 42 }}>{step.id}</span>
                <span style={{ fontSize: "11px", fontWeight: 600, width: 140,
                  color: active ? C.orange : done ? C.green : C.dim }}>
                  {step.label}
                </span>
                <span style={{ fontSize: "10px", color: C.dim }}>{step.desc}</span>
                {active && (
                  <span style={{ marginLeft: "auto", fontSize: "10px",
                    color: C.orange }}>RUNNING</span>
                )}
                {done && !active && (
                  <span style={{ marginLeft: "auto", fontSize: "10px",
                    color: C.green }}>COMPLETE</span>
                )}
              </div>
            );
          })}
        </div>
      </div>

      {result && (
        <>
          <div style={S.panel}>
            <div style={S.ph}><span style={S.pt}>Summary Statistics</span></div>
            <div style={S.pb}>
              <div style={S.grid4}>
                {[
                  ["Total Agents",  result.artifact.summary.total_agents, C.blue],
                  ["Mean Score",    result.artifact.summary.mean_score,    C.teal],
                  ["Std Deviation", result.artifact.summary.stddev,        C.purple],
                  ["Outliers",      result.artifact.summary.outliers,
                    result.artifact.summary.outliers > 0 ? C.orange : C.green],
                ].map(([lbl, val, c]) => (
                  <div key={lbl} style={S.card}>
                    <div style={S.cardLabel}>{lbl}</div>
                    <div style={{ ...S.cardVal, color: c }}>{val}</div>
                  </div>
                ))}
              </div>
            </div>
          </div>

          <div style={S.grid2}>
            <div style={S.panel}>
              <div style={S.ph}><span style={S.pt}>P050 GROUP_AGG</span></div>
              <div style={S.pb}>
                <table style={S.table}>
                  <thead>
                    <tr>{["Provider","N","Mean","Mem","Tools"].map(h =>
                      <th key={h} style={S.th}>{h}</th>)}</tr>
                  </thead>
                  <tbody>
                    {Object.entries(
                      result.log.find(l => l.id === "P050").result
                    ).map(([p, d]) => (
                      <tr key={p}>
                        <td style={{ ...S.td, color: C.orange }}>{p}</td>
                        <td style={S.td}>{d.count}</td>
                        <td style={S.td}>{d.mean_score}</td>
                        <td style={S.td}>{d.memory}</td>
                        <td style={S.td}>{d.tools}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </div>

            <div style={S.panel}>
              <div style={S.ph}><span style={S.pt}>P047 AGG_QUANTILE</span></div>
              <div style={S.pb}>
                {Object.entries(
                  result.log.find(l => l.id === "P047").result
                ).map(([k, v]) => (
                  <div key={k} style={{ marginBottom: 8 }}>
                    <div style={S.row}>
                      <span style={S.lbl}>{k}</span>
                      <span style={{ ...S.lbl, marginLeft: "auto",
                        color: C.blue }}>{v}</span>
                    </div>
                    <div style={S.prog}>
                      <div style={S.progBar(
                        Math.min((v / 1000) * 100, 100), C.teal
                      )} />
                    </div>
                  </div>
                ))}
              </div>
            </div>
          </div>

          <div style={S.panel}>
            <div style={S.ph}><span style={S.pt}>P052 CROSS_TAB — Provider x Type</span></div>
            <div style={S.pb}>
              <div style={S.code}>
                {JSON.stringify(
                  result.log.find(l => l.id === "P052").result, null, 2
                )}
              </div>
            </div>
          </div>

          <div style={S.panel}>
            <div style={S.ph}><span style={S.pt}>G015 EMIT_ARTIFACT — Validated Output</span></div>
            <div style={S.pb}>
              <div style={S.code}>
                {JSON.stringify(result.artifact.summary, null, 2)}
              </div>
            </div>
          </div>
        </>
      )}
    </div>
  );
}

// ─── CHAT PANEL ──────────────────────────────────────────────────────────────

function ChatPanel({ agents }) {
  const [agentId, setAgentId] = useState(agents[0]?.id || "");
  const [msg, setMsg] = useState("");
  const [history, setHistory] = useState([]);
  const [sending, setSending] = useState(false);
  const [error, setError] = useState(null);
  const endRef = useRef(null);

  useEffect(() => { endRef.current?.scrollIntoView({ behavior: "smooth" }); }, [history]);

  const send = async () => {
    const text = msg.trim();
    if (!agentId || !text) return;
    setMsg(""); setSending(true); setError(null);
    setHistory(h => [...h, { role: "user", content: text,
      ts: new Date().toISOString() }]);
    try {
      const res = await apiPost(`/agents/${agentId}/chat`, {
        message: text,
        context: history.slice(-6).map(m => ({
          role: m.role, content: m.content })),
      });
      setHistory(h => [...h, { role: "assistant", content: res.content,
        ts: res.timestamp, usage: res.usage }]);
    } catch (e) {
      setError(e.message);
      setHistory(h => [...h, { role: "error", content: e.message,
        ts: new Date().toISOString() }]);
    } finally { setSending(false); }
  };

  return (
    <div style={S.panel}>
      <div style={S.ph}>
        <span style={S.pt}>Agent Chat</span>
        <div style={S.row}>
          <select style={{ ...S.input, width: 240 }} value={agentId}
            onChange={e => setAgentId(e.target.value)}>
            {agents.map(a => (
              <option key={a.id} value={a.id}>
                {a.name} [{a.provider}]
              </option>
            ))}
          </select>
          <button style={S.btn()} onClick={() => setHistory([])}>CLR</button>
        </div>
      </div>
      <div style={S.pb}>
        {error && <div style={S.notice("error")}>{error}</div>}
        <div style={{ minHeight: 160, maxHeight: 320, overflowY: "auto",
          marginBottom: 10 }}>
          {history.length === 0 && (
            <div style={S.notice("info")}>
              Agent ready. Enter a message to begin.
            </div>
          )}
          {history.map((m, i) => (
            <div key={i} style={{
              padding: "9px 12px", borderRadius: 4, marginBottom: 6,
              background: m.role === "user" ? "#1c285022" :
                          m.role === "error" ? "#2d0f0f" : C.bg2,
              border: `1px solid ${m.role === "user" ? C.blue + "44" :
                m.role === "error" ? C.red + "44" : C.border}`,
              maxWidth: m.role === "user" ? "80%" : "100%",
              marginLeft: m.role === "user" ? "auto" : 0,
            }}>
              <div style={{ fontSize: "10px", color: C.dim, marginBottom: 3 }}>
                {m.role.toUpperCase()} — {m.ts?.slice(11, 19)}
                {m.usage &&
                  ` | in:${m.usage.input_tokens} out:${m.usage.output_tokens}`}
              </div>
              <div style={{ whiteSpace: "pre-wrap", fontSize: "12px",
                color: m.role === "error" ? C.red : C.text }}>
                {m.content}
              </div>
            </div>
          ))}
          <div ref={endRef} />
        </div>
        <div style={S.row}>
          <input style={S.input} value={msg}
            onChange={e => setMsg(e.target.value)}
            onKeyDown={e => e.key === "Enter" && !e.shiftKey && send()}
            placeholder="Message... (Enter to send)" />
          <button style={S.btn("primary")} onClick={send}
            disabled={sending || !agentId}>
            {sending ? "..." : "SEND"}
          </button>
        </div>
      </div>
    </div>
  );
}

// ─── TRAINING PANEL ──────────────────────────────────────────────────────────

function TrainingPanel({ agents }) {
  const [agentName, setAgentName] = useState(agents[0]?.name || "");
  const [epochs, setEpochs] = useState(3);
  const [status, setStatus] = useState(null);
  const [ledger, setLedger] = useState(null);
  const [error, setError] = useState(null);
  const [busy, setBusy] = useState(false);

  useEffect(() => {
    if (agents.length && !agentName) setAgentName(agents[0].name);
  }, [agents]);

  const act = async (fn) => {
    setBusy(true); setError(null);
    try { await fn(); }
    catch (e) { setError(e.message); }
    finally { setBusy(false); }
  };

  return (
    <div style={S.panel}>
      <div style={S.ph}>
        <span style={S.pt}>Recursive Training + Loop Closure Ledger</span>
      </div>
      <div style={S.pb}>
        {error && <div style={S.notice("error")}>{error}</div>}
        <div style={S.grid3}>
          <div>
            <label style={S.lbl}>Agent Name</label>
            <input style={S.input} value={agentName}
              onChange={e => setAgentName(e.target.value)} />
          </div>
          <div>
            <label style={S.lbl}>Epochs [1-100]</label>
            <input type="number" min="1" max="100" style={S.input}
              value={epochs}
              onChange={e => setEpochs(parseInt(e.target.value))} />
          </div>
          <div style={{ display: "flex", alignItems: "flex-end", gap: 6 }}>
            <button style={S.btn("primary")} disabled={busy}
              onClick={() => act(async () => {
                const r = await apiPost("/training/start", {
                  agent_name: agentName, epochs, enable_closure: true });
                setStatus(r);
              })}>START</button>
            <button style={S.btn()} disabled={busy}
              onClick={() => act(async () => {
                const r = await apiFetch("/training/status/" + agentName);
                setStatus(r);
              })}>STATUS</button>
            <button style={S.btn()} disabled={busy}
              onClick={() => act(async () => {
                const r = await apiFetch("/training/ledger/" + agentName);
                setLedger(r);
              })}>LEDGER</button>
            <button style={S.btn("warn")} disabled={busy}
              onClick={() => act(async () => {
                await apiFetch("/training/reset/" + agentName,
                  { method: "DELETE" });
                setStatus(null); setLedger(null);
              })}>RESET</button>
          </div>
        </div>

        {status && (
          <div style={{ marginTop: 12 }}>
            <div style={S.grid4}>
              {[
                ["Status",        status.status,          status.status === "training" ? C.orange : C.green],
                ["Epoch",         `${status.current_epoch ?? 0} / ${status.total_epochs ?? epochs}`, C.blue],
                ["Loops Closed",  status.loops_closed ?? 0,   C.teal],
                ["Patterns",      status.patterns_found ?? 0, C.purple],
              ].map(([lbl, val, c]) => (
                <div key={lbl} style={S.card}>
                  <div style={S.cardLabel}>{lbl}</div>
                  <div style={{ ...S.cardVal, fontSize: 15, color: c }}>{val}</div>
                </div>
              ))}
            </div>
            <div style={{ ...S.notice("info"), marginTop: 8 }}>{status.message}</div>
          </div>
        )}

        {ledger && (
          <div style={{ marginTop: 12 }}>
            <div style={S.grid2}>
              <div style={S.card}>
                <div style={S.cardLabel}>Total Loop Closures</div>
                <div style={S.cardVal}>{ledger.total_loops}</div>
              </div>
              <div style={S.card}>
                <div style={S.cardLabel}>Closure Rate</div>
                <div style={{ ...S.cardVal, color: C.green }}>
                  {(ledger.closure_rate * 100).toFixed(1)}%
                </div>
                <div style={S.prog}>
                  <div style={S.progBar(ledger.closure_rate * 100)} />
                </div>
              </div>
            </div>
            {ledger.recent_loops?.length > 0 && (
              <div style={{ marginTop: 8 }}>
                <div style={S.lbl}>Recent Loop Closures</div>
                <div style={S.code}>
                  {JSON.stringify(ledger.recent_loops, null, 2)}
                </div>
              </div>
            )}
          </div>
        )}
      </div>
    </div>
  );
}

// ─── RAG PANEL ───────────────────────────────────────────────────────────────

function RAGPanel() {
  const [docs, setDocs] = useState([]);
  const [chunks, setChunks] = useState([]);
  const [error, setError] = useState(null);
  const [loading, setLoading] = useState(false);

  const load = async () => {
    setLoading(true); setError(null);
    try {
      const [d, c] = await Promise.all([
        apiFetch("/rag/documents"),
        apiFetch("/rag/chunks"),
      ]);
      setDocs(d); setChunks(c);
    } catch (e) { setError(e.message); }
    finally { setLoading(false); }
  };

  return (
    <div style={S.panel}>
      <div style={S.ph}>
        <span style={S.pt}>RAG Document + Chunk Store (1536-dim)</span>
        <button style={S.btn()} onClick={load} disabled={loading}>
          {loading ? "Loading..." : "LOAD"}
        </button>
      </div>
      <div style={S.pb}>
        {error && <div style={S.notice("error")}>{error}</div>}
        <div style={S.grid2}>
          <div style={S.card}>
            <div style={S.cardLabel}>Documents</div>
            <div style={S.cardVal}>{docs.length}</div>
          </div>
          <div style={S.card}>
            <div style={S.cardLabel}>Vector Chunks</div>
            <div style={S.cardVal}>{chunks.length}</div>
            <div style={S.cardSub}>1536-dimensional embeddings</div>
          </div>
        </div>
        {docs.length > 0 && (
          <div style={{ marginTop: 10 }}>
            <div style={S.lbl}>Documents</div>
            <div style={S.code}>{JSON.stringify(docs, null, 2)}</div>
          </div>
        )}
        {chunks.length > 0 && (
          <div style={{ marginTop: 10 }}>
            <div style={S.lbl}>Chunks (first 3)</div>
            <div style={S.code}>
              {JSON.stringify(chunks.slice(0, 3), null, 2)}
            </div>
          </div>
        )}
        {docs.length === 0 && chunks.length === 0 && !error && !loading && (
          <div style={S.notice("info")}>
            RAG store empty or not yet queried. Click LOAD to fetch from /rag/documents and /rag/chunks.
          </div>
        )}
      </div>
    </div>
  );
}

// ─── QUICK START PANEL ───────────────────────────────────────────────────────

function QuickPanel() {
  const [msg, setMsg] = useState("");
  const [results, setResults] = useState({});
  const [loading, setLoading] = useState({});
  const providers = [
    { key: "gpt4",   label: "GPT-4",   path: "/quick-start/chat-gpt4" },
    { key: "gemini", label: "Gemini",  path: "/quick-start/chat-gemini" },
    { key: "claude", label: "Claude",  path: "/quick-start/chat-claude" },
  ];

  const fire = async (p) => {
    if (!msg.trim()) return;
    setLoading(l => ({ ...l, [p.key]: true }));
    try {
      const form = new URLSearchParams({ message: msg });
      const res = await fetch(API_BASE + p.path, {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: form,
      });
      const data = await res.json();
      setResults(r => ({ ...r, [p.key]: data.content || JSON.stringify(data) }));
    } catch (e) {
      setResults(r => ({ ...r, [p.key]: "Error: " + e.message }));
    } finally {
      setLoading(l => ({ ...l, [p.key]: false }));
    }
  };

  const fireAll = () => providers.forEach(p => fire(p));

  return (
    <div style={S.panel}>
      <div style={S.ph}>
        <span style={S.pt}>Quick Start — Provider Comparison</span>
        <button style={S.btn("primary")} onClick={fireAll}
          disabled={!msg.trim()}>
          FIRE ALL
        </button>
      </div>
      <div style={S.pb}>
        <div style={S.row}>
          <input style={S.input} value={msg}
            onChange={e => setMsg(e.target.value)}
            onKeyDown={e => e.key === "Enter" && fireAll()}
            placeholder="Enter prompt to compare across all providers..." />
        </div>
        <div style={{ ...S.spacer(10) }} />
        <div style={S.grid3}>
          {providers.map(p => (
            <div key={p.key}>
              <div style={S.row}>
                <span style={S.tag(
                  p.key === "claude" ? C.orange :
                  p.key === "gpt4" ? C.green : C.purple
                )}>{p.label}</span>
                <button style={{ ...S.btn(), padding: "3px 8px", fontSize: "10px" }}
                  onClick={() => fire(p)}
                  disabled={loading[p.key] || !msg.trim()}>
                  {loading[p.key] ? "..." : "FIRE"}
                </button>
              </div>
              <div style={{ ...S.code, marginTop: 6, minHeight: 80,
                color: results[p.key] ? C.text : C.dim }}>
                {results[p.key] || "Awaiting response..."}
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}

// ─── ROOT ────────────────────────────────────────────────────────────────────

const TABS = [
  { id: "OMEGA",    label: "OMEGA STATE" },
  { id: "AGENTS",   label: "AGENTS" },
  { id: "EDA",      label: "EDA CHAIN" },
  { id: "CHAT",     label: "CHAT" },
  { id: "TRAINING", label: "TRAINING" },
  { id: "RAG",      label: "RAG" },
  { id: "QUICK",    label: "QUICK START" },
];

export default function App() {
  const [tab, setTab] = useState("OMEGA");
  const [agents, setAgents] = useState([]);
  const [health, setHealth] = useState(null);
  const [loading, setLoading] = useState(false);
  const [apiError, setApiError] = useState(null);
  const [omegaParams, setOmegaParams] = useState(OMEGA_DEFAULTS);
  const [edaStep, setEdaStep] = useState(-1);

  const loadAgents = useCallback(async () => {
    setLoading(true); setApiError(null);
    try {
      const [h, a] = await Promise.all([
        apiFetch("/health"),
        apiFetch("/agents"),
      ]);
      setHealth(h); setAgents(a);
    } catch (e) { setApiError(e.message); }
    finally { setLoading(false); }
  }, []);

  useEffect(() => { loadAgents(); }, []);

  const omega = computeOmega(omegaParams);

  return (
    <div style={S.root}>
      <div style={S.header}>
        <div>
          <div style={S.hTitle}>
            GLOG EDA ENGINE — NeuroDivergent AI Evolution
          </div>
          <div style={S.hSub}>
            glogalhosts.com | Multimodal Agent Builder v0.1.0 | 26 Endpoints | OpenAPI 3.1.0
          </div>
        </div>
        <div style={S.row}>
          <span style={S.tag(C.orange)}>Omega: {omega.toFixed(4)}</span>
          <span style={S.tag(
            health?.status === "healthy" ? C.green : C.red
          )}>
            {health ? health.status.toUpperCase() : "CONNECTING"}
          </span>
          {health?.api_keys_configured && Object.entries(health.api_keys_configured)
            .filter(([,v]) => v)
            .map(([k]) => (
              <span key={k} style={S.tag(C.teal)}>{k}</span>
            ))}
          <button style={S.btn()} onClick={loadAgents} disabled={loading}>
            {loading ? "SYNCING..." : "SYNC"}
          </button>
        </div>
      </div>

      <div style={S.body}>
        <div style={S.sidebar}>
          <div style={S.sSection}>Modules</div>
          {TABS.map(t => (
            <div key={t.id} style={S.sItem(tab === t.id)}
              onClick={() => setTab(t.id)}>
              <span style={S.sDot(tab === t.id)} />
              <span style={S.sLabel(tab === t.id)}>{t.label}</span>
            </div>
          ))}

          <div style={{ ...S.sSection, marginTop: 16 }}>EDA Chain</div>
          {EDA_CHAIN.map((step, i) => (
            <div key={step.id} style={S.sItem(i === edaStep)}
              onClick={() => setTab("EDA")}>
              <span style={S.sDot(edaStep > i)} />
              <div>
                <div style={S.sId}>{step.id}</div>
                <div style={{ ...S.sLabel(false), fontSize: "10px" }}>
                  {step.label}
                </div>
              </div>
            </div>
          ))}

          <div style={{ ...S.sSection, marginTop: 16 }}>API</div>
          {[
            ["/health",              C.green],
            ["/agents GET/POST",     C.blue],
            ["/agents/{id}/chat",    C.teal],
            ["/agents/{id}/invoke",  C.teal],
            ["/training/start",      C.orange],
            ["/training/ledger",     C.orange],
            ["/rag/documents",       C.purple],
            ["/rag/chunks",          C.purple],
            ["/quick-start/*",       C.muted],
          ].map(([ep, c]) => (
            <div key={ep} style={{ padding: "3px 14px" }}>
              <span style={{ fontSize: "10px", color: c }}>{ep}</span>
            </div>
          ))}

          <div style={{ padding: "16px 14px 8px" }}>
            <div style={{ fontSize: "10px", color: C.dim, lineHeight: 1.6 }}>
              Author: Jason M Jarmacz
              <br />Co-Author: Claude by Anthropic
              <br />NeuroDivergent AI Evolution
            </div>
          </div>
        </div>

        <div style={S.main}>
          {apiError && (
            <div style={S.notice("error")}>
              API Error: {apiError}
            </div>
          )}

          {tab === "OMEGA"    && <OmegaPanel params={omegaParams} setParams={setOmegaParams} />}
          {tab === "AGENTS"   && <AgentPanel agents={agents} loading={loading}
                                   onLoad={loadAgents} health={health} />}
          {tab === "EDA"      && <EDAPanel agents={agents}
                                   activeStep={edaStep} setActiveStep={setEdaStep} />}
          {tab === "CHAT"     && (agents.length === 0
            ? <div style={S.notice("warn")}>No agents loaded. Create one in AGENTS first.</div>
            : <ChatPanel agents={agents} />)}
          {tab === "TRAINING" && <TrainingPanel agents={agents} />}
          {tab === "RAG"      && <RAGPanel />}
          {tab === "QUICK"    && <QuickPanel />}
        </div>
      </div>
    </div>
  );
}
