#include "TraceabilityQueryDialog.h"
#include "MainWindow.h"
#include "Util/Helper.h"
#include "Modeling/ModelWidgetContainer.h"
#include "Options/OptionsDialog.h"
#include "Git/GitCommands.h"


/*!
 * \class TraceabilityQueryDialog
 * \brief Creates a dialog that query and visualize the traceability information.
 */
/*!
 * \brief TraceabilityQueryDialog::TraceabilityQueryDialog
 * \param pMainWindow - pointer to MainWindow
 */
TraceabilityQueryDialog::TraceabilityQueryDialog(QWidget *pParent)
  : QDialog(pParent)
{
  setWindowTitle(QString(Helper::applicationName).append(" - ").append(tr("Query Traceability Information")));
  setAttribute(Qt::WA_DeleteOnClose);
  resize(600, 400);
  //
  mpNodeToQueryLabel = new Label(tr("Node to Query:"));
  mpNodeToQueryComboBox = new QComboBox;
  //
  QGroupBox *pQueryTraceGroupBox = new QGroupBox(tr("Query Trace Type"));
  mpTraceToRadioButton = new QRadioButton(tr("Trace To "));
  mpTraceFromRadioButton = new QRadioButton(tr("Trace From "));
  mpTraceToRadioButton->setChecked(true);
  QHBoxLayout *pQueryTraceHBoxLayout = new QHBoxLayout;
  pQueryTraceHBoxLayout->addWidget(mpTraceToRadioButton);
  pQueryTraceHBoxLayout->addWidget(mpTraceFromRadioButton);
//  pQueryTraceHBoxLayout->addStretch(1);
  pQueryTraceGroupBox->setLayout(pQueryTraceHBoxLayout);
  // Traceability information
  QGroupBox *pTraceabilityInformationGroupBox = new QGroupBox(tr("Traceability Information:"));
  mpTraceabilityInformationTextBox = new QPlainTextEdit;
  mpTraceabilityGraphWebView = new QWebView;
  mpTraceabilityInformationTextBox->setLineWrapMode(QPlainTextEdit::NoWrap);
  mpTraceabilityInformationTextBox->setReadOnly(true);
  QGridLayout *pTraceabilityInformationLayout = new QGridLayout;
//  pTraceabilityInformationLayout->addWidget(mpTraceabilityInformationTextBox);
  pTraceabilityInformationLayout->addWidget(mpTraceabilityGraphWebView);
  pTraceabilityInformationGroupBox->setLayout(pTraceabilityInformationLayout);
  mpTraceabilityGraphWebView->load(QUrl("http://localhost:7474/browser/"));
  // Create the buttons
  mpQueryTraceabilitytButton = new QPushButton(tr("Query"));
  mpQueryTraceabilitytButton->setEnabled(true);
  connect(mpQueryTraceabilitytButton, SIGNAL(clicked()), SLOT(queryTraceabilityInformation()));
  mpCancelButton = new QPushButton(Helper::cancel);
  connect(mpCancelButton, SIGNAL(clicked()), SLOT(reject()));
  // create buttons box
  mpButtonBox = new QDialogButtonBox(Qt::Horizontal);
  mpButtonBox->addButton(mpQueryTraceabilitytButton, QDialogButtonBox::ActionRole);
  mpButtonBox->addButton(mpCancelButton, QDialogButtonBox::ActionRole);
  // set the layout
  QGridLayout *pMainLayout = new QGridLayout;
  pMainLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pMainLayout->addWidget(mpNodeToQueryLabel, 0, 0);
  pMainLayout->addWidget(mpNodeToQueryComboBox, 0, 1);
  pMainLayout->addWidget(pQueryTraceGroupBox, 1, 0, 1, 2);
  pMainLayout->addWidget(pTraceabilityInformationGroupBox, 2, 0, 2, 2);
  pMainLayout->addWidget(mpButtonBox, 4, 1,  Qt::AlignRight);
  setLayout(pMainLayout);
  translateURIToJsonMessageFormat();
}

void TraceabilityQueryDialog::translateURIToJsonMessageFormat()
{
  QString filePath = MainWindow::instance()->getModelWidgetContainer()->getCurrentModelWidget()->getLibraryTreeItem()->getFileName();
  QString nameStructure = MainWindow::instance()->getModelWidgetContainer()->getCurrentModelWidget()->getLibraryTreeItem()->getNameStructure();
  QFileInfo info(filePath);

  QFile URIFile(info.absolutePath() + "/" + nameStructure +".md");

  QString fileNameURI, activityURI, agentURI, toolURI;
  QString Test;
  QStringList URIList;
  URIFile.open(QIODevice::ReadOnly | QIODevice::Text);
  Test = URIFile.readAll();
  URIList = Test.split(',');
  URIFile.close();
  for (int i = 0; i < URIList.size(); ++i){
      fileNameURI = URIList.at(0);
      activityURI = URIList.at(1);
      agentURI = URIList.at(2);
      toolURI =URIList.at(3);
    }
  mpNodeToQueryComboBox->addItem(fileNameURI.simplified());
  mpNodeToQueryComboBox->addItem(activityURI.simplified());
  mpNodeToQueryComboBox->addItem(agentURI.simplified());
  mpNodeToQueryComboBox->addItem(toolURI.simplified());

}
/*!
 * \brief TraceabilityQueryDialog::sendTraceabilityInformation
 * Slot activated when mpPushTraceabilitytButton clicked signal is raised.\n
 * Sends the traceability information.
 */
void TraceabilityQueryDialog::queryTraceabilityInformation()
{
  // create the request
  QString ipAdress = OptionsDialog::instance()->getTraceabilityPage()->getTraceabilityDaemonIpAdress()->text();
  QString port = OptionsDialog::instance()->getTraceabilityPage()->getTraceabilityDaemonPort()->text();
  QUrl url;
  if(mpTraceToRadioButton->isChecked())
    url = ("http://"+ ipAdress +":"+ port +"/traces/to/"+ mpNodeToQueryComboBox->currentText() + "/json");
  else
    url = ("http://"+ ipAdress +":"+ port +"/traces/from/"+ mpNodeToQueryComboBox->currentText() + "/json");
  QNetworkRequest networkRequest(url);
  networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json" );
  networkRequest.setRawHeader( "Accept-Charset", "UTF-8");
  QNetworkAccessManager *pNetworkAccessManager = new QNetworkAccessManager;
  QNetworkReply *pNetworkReply = pNetworkAccessManager->get(networkRequest);
  pNetworkReply->ignoreSslErrors();
  connect(pNetworkAccessManager, SIGNAL(finished(QNetworkReply*)), SLOT(readTraceabilityInformation(QNetworkReply*)));
}

/*!
 * \brief TraceabilityQueryDialog::readTraceabilityInformation
 * \param pNetworkReply
 * Slot activated when QNetworkAccessManager finished signal is raised.\n
 * Displays the traceability information or Show an error message if the traceability information not found.\n
 * Deletes QNetworkReply object .
 */
void TraceabilityQueryDialog::readTraceabilityInformation(QNetworkReply *pNetworkReply)
{
  QString strReply = (QString)pNetworkReply->readAll();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
  QJsonDocument doc = QJsonDocument::fromJson(strReply.toUtf8());
  QString formattedJsonString = doc.toJson(QJsonDocument::Indented);
  mpTraceabilityInformationTextBox->setPlainText(formattedJsonString);
  if (pNetworkReply->error() != QNetworkReply::NoError) {
    QMessageBox::critical(0, QString(Helper::applicationName).append(" - ").append(Helper::error),
                                     QString("Following error has occurred while querying the traceability information \n\n%1").arg(pNetworkReply->errorString()),
                                     Helper::ok);
  }
  else
    pNetworkReply->deleteLater();
#else // Qt4
#endif
//  accept();
}
