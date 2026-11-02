#pragma once
#include "controller.h"
#include <QObject>
#include <QDBusContext>
#include <QDBusServiceWatcher>

// Only allowance data already displayed by the app crosses the session bus.
class PanelService : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.usagefloat.UsageFloat.Panel")
public:
    explicit PanelService(Controller *controller, QObject *parent = nullptr);
    bool start(bool demo = false);
public slots:
    QString GetSnapshot() const;
    void Show();
    void Hide();
    void Refresh();
    void SetShowRemaining(bool remaining);
    void SetPanelActive(bool active);
signals:
    void SnapshotChanged(const QString &json);
    void panelActiveChanged(bool active);
private:
    Controller *m_controller;
    QDBusServiceWatcher m_watcher;
    QString m_panelOwner;
};
