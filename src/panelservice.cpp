#include "panelservice.h"
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QJsonDocument>
#include <QJsonObject>

PanelService::PanelService(Controller *controller, QObject *parent)
    : QObject(parent), m_controller(controller) {
    const auto publish = [this] { emit SnapshotChanged(GetSnapshot()); };
    connect(controller, &Controller::statesChanged, this, publish);
    connect(controller, &Controller::preferencesChanged, this, publish);
    m_watcher.setConnection(QDBusConnection::sessionBus());
    m_watcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(&m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &owner) {
        if (owner != m_panelOwner) return;
        m_watcher.removeWatchedService(owner);
        m_panelOwner.clear();
        emit panelActiveChanged(false);
    });
}

bool PanelService::start(bool demo) {
    auto bus = QDBusConnection::sessionBus();
    const QString service = demo ? "io.usagefloat.UsageFloat.Panel.Demo" : "io.usagefloat.UsageFloat.Panel";
    if (!bus.registerObject("/io/usagefloat/UsageFloat/Panel", this,
                            QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals)) return false;
    if (bus.registerService(service)) return true;
    bus.unregisterObject("/io/usagefloat/UsageFloat/Panel");
    return false;
}

QString PanelService::GetSnapshot() const {
    return QString::fromUtf8(QJsonDocument(QJsonObject{
        {"version", QCoreApplication::applicationVersion()},
        {"claude", QJsonObject::fromVariantMap(m_controller->claude())},
        {"codex", QJsonObject::fromVariantMap(m_controller->codex())},
        {"showRemaining", m_controller->showRemaining()},
        {"status", m_controller->statusText()}
    }).toJson(QJsonDocument::Compact));
}
void PanelService::Show() { m_controller->showWindow(); }
void PanelService::Hide() { m_controller->hideWindow(); }
void PanelService::Refresh() { m_controller->refresh(); }
void PanelService::SetShowRemaining(bool remaining) { m_controller->setShowRemaining(remaining); }
void PanelService::SetPanelActive(bool active) {
    if (!calledFromDBus()) return;
    const QString owner = message().service();
    if (active) {
        if (owner == m_panelOwner) return;
        if (!m_panelOwner.isEmpty()) m_watcher.removeWatchedService(m_panelOwner);
        m_panelOwner = owner;
        m_watcher.addWatchedService(owner);
        emit panelActiveChanged(true);
    } else if (owner == m_panelOwner) {
        m_watcher.removeWatchedService(owner);
        m_panelOwner.clear();
        emit panelActiveChanged(false);
    }
}
