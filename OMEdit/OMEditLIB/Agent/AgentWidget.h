/*
 * This file is part of OpenModelica.
 * AI Agent dock: a chat panel that drives an external agent process (the Machina Modelica agent kit) which in
 * turn uses OMEdit's own MCP tools (diagram editing, parameters, simulation, plots) plus the Machina vocabulary,
 * oracle diagnostics and the verified Modelica→Machina converter. Licensed under OSMC-PL 1.2 / GPL v3 like OMEdit.
 */
#pragma once

#include <QWidget>
#include <QProcess>
#include <QJsonObject>
#include <QJsonArray>

class QTextBrowser;
class QPlainTextEdit;
class QPushButton;
class QComboBox;
class QLabel;
class MCPServer;

class AgentWidget : public QWidget
{
  Q_OBJECT
public:
  explicit AgentWidget(QWidget *pParent = nullptr);
  ~AgentWidget();
  static AgentWidget* instance() { return mpInstance; }
  bool isRunning() const;
  void sendTask(const QString &task, const QJsonObject &context = QJsonObject());
  void ensureMCPServer();
  int mcpPort() const { return mMCPPort; }
public slots:
  void start();
  void stop();
  void submit();
  void convertCurrentModel();
  void verifyCurrentModel();
  void explainCurrentModel();
private slots:
  void readStdout();
  void readStderr();
  void processFinished(int exitCode, QProcess::ExitStatus status);
  void processError(QProcess::ProcessError error);
private:
  void handleEvent(const QJsonObject &event);
  void appendHtml(const QString &html);
  void setStatus(const QString &text, bool busy);
  QString currentClassName() const;

  static AgentWidget *mpInstance;
  QTextBrowser *mpTranscript;
  QPlainTextEdit *mpInput;
  QPushButton *mpSendButton;
  QPushButton *mpStopButton;
  QComboBox *mpModelComboBox;
  QLabel *mpStatusLabel;
  QProcess *mpProcess;
  QByteArray mStdoutBuffer;
  MCPServer *mpMCPServer;
  int mMCPPort;
  bool mBusy;
};
