/* This file is part of OpenModelica. Licensed under OSMC-PL 1.2 / GPL v3 like OMEdit. Options page for the AI Agent dock. */
#pragma once
#include <QWidget>
class QLineEdit;
class QCheckBox;
class QSpinBox;
class OptionsDialog;

class AIAgentPage : public QWidget
{
  Q_OBJECT
public:
  AIAgentPage(OptionsDialog *pOptionsDialog);
  QLineEdit* getNodePathTextBox() const { return mpNodePathTextBox; }
  QLineEdit* getKitPathTextBox() const { return mpKitPathTextBox; }
  QLineEdit* getWorkspaceTextBox() const { return mpWorkspaceTextBox; }
  QLineEdit* getModelTextBox() const { return mpModelTextBox; }
  QCheckBox* getMCPEnabledCheckBox() const { return mpMCPEnabledCheckBox; }
  QSpinBox* getMCPPortSpinBox() const { return mpMCPPortSpinBox; }
  QCheckBox* getMCPAdminCheckBox() const { return mpMCPAdminCheckBox; }
private slots:
  void browseKit();
  void browseWorkspace();
private:
  OptionsDialog *mpOptionsDialog;
  QLineEdit *mpNodePathTextBox;
  QLineEdit *mpKitPathTextBox;
  QLineEdit *mpWorkspaceTextBox;
  QLineEdit *mpModelTextBox;
  QCheckBox *mpMCPEnabledCheckBox;
  QSpinBox *mpMCPPortSpinBox;
  QCheckBox *mpMCPAdminCheckBox;
};
