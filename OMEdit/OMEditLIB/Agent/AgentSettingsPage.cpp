/* This file is part of OpenModelica. Licensed under OSMC-PL 1.2 / GPL v3 like OMEdit. */
#include "Agent/AgentSettingsPage.h"
#include "Options/OptionsDialog.h"
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QFileDialog>

AIAgentPage::AIAgentPage(OptionsDialog *pOptionsDialog)
  : QWidget(pOptionsDialog), mpOptionsDialog(pOptionsDialog)
{
  QGroupBox *pAgentGroupBox = new QGroupBox(tr("Agent process"));
  mpNodePathTextBox = new QLineEdit("node");
  mpKitPathTextBox = new QLineEdit;
  mpKitPathTextBox->setPlaceholderText(tr("folder of the Machina Modelica agent kit (contains src/omedit-agent.mjs)"));
  QPushButton *pKitBrowseButton = new QPushButton(tr("Browse…"));
  connect(pKitBrowseButton, SIGNAL(clicked()), SLOT(browseKit()));
  mpWorkspaceTextBox = new QLineEdit;
  mpWorkspaceTextBox->setPlaceholderText(tr("workspace folder: Machina/Machina.mo, .machina/config.json (models), files the agent writes"));
  QPushButton *pWorkspaceBrowseButton = new QPushButton(tr("Browse…"));
  connect(pWorkspaceBrowseButton, SIGNAL(clicked()), SLOT(browseWorkspace()));
  mpModelTextBox = new QLineEdit;
  mpModelTextBox->setPlaceholderText(tr("model name from .machina/config.json (e.g. azure-astra); API keys are read from environment variables"));
  QGridLayout *pAgentLayout = new QGridLayout;
  pAgentLayout->addWidget(new QLabel(tr("Node executable:")), 0, 0);
  pAgentLayout->addWidget(mpNodePathTextBox, 0, 1, 1, 2);
  pAgentLayout->addWidget(new QLabel(tr("Agent kit folder:")), 1, 0);
  pAgentLayout->addWidget(mpKitPathTextBox, 1, 1);
  pAgentLayout->addWidget(pKitBrowseButton, 1, 2);
  pAgentLayout->addWidget(new QLabel(tr("Workspace:")), 2, 0);
  pAgentLayout->addWidget(mpWorkspaceTextBox, 2, 1);
  pAgentLayout->addWidget(pWorkspaceBrowseButton, 2, 2);
  pAgentLayout->addWidget(new QLabel(tr("Default model:")), 3, 0);
  pAgentLayout->addWidget(mpModelTextBox, 3, 1, 1, 2);
  pAgentGroupBox->setLayout(pAgentLayout);

  QGroupBox *pMCPGroupBox = new QGroupBox(tr("OMEdit MCP server (the agent's GUI tools)"));
  mpMCPEnabledCheckBox = new QCheckBox(tr("Start the MCP server when OMEdit starts (otherwise the agent starts it on demand)"));
  mpMCPPortSpinBox = new QSpinBox;
  mpMCPPortSpinBox->setRange(1024, 65535);
  mpMCPPortSpinBox->setValue(3000);
  mpMCPAdminCheckBox = new QCheckBox(tr("Enable admin tools (loadFile, loadModel, result paths) for the agent"));
  QGridLayout *pMCPLayout = new QGridLayout;
  pMCPLayout->addWidget(mpMCPEnabledCheckBox, 0, 0, 1, 2);
  pMCPLayout->addWidget(new QLabel(tr("Port:")), 1, 0);
  pMCPLayout->addWidget(mpMCPPortSpinBox, 1, 1);
  pMCPLayout->addWidget(mpMCPAdminCheckBox, 2, 0, 1, 2);
  pMCPGroupBox->setLayout(pMCPLayout);

  QVBoxLayout *pLayout = new QVBoxLayout;
  pLayout->setAlignment(Qt::AlignTop);
  pLayout->addWidget(pAgentGroupBox);
  pLayout->addWidget(pMCPGroupBox);
  setLayout(pLayout);
}

void AIAgentPage::browseKit()
{
  QString dir = QFileDialog::getExistingDirectory(this, tr("Machina Modelica agent kit folder"), mpKitPathTextBox->text());
  if (!dir.isEmpty()) mpKitPathTextBox->setText(dir);
}

void AIAgentPage::browseWorkspace()
{
  QString dir = QFileDialog::getExistingDirectory(this, tr("Agent workspace folder"), mpWorkspaceTextBox->text());
  if (!dir.isEmpty()) mpWorkspaceTextBox->setText(dir);
}
