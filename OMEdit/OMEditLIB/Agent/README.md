# OMEdit AI Agent panel

An in-OMEdit agent: one window where you chat, and the agent builds Modelica, places components, draws connections,
sets parameters, checks, simulates and plots — in the live OMEdit session — with the Machina vocabulary and oracle.

## How it works

- `AgentWidget` (dock "AI Agent", View > Windows, or **AI Agent > AI Agent Panel**, `Ctrl+Shift+A`) runs the
  [Machina Modelica agent kit](https://github.com/create-flow-ai/machina-modelica/tree/main/agent) as a subprocess
  (`node <kit>/src/omedit-agent.mjs`) and talks to it over JSON lines on stdio.
- The kit discovers OMEdit's own MCP tools (`MCP/`) over HTTP (`http://localhost:<port>/mcp[/admin]`) and offers
  them to the model as `omedit_*` tools next to `machina_cheatsheet`, `machina_check` (OpenModelica check +
  simulate with stage-classified diagnostics) and `machina_convert` (verified Modelica → Machina).
- If the MCP server was not enabled at startup, the panel starts one for the session (`AgentWidget::ensureMCPServer`).
- **AI Agent** menu: *Verify Current Model with Machina*, *Convert Current Model to Machina* (applied only after the
  side-by-side equivalence check passes), *Explain Current Model*.
- **Tools > Options > AI Agent**: node executable, kit folder, workspace, default model, MCP server settings.
  Models are configured in `<workspace>/.machina/config.json` (provider, deployment, environment variable names
  for endpoint and API key); keys never touch disk or OMEdit settings.

## Requirements

Qt 6.4+ with QtHttpServer for the GUI tools (same requirement as the MCP server); with older Qt the panel still
writes and verifies Modelica but cannot drive the diagram. Node.js 18+ for the kit. OpenModelica as usual.

## Files

`Agent/AgentWidget.{h,cpp}` (dock + process bridge), `Agent/AgentSettingsPage.{h,cpp}` (options page),
wiring in `MainWindow.{h,cpp}`, `Options/OptionsDialog.{h,cpp}`, `CMakeLists.txt`, `OMEditLIB.pro`.


## Verified end to end (2026-09-05)

Headless run of the built OMEdit (Linux arm64, Xvfb) with the Machina agent kit driving it over the MCP server:
the agent read the Machina sheet, verified a Machina mass-spring-damper with the local omc (`machina_check`),
loaded Machina into the GUI (`omedit_loadFile`), created the class (`omedit_createClass` + `omedit_setSourceCode`),
simulated it (`omedit_simulate`) and read the final position back from the result
(`omedit_getSimulationResultVariables`): 0.005346 m at 10 s, matching the oracle. Nine tool steps.

Requirements learned from that run:

- **Node.js 20 or newer** on the machine (the kit's MCP SDK needs `fetch`/`undici`; Ubuntu 24.04's Node 18 fails).
- **omc refuses interactive server mode as root**; run OMEdit as a normal user.
- **The Modelica Standard Library is not part of an OpenModelica source build.** Either install it once
  (`installPackage(Modelica, "4.0.0+maint.om", exactMatch=true)` in the OMC shell, which writes to
  `~/.openmodelica/libraries`) or ship it under `<install>/lib/omlibrary` and start OMEdit through
  `omedit-ai` (this folder), which sets `OPENMODELICALIBRARY` to the bundled tree plus the user's libraries.
  omc only honours `OPENMODELICALIBRARY`, not `MODELICAPATH`.
- The kit is self-contained (`agent/lib/` carries `Machina.mo` and the sheet); point Tools > Options > AI Agent >
  kit folder at it and run `machina-modelica init <workspace>` once per workspace.
