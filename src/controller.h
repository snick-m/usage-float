#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <QVariantMap>
#include <QWindow>

class Controller : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap claude READ claude NOTIFY statesChanged)
    Q_PROPERTY(QVariantMap codex READ codex NOTIFY statesChanged)
    Q_PROPERTY(bool refreshing READ refreshing NOTIFY statesChanged)
    Q_PROPERTY(bool compact READ compact WRITE setCompact NOTIFY preferencesChanged)
    Q_PROPERTY(bool wide READ wide WRITE setWide NOTIFY preferencesChanged)
    Q_PROPERTY(bool showRemaining READ showRemaining WRITE setShowRemaining NOTIFY preferencesChanged)
    Q_PROPERTY(bool borderless READ borderless WRITE setBorderless NOTIFY preferencesChanged)
    Q_PROPERTY(bool pinned READ pinned WRITE setPinned NOTIFY preferencesChanged)
    Q_PROPERTY(double panelOpacity READ panelOpacity WRITE setPanelOpacity NOTIFY preferencesChanged)
    Q_PROPERTY(int intervalMinutes READ intervalMinutes WRITE setIntervalMinutes NOTIFY preferencesChanged)
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY preferencesChanged)
    Q_PROPERTY(QString codexPath READ codexPath WRITE setCodexPath NOTIFY preferencesChanged)
    Q_PROPERTY(bool wayland READ wayland CONSTANT)
    Q_PROPERTY(int availableScreenHeight READ availableScreenHeight NOTIFY screenGeometryChanged)
    Q_PROPERTY(QString notice READ notice NOTIFY noticeChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statesChanged)
public:
    explicit Controller(bool demo, QObject *parent = nullptr);
    ~Controller() override;
    QVariantMap claude() const;
    QVariantMap codex() const;
    bool refreshing() const { return m_claudeBusy || m_codexBusy; }
    bool compact() const { return m_settings.value("compact", true).toBool(); }
    bool wide() const { return m_settings.value("wide", false).toBool(); }
    bool showRemaining() const { return m_settings.value("showRemaining", false).toBool(); }
    bool borderless() const { return m_settings.value("borderless", true).toBool(); }
    bool pinned() const { return m_settings.value("pinned", true).toBool(); }
    double panelOpacity() const { return m_settings.value("opacity", 1.0).toDouble(); }
    int intervalMinutes() const { return m_settings.value("intervalMinutes", 3).toInt(); }
    bool autoStart() const { return m_settings.value("autoStart", false).toBool(); }
    QString codexPath() const { return m_settings.value("codexPath").toString(); }
    bool wayland() const;
    int availableScreenHeight() const;
    QString notice() const { return m_notice; }
    QString statusText() const;
    void setCompact(bool value);
    void setWide(bool value);
    void setShowRemaining(bool value);
    void setBorderless(bool value);
    void setPinned(bool value);
    void setPanelOpacity(double value);
    void setIntervalMinutes(int value);
    void setAutoStart(bool value);
    void setCodexPath(const QString &value);
    void attachWindow(QWindow *window);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void moveWindow();
    Q_INVOKABLE void resizeWindow();
    Q_INVOKABLE void openUsage(const QString &provider);
    Q_INVOKABLE void copyLoginCommand(const QString &provider);
    Q_INVOKABLE QString resetText(qint64 seconds) const;
    Q_INVOKABLE void showWindow();
    Q_INVOKABLE void hideWindow();
    Q_INVOKABLE void quit();
signals:
    void statesChanged();
    void preferencesChanged();
    void noticeChanged();
    void screenGeometryChanged();
private:
    void fetchClaude();
    void requestClaude(const QJsonObject &auth);
    void fetchCodex();
    void readCodex();
    void sendRpc(const QJsonObject &message);
    void success(const QString &provider, const QVariantList &rows, const QString &plan);
    void failure(const QString &provider, const QString &message);
    void setNotice(const QString &message);
    QVariantMap decorate(const QVariantMap &state) const;
    QSettings m_settings;
    QNetworkAccessManager m_network;
    QTimer m_poll, m_clock, m_codexDeadline;
    QProcess m_codexProcess;
    QVariantMap m_claude, m_codex;
    QWindow *m_window = nullptr;
    bool m_demo = false, m_claudeBusy = false, m_codexBusy = false;
    qint64 m_claudeNextAttempt = 0;
    int m_claudeFailures = 0;
    QString m_notice;
};
