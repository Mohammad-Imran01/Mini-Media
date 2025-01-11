#include "themehandler.h"
#include "ui_mainwindow.h"
#include "src/gui/mainwindow.h"

#include <QDir>
#include <QSpacerItem>
#include <QDebug>
#include <QStringList>
#include <QStyle>
#include <QStyleFactory>
#include <QApplication>

ThemeHandler::ThemeHandler(MainWindow* mainWindow, QObject* parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
{
    mainUi = m_mainWindow->getMainUi();

    setupUiComponents();
    readAllThemeStyleSheets();
    setupConnections();

    m_themeComboBox->setCurrentText("WINDOWSVISTA");
}

void ThemeHandler::setupUiComponents()
{
    // Initialize UI components
    m_themeComboBox = new QComboBox(m_mainWindow);
    m_labelHelpText = new QLabel(m_mainWindow);

    // Configure layouts
    hLayoutTheme = new QHBoxLayout(mainUi->mainTabWidget);
    hLayoutTheme->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding));
    hLayoutTheme->addWidget(m_themeComboBox);
    hLayoutTheme->addWidget(m_labelHelpText);

    // Set alignments
    hLayoutTheme->setAlignment(m_themeComboBox, Qt::AlignRight | Qt::AlignTop);
    hLayoutTheme->setAlignment(m_labelHelpText, Qt::AlignRight | Qt::AlignTop);

    // Configure combo box
    m_themeComboBox->setFocusPolicy(Qt::NoFocus);

    // Configure help text label
    m_labelHelpText->setPixmap(QPixmap(":/resource/markQuestion.svg"));
    m_labelHelpText->setFixedSize(16, 16);
    m_labelHelpText->setScaledContents(true);
    m_labelHelpText->setToolTip("About the Application");

    // Adjust layout margins
    hLayoutTheme->setContentsMargins(0, 0, 10, 10);
}

void ThemeHandler::readAllThemeStyleSheets()
{
    // Directory containing the QSS files
    const QString qssDirectory = ":/resource/qss/";
    QDir dir(qssDirectory);

    // Retrieve QSS files
    QStringList qssFiles = dir.entryList({"*.qss"}, QDir::Files);

    for (const QString& fileName : qssFiles) {
        QString filePath = qssDirectory + fileName;
        QFile qssFile(filePath);

        if (qssFile.open(QFile::ReadOnly)) {
            QString qssContent = QLatin1String(qssFile.readAll());
            QString themeName = fileName.split('.').first().toUpper();
            m_themesQSS.insert(themeName, qssContent);
            qssFile.close();
        } else {
            qDebug() << "Failed to open QSS file:" << filePath;
        }
    }

    // built-in themes
    auto defThemes = QStyleFactory::keys();
    for(auto key: defThemes)
        m_themePC.insert(key.toUpper(), key);
}


void ThemeHandler::setupConnections()
{
    QStringList sortedThemes = m_themesQSS.keys() + m_themePC.keys();
    sortedThemes.sort();

    m_themeComboBox->addItems(sortedThemes);  // Add both default and custom themes

    connect(m_themeComboBox, &QComboBox::currentTextChanged, this, &ThemeHandler::setAppTheme);
}

void ThemeHandler::setAppTheme(const QString& themeName)
{
    if (m_themePC.contains(themeName)) {  // Check if it's a default theme
        QStyle* style = QStyleFactory::create(m_themePC.value(themeName));
        if (style) {
            qApp->setStyleSheet(""); // Clear any existing QSS
            QApplication::setStyle(style); // Apply default Qt theme
            qDebug() << "Applied default Qt theme:" << themeName;
        } else {
            qDebug() << "Failed to apply Qt style for:" << themeName;
        }
    } // Handle custom QSS themes
    else if (m_themesQSS.contains(themeName)) {
        QString themeStyleSheet = m_themesQSS.value(themeName);
        qApp->setStyleSheet(themeStyleSheet); // Apply custom QSS theme
        qDebug() << "Applied custom theme:" << themeName;
    }
    else {
        qDebug() << "Theme not found:" << themeName;
        return;
    }
}


