/*
 * This file is part of OpenModelica. Licensed under OSMC-PL 1.2 / GPL v3 like OMEdit.
 * AI Agent dock implementation: see AgentWidget.h.
 */
#include "Agent/AgentWidget.h"
#include "MainWindow.h"
#include "MCP/MCPServer.h"
#include "Modeling/ModelWidgetContainer.h"
#include "Modeling/LibraryTreeWidget.h"
#include "Options/OptionsDialog.h"
#include "Util/Utilities.h"

#include <QTextBrowser>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QDateTime>

AgentWidget *AgentWidget::mpInstance = nullptr;

AgentWidget::AgentWidget(QWidget *pParent)
  : QWidget(pParent), mpProcess(nullptr), mpMCPServer(nullptr), mMCPPort(3000), mBusy(false)
{
  mpInstance = this;
  mpTranscript = new QTextBrowser(this);
  mpTranscript->setOpenExternalLinks(true);
  mpTranscript->setPlaceholderText(tr("Ask the agent to build, fix, convert or simulate a model. It edits the diagram and runs simulations in this OMEdit session and verifies Modelica with the Machina oracle."));
  mpInput = new QPlainTextEdit(this);
  mpInput->setPlaceholderText(tr("e.g. Build a mass-spring-damper with a 2 kg mass, 8 N/m spring, 1 N.s/m damper released from 0.1 m, simulate 10 s and plot the position."));
  mpInput->setMaximumHeight(90);
  mpSendButton = new QPushButton(tr("Send"), this);
  mpSendButton->setDefault(true);
  mpStopButton = new QPushButton(tr("Stop"), this);
  mpStopButton->setEnabled(false);
  mpModelComboBox = new QComboBox(this);
  mpModelComboBox->setToolTip(tr("Model used by the agent (configured in the workspace .machina/config.json; API keys come from environment variables)"));
  mpStatusLabel = new QLabel(tr("Agent not started"), this);
  QHBoxLayout *pTopLayout = new QHBoxLayout;
  pTopLayout->addWidget(new QLabel(tr("Model:"), this));
  pTopLayout->addWidget(mpModelComboBox, 1);
  pTopLayout->addWidget(mpStatusLabel, 2);
  QHBoxLayout *pButtonLayout = new QHBoxLayout;
  pButtonLayout->addStretch();
  pButtonLayout->addWidget(mpStopButton);
  pButtonLayout->addWidget(mpSendButton);
  QVBoxLayout *pLayout = new QVBoxLayout(this);
  pLayout->setContentsMargins(4, 4, 4, 4);
  pLayout->addLayout(pTopLayout);
  pLayout->addWidget(mpTranscript, 1);
  pLayout->addWidget(mpInput);
  pLayout->addLayout(pButtonLayout);
  connect(mpSendButton, SIGNAL(clicked()), SLOT(submit()));
  connect(mpStopButton, SIGNAL(clicked()), SLOT(stop()));
  connect(mpModelComboBox, &QComboBox::currentTextChanged, this, [this](const QString &name) {
    if (!name.isEmpty()) { QSettings *pSettings = Utilities::getApplicationSettings(); pSettings->setValue("aiAgent/model", name); }
  });
}

AgentWidget::~AgentWidget()
{
  stop();
  mpInstance = nullptr;
}

bool AgentWidget::isRunning() const
{
  return mpProcess && mpProcess->state() != QProcess::NotRunning;
}

/*! Starts the MCP server in this OMEdit session if none is running, so the agent can drive the GUI. */
void AgentWidget::ensureMCPServer()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0) && __has_include(<QtHttpServer>)
  QSettings *pSettings = Utilities::getApplicationSettings();
  mMCPPort = pSettings->value("modelContextProtocol/port", 3000).toInt();
  bool startedAtLaunch = pSettings->value("modelContextProtocol/enabled", false).toBool();
  if (!startedAtLaunch && !mpMCPServer) {
    bool enableAdminTools = pSettings->value("modelContextProtocol/enableAdminTools", true).toBool();
    mpMCPServer = new MCPServer(MainWindow::instance()->getOMCProxy(), mMCPPort, enableAdminTools, MainWindow::instance());
    appendHtml(tr("<i>Started the OMEdit MCP server on port %1 for this session.</i>").arg(mMCPPort));
  }
#else
  appendHtml(tr("<b>This OMEdit build has no QtHttpServer (Qt 6.4+ required): the agent can still write and verify Modelica, but cannot edit the diagram or simulate through the GUI.</b>"));
#endif
}

void AgentWidget::start()
{
  if (isRunning()) return;
  ensureMCPServer();
  QSettings *pSettings = Utilities::getApplicationSettings();
  QString node = pSettings->value("aiAgent/nodePath", "node").toString();
  QString kit = pSettings->value("aiAgent/kitPath", "").toString();
  QString workspace = pSettings->value("aiAgent/workspace", QDir::homePath() + "/MachinaWorkspace").toString();
  if (kit.isEmpty() || !QFileInfo(kit + "/src/omedit-agent.mjs").exists()) {
    QMessageBox::warning(this, tr("AI Agent"), tr("Set the path of the Machina Modelica agent kit (the folder containing src/omedit-agent.mjs) in Tools > Options > AI Agent."));
    return;
  }
  QDir().mkpath(workspace);
  mpProcess = new QProcess(this);
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("MACHINA_WORKSPACE", workspace);
  env.insert("OMEDIT_MCP_URL", QString("http://localhost:%1/mcp%2").arg(mMCPPort).arg(pSettings->value("modelContextProtocol/enableAdminTools", true).toBool() ? "/admin" : ""));
  env.insert("MACHINA_MODEL", pSettings->value("aiAgent/model", "").toString());
  mpProcess->setProcessEnvironment(env);
  mpProcess->setWorkingDirectory(workspace);
  connect(mpProcess, SIGNAL(readyReadStandardOutput()), SLOT(readStdout()));
  connect(mpProcess, SIGNAL(readyReadStandardError()), SLOT(readStderr()));
  connect(mpProcess, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(processFinished(int,QProcess::ExitStatus)));
  connect(mpProcess, SIGNAL(errorOccurred(QProcess::ProcessError)), SLOT(processError(QProcess::ProcessError)));
  mpProcess->start(node, QStringList() << kit + "/src/omedit-agent.mjs");
  setStatus(tr("Starting agent…"), true);
}

void AgentWidget::stop()
{
  if (!isRunning()) return;
  mpProcess->kill();
  mpProcess->waitForFinished(2000);
  setStatus(tr("Agent stopped"), false);
}

void AgentWidget::sendTask(const QString &task, const QJsonObject &context)
{
  if (!isRunning()) start();
  if (!isRunning()) return;
  QJsonObject message{{"type", "task"}, {"task", task}, {"model", mpModelComboBox->currentText()}, {"context", context}};
  appendHtml(QString("<p style='color:#1a4f8b'><b>You</b> · %1<br>%2</p>").arg(QDateTime::currentDateTime().toString("hh:mm")).arg(task.toHtmlEscaped().replace("\n", "<br>")));
  mpProcess->write(QJsonDocument(message).toJson(QJsonDocument::Compact) + "\n");
  setStatus(tr("Working…"), true);
}

void AgentWidget::submit()
{
  QString task = mpInput->toPlainText().trimmed();
  if (task.isEmpty()) return;
  mpInput->clear();
  QJsonObject context;
  QString cls = currentClassName();
  if (!cls.isEmpty()) context.insert("activeClass", cls);
  sendTask(task, context);
}

QString AgentWidget::currentClassName() const
{
  ModelWidget *pModelWidget = MainWindow::instance()->getModelWidgetContainer()->getCurrentModelWidget();
  if (pModelWidget && pModelWidget->getLibraryTreeItem()) return pModelWidget->getLibraryTreeItem()->getNameStructure();
  return QString();
}

void AgentWidget::convertCurrentModel()
{
  QString cls = currentClassName();
  if (cls.isEmpty()) { QMessageBox::information(this, tr("AI Agent"), tr("Open a model first.")); return; }
  sendTask(tr("Convert the Modelica class %1 to Machina sentences: read its source with omedit_getSourceCode, convert it with machina_convert (verification on), and when the conversion is VERIFIED replace the class with omedit_setSourceCode. Report what changed and the verification result. Do not replace the class if the conversion was not verified.").arg(cls), QJsonObject{{"activeClass", cls}});
}

void AgentWidget::verifyCurrentModel()
{
  QString cls = currentClassName();
  if (cls.isEmpty()) { QMessageBox::information(this, tr("AI Agent"), tr("Open a model first.")); return; }
  sendTask(tr("Verify the Modelica class %1: read it with omedit_getTotalModel, run machina_check on it, and explain any failure stage and diagnostic with a concrete fix. If it fails, propose the corrected source and, after machina_check passes, apply it with omedit_setSourceCode.").arg(cls), QJsonObject{{"activeClass", cls}});
}

void AgentWidget::explainCurrentModel()
{
  QString cls = currentClassName();
  if (cls.isEmpty()) { QMessageBox::information(this, tr("AI Agent"), tr("Open a model first.")); return; }
  sendTask(tr("Explain the Modelica class %1 in plain language: read it with omedit_getSourceCode and list its components, equations, parameters and what a simulation would show.").arg(cls), QJsonObject{{"activeClass", cls}});
}

void AgentWidget::readStdout()
{
  mStdoutBuffer += mpProcess->readAllStandardOutput();
  int newline;
  while ((newline = mStdoutBuffer.indexOf('\n')) != -1) {
    QByteArray line = mStdoutBuffer.left(newline).trimmed();
    mStdoutBuffer = mStdoutBuffer.mid(newline + 1);
    if (line.isEmpty()) continue;
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(line, &error);
    if (error.error == QJsonParseError::NoError && doc.isObject()) handleEvent(doc.object());
    else appendHtml(QString("<pre style='color:#555'>%1</pre>").arg(QString::fromUtf8(line).toHtmlEscaped()));
  }
}

void AgentWidget::readStderr()
{
  QString text = QString::fromUtf8(mpProcess->readAllStandardError()).trimmed();
  if (!text.isEmpty()) appendHtml(QString("<pre style='color:#8a4b00'>%1</pre>").arg(text.toHtmlEscaped()));
}

void AgentWidget::handleEvent(const QJsonObject &event)
{
  QString type = event.value("type").toString();
  if (type == "ready") {
    setStatus(tr("Agent ready (%1 OMEdit tools, %2 Machina tools)").arg(event.value("omeditTools").toInt()).arg(event.value("machinaTools").toInt()), false);
  } else if (type == "models") {
    QString current = Utilities::getApplicationSettings()->value("aiAgent/model", event.value("default").toString()).toString();
    mpModelComboBox->blockSignals(true);
    mpModelComboBox->clear();
    for (const QJsonValue &m : event.value("models").toArray()) mpModelComboBox->addItem(m.toString());
    int index = mpModelComboBox->findText(current);
    mpModelComboBox->setCurrentIndex(index >= 0 ? index : 0);
    mpModelComboBox->blockSignals(false);
  } else if (type == "step") {
    appendHtml(QString("<p style='color:#444'><b>%1</b> <code>%2</code> — %3<br><span style='color:#777;font-size:90%'>%4</span></p>")
               .arg(event.value("n").toInt()).arg(event.value("tool").toString().toHtmlEscaped()).arg(event.value("thought").toString().toHtmlEscaped())
               .arg(event.value("result").toString().left(400).toHtmlEscaped().replace("\n", "<br>")));
    setStatus(tr("Step %1: %2").arg(event.value("n").toInt()).arg(event.value("tool").toString()), true);
  } else if (type == "final") {
    appendHtml(QString("<p style='color:#0b5d1e'><b>Agent</b><br>%1</p>").arg(event.value("answer").toString().toHtmlEscaped().replace("\n", "<br>")));
    setStatus(tr("Done"), false);
  } else if (type == "error") {
    appendHtml(QString("<p style='color:#a11'><b>Error</b>: %1</p>").arg(event.value("text").toString().toHtmlEscaped()));
    setStatus(tr("Error"), false);
  } else if (type == "status") {
    setStatus(event.value("text").toString(), mBusy);
  }
}

void AgentWidget::processFinished(int exitCode, QProcess::ExitStatus status)
{
  Q_UNUSED(status)
  setStatus(tr("Agent exited (%1)").arg(exitCode), false);
}

void AgentWidget::processError(QProcess::ProcessError error)
{
  Q_UNUSED(error)
  appendHtml(tr("<p style='color:#a11'><b>Could not run the agent process</b>: %1. Check Tools > Options > AI Agent (node path, kit path).</p>").arg(mpProcess ? mpProcess->errorString().toHtmlEscaped() : QString()));
  setStatus(tr("Agent failed to start"), false);
}

void AgentWidget::appendHtml(const QString &html)
{
  mpTranscript->append(html);
}

void AgentWidget::setStatus(const QString &text, bool busy)
{
  mBusy = busy;
  mpStatusLabel->setText(text);
  mpSendButton->setEnabled(!busy);
  mpStopButton->setEnabled(isRunning());
}
