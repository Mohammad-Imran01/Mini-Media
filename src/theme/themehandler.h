#ifndef THEMEHANDLER_H
#define THEMEHANDLER_H

#include <QObject>
#include <QString>
#include <QSet>
#include <QMap>
#include <QComboBox>

class QHBoxLayout;
class QLabel;

class MainWindow;
namespace Ui {
class MainWindow;
}

class ThemeHandler : public QObject
{
    Q_OBJECT
public:
    explicit ThemeHandler(MainWindow* mainWindow, QObject* parent = nullptr);

signals:

private:
    void setupUiComponents();
    void setupConnections();
    void readAllThemeStyleSheets();
    void setAppTheme(const QString& themeName);

    MainWindow* m_mainWindow = nullptr;
    Ui::MainWindow *mainUi = nullptr;
    QComboBox* m_themeComboBox = nullptr;
    QLabel *m_labelHelpText = nullptr;
    QHBoxLayout* hLayoutTheme = nullptr;
    QMap<QString, QString> m_themesQSS, m_themePC;
};

#endif // THEMEHANDLER_H
