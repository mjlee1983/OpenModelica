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
