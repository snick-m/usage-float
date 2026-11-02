#include "controller.h"
#include "usage.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSaveFile>
#include <QScreen>
#include <QStandardPaths>
#include <QUrl>
#include <algorithm>

namespace {
QVariantMap initial(const QString &source) {
    return {{"rows", QVariantList{}}, {"status", "loading"}, {"message", "Reading your existing login…"},
            {"plan", ""}, {"model", "Auto"}, {"source", source}, {"updated", qint64(0)}, {"busy", false}};
}
QString claudeModel() {
    QString model = qEnvironmentVariable("CLAUDE_MODEL");
    if (model.isEmpty()) {
        const QString folder = qEnvironmentVariable("CLAUDE_CONFIG_DIR", QDir::homePath() + "/.claude");
        QFile file(folder + "/settings.json");
        if (file.open(QIODevice::ReadOnly))
            model = QJsonDocument::fromJson(file.read(1024 * 1024)).object().value("model").toString();
    }
    return model.trimmed().isEmpty() ? QString("Auto") : model.trimmed().left(128);
}
QString executable(const QString &name) {
    QStringList paths = QProcessEnvironment::systemEnvironment().value("PATH").split(QDir::listSeparator());
    for (const QString &path : {"/.local/bin", "/.npm-global/bin", "/.bun/bin"}) paths.append(QDir::homePath() + path);
    paths << "/usr/local/bin" << "/opt/homebrew/bin" << "/usr/bin";
    return QStandardPaths::findExecutable(name, paths);
}
QString quotedDesktop(const QString &value) {
    QString s = value;
    s.replace("\\", "\\\\").replace("\"", "\\\"").replace("`", "\\`").replace("$", "\\$").replace("%", "%%");
    return '"' + s + '"';
}
}

Controller::Controller(bool demo, QObject *parent) : QObject(parent), m_demo(demo) {
    m_claude = initial("Claude Code login");
    m_codex = initial("Codex app-server");
    if (!demo) m_claude.insert("model", claudeModel());
    m_poll.setInterval(intervalMinutes() * 60000);
    connect(&m_poll, &QTimer::timeout, this, &Controller::refresh);
    m_poll.start();
    m_clock.setInterval(15000);
    connect(&m_clock, &QTimer::timeout, this, [this] { emit statesChanged(); });
    m_clock.start();
    m_codexDeadline.setSingleShot(true);
    m_codexDeadline.setInterval(25000);
    connect(&m_codexDeadline, &QTimer::timeout, this, [this] {
        if (!m_codexBusy) return;
        failure("codex", "Codex took too long to reply. Check the CLI and try again.");
        m_codexProcess.kill();
    });
    connect(&m_codexProcess, &QProcess::readyReadStandardOutput, this, &Controller::readCodex);
    connect(&m_codexProcess, &QProcess::readyReadStandardError, this, [this] {
        // Diagnostics from child tools may contain account data. Never display or log them.
        m_codexProcess.readAllStandardError();
    });
    connect(&m_codexProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (m_codexBusy && error == QProcess::FailedToStart)
            failure("codex", "Could not launch Codex. Set its executable path in Settings.");
    });
    connect(&m_codexProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this](int, QProcess::ExitStatus) {
            if (m_codexBusy) failure("codex", "Codex exited before returning usage. Run codex login if needed.");
        });
    connect(&m_codexProcess, &QProcess::started, this, [this] {
        sendRpc({{"id", 1}, {"method", "initialize"}, {"params", QJsonObject{
            {"clientInfo", QJsonObject{{"name", "usage_float"}, {"title", "Usage Float"}, {"version", QCoreApplication::applicationVersion()}}}}}});
    });
    QTimer::singleShot(0, this, &Controller::refresh);
}

Controller::~Controller() {
    if (m_codexProcess.state() != QProcess::NotRunning) {
        m_codexProcess.kill();
        m_codexProcess.waitForFinished(1000);
    }
    for (auto *process : findChildren<QProcess *>()) {
        if (process->state() == QProcess::NotRunning) continue;
        process->kill();
        process->waitForFinished(1000);
    }
}

bool Controller::wayland() const { return QGuiApplication::platformName().startsWith("wayland"); }
int Controller::availableScreenHeight() const {
    auto *screen = m_window ? m_window->screen() : QGuiApplication::primaryScreen();
    return screen ? screen->availableGeometry().height() : 800;
}
QVariantMap Controller::decorate(const QVariantMap &input) const {
    auto state = input;
    auto rows = state.value("rows").toList();
    for (auto &value : rows) {
        auto row = value.toMap();
        row.insert("displayPercent", showRemaining() ? 100.0 - row.value("percent").toDouble() : row.value("percent").toDouble());
        const qint64 reset = row.value("reset").toLongLong();
        row.insert("countdown", resetText(reset));
        row.insert("resetDate", reset > 0 ? QDateTime::fromSecsSinceEpoch(reset).toLocalTime().toString("ddd, MMM d · h:mm AP") : "");
        value = row;
    }
    state.insert("rows", rows);
    state.insert("hasData", !rows.isEmpty());
    const qint64 updated = state.value("updated").toLongLong();
    const qint64 age = QDateTime::currentSecsSinceEpoch() - updated;
    const bool stale = updated > 0 && (state.value("status") == "error" || age > intervalMinutes() * 120 + 30);
    state.insert("stale", stale);
    state.insert("age", updated == 0 ? "Not synced" : age < 60 ? "Just now" : QString("%1m ago").arg(age / 60));
    return state;
}
QVariantMap Controller::claude() const { return decorate(m_claude); }
QVariantMap Controller::codex() const { return decorate(m_codex); }
QString Controller::statusText() const {
    if (m_demo) return "Demo data · no accounts accessed";
    if (refreshing()) return "Syncing your allowances…";
    if (m_claude.value("status") == "error" || m_codex.value("status") == "error") return "Some data needs attention";
    return QString(showRemaining() ? "Remaining allowance · every " : "Used allowance · every ") + QString::number(intervalMinutes()) + " min";
}
QString Controller::resetText(qint64 seconds) const { return Usage::countdown(seconds, QDateTime::currentSecsSinceEpoch()); }

void Controller::setCompact(bool v) { m_settings.setValue("compact", v); emit preferencesChanged(); }
void Controller::setWide(bool v) { m_settings.setValue("wide", v); emit preferencesChanged(); }
void Controller::setShowRemaining(bool v) {
    m_settings.setValue("showRemaining", v);
    emit preferencesChanged();
    emit statesChanged();
}
void Controller::setBorderless(bool v) { m_settings.setValue("borderless", v); emit preferencesChanged(); }
void Controller::setPinned(bool v) { m_settings.setValue("pinned", v); emit preferencesChanged(); }
void Controller::setPanelOpacity(double v) { m_settings.setValue("opacity", qBound(0.45, v, 1.0)); emit preferencesChanged(); }
void Controller::setIntervalMinutes(int v) {
    m_settings.setValue("intervalMinutes", qBound(1, v, 30));
    m_poll.setInterval(intervalMinutes() * 60000);
    emit preferencesChanged(); emit statesChanged();
}
void Controller::setCodexPath(const QString &v) { m_settings.setValue("codexPath", v.trimmed()); emit preferencesChanged(); }
void Controller::setNotice(const QString &v) { m_notice = v; emit noticeChanged(); }

void Controller::setAutoStart(bool enabled) {
    const QString program = QCoreApplication::applicationFilePath();
    bool ok = false;
#ifdef Q_OS_LINUX
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    const QString path = dir + "/io.usagefloat.UsageFloat.desktop";
    if (enabled) {
        QDir().mkpath(dir);
        QSaveFile file(path);
        const auto data = ("[Desktop Entry]\nType=Application\nName=Usage Float\nExec=" + quotedDesktop(program)
            + "\nIcon=io.usagefloat.UsageFloat\nTerminal=false\nX-GNOME-Autostart-enabled=true\n").toUtf8();
        ok = file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit();
    } else ok = !QFile::exists(path) || QFile::remove(path);
#elif defined(Q_OS_WIN)
    QSettings run("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (enabled) run.setValue("UsageFloat", '"' + QDir::toNativeSeparators(program) + '"');
    else run.remove("UsageFloat");
    run.sync(); ok = run.status() == QSettings::NoError;
#elif defined(Q_OS_MACOS)
    const QString dir = QDir::homePath() + "/Library/LaunchAgents";
    const QString path = dir + "/io.usagefloat.UsageFloat.plist";
    if (enabled) {
        QDir().mkpath(dir);
        QString escaped = program.toHtmlEscaped();
        const auto data = ("<?xml version=\"1.0\" encoding=\"UTF-8\"?><plist version=\"1.0\"><dict>"
            "<key>Label</key><string>io.usagefloat.UsageFloat</string><key>ProgramArguments</key><array><string>" + escaped
            + "</string></array><key>RunAtLoad</key><true/></dict></plist>").toUtf8();
        QSaveFile file(path);
        ok = file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit();
    } else ok = !QFile::exists(path) || QFile::remove(path);
#endif
    if (ok) { m_settings.setValue("autoStart", enabled); emit preferencesChanged(); }
    else setNotice("Could not update startup settings. Check folder permissions.");
}

void Controller::attachWindow(QWindow *window) {
    m_window = window;
    connect(window, &QWindow::screenChanged, this, &Controller::screenGeometryChanged);
    for (auto *screen : QGuiApplication::screens())
        connect(screen, &QScreen::availableGeometryChanged, this, &Controller::screenGeometryChanged);
    connect(qGuiApp, &QGuiApplication::screenAdded, this, [this](QScreen *screen) {
        connect(screen, &QScreen::availableGeometryChanged, this, &Controller::screenGeometryChanged);
        emit screenGeometryChanged();
    });
    emit screenGeometryChanged();
    // Wayland owns positioning. Restore coordinates only on platforms that allow it.
    if (!wayland() && m_settings.contains("position")) {
        const QPoint pos = m_settings.value("position").toPoint();
        for (auto *screen : QGuiApplication::screens()) {
            if (screen->availableGeometry().contains(pos)) { window->setPosition(pos); break; }
        }
    }
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this] {
        if (m_window && !wayland()) m_settings.setValue("position", m_window->position());
        if (m_window && !compact()) m_settings.setValue("width", m_window->width());
        m_settings.sync();
    });
    if (!compact()) m_window->setWidth(qBound(280, m_settings.value("width", 328).toInt(), 560));
}
void Controller::moveWindow() { if (m_window) m_window->startSystemMove(); }
void Controller::resizeWindow() { if (m_window) m_window->startSystemResize(Qt::RightEdge); }
void Controller::showWindow() { if (m_window) { m_window->showNormal(); m_window->raise(); m_window->requestActivate(); } }
void Controller::hideWindow() { if (m_window) m_window->hide(); }
void Controller::quit() { QCoreApplication::quit(); }
void Controller::openUsage(const QString &provider) {
    QDesktopServices::openUrl(QUrl(provider == "claude" ? "https://claude.ai/settings/usage" : "https://chatgpt.com/codex/settings/usage"));
}
void Controller::copyLoginCommand(const QString &provider) {
    const QString command = provider == "claude" ? "claude auth login" : "codex login";
    QGuiApplication::clipboard()->setText(command);
    setNotice("Copied " + command + " — run it in your terminal, then refresh.");
}

void Controller::refresh() {
    if (m_demo) {
        m_claude.insert("model", "Sonnet");
        m_codex.insert("model", "gpt-6.1-sol");
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        success("claude", {QVariantMap{{"label", "Session"}, {"percent", 32.0}, {"reset", now + 9480}},
                           QVariantMap{{"label", "Weekly"}, {"percent", 58.0}, {"reset", now + 260000}}}, "Max");
        success("codex", {QVariantMap{{"label", "Session"}, {"percent", 16.0}, {"reset", now + 14400}},
                          QVariantMap{{"label", "Weekly"}, {"percent", 42.0}, {"reset", now + 390000}}}, "Plus");
        return;
    }
    if (!m_claudeBusy && QDateTime::currentSecsSinceEpoch() >= m_claudeNextAttempt) fetchClaude();
    if (!m_codexBusy && m_codexProcess.state() == QProcess::NotRunning) fetchCodex();
}

void Controller::success(const QString &provider, const QVariantList &rows, const QString &plan) {
    if (rows.isEmpty()) { failure(provider, "No quota windows returned. Your plan may not expose usage right now."); return; }
    auto &state = provider == "claude" ? m_claude : m_codex;
    state.insert("rows", rows); state.insert("plan", plan);
    state.insert("status", "ready"); state.insert("message", ""); state.insert("busy", false);
    state.insert("updated", QDateTime::currentSecsSinceEpoch());
    if (provider == "claude") { m_claudeBusy = false; m_claudeFailures = 0; }
    else { m_codexBusy = false; m_codexDeadline.stop(); }
    emit statesChanged();
}
void Controller::failure(const QString &provider, const QString &message) {
    auto &state = provider == "claude" ? m_claude : m_codex;
    state.insert("status", "error"); state.insert("message", message); state.insert("busy", false);
    if (provider == "claude") {
        m_claudeBusy = false;
        ++m_claudeFailures;
        m_claudeNextAttempt = qMax(m_claudeNextAttempt, QDateTime::currentSecsSinceEpoch() + qMin(1800, 30 * (1 << qMin(m_claudeFailures, 5))));
    } else { m_codexBusy = false; m_codexDeadline.stop(); }
    emit statesChanged();
}

void Controller::fetchClaude() {
    m_claude.insert("model", claudeModel());
    m_claudeBusy = true; m_claude.insert("busy", true); emit statesChanged();
    const QString dir = qEnvironmentVariable("CLAUDE_CONFIG_DIR", QDir::homePath() + "/.claude");
    QFile file(dir + "/.credentials.json");
    if (file.open(QIODevice::ReadOnly)) {
        const auto data = QJsonDocument::fromJson(file.read(1024 * 1024)).object();
        requestClaude(data.value("claudeAiOauth").toObject());
        return;
    }
#ifdef Q_OS_MACOS
    auto *keychain = new QProcess(this);
    auto *deadline = new QTimer(keychain);
    deadline->setSingleShot(true);
    connect(deadline, &QTimer::timeout, keychain, &QProcess::kill);
    connect(keychain, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this, keychain](int code, QProcess::ExitStatus) {
            if (code == 0) requestClaude(QJsonDocument::fromJson(keychain->readAllStandardOutput()).object().value("claudeAiOauth").toObject());
            else failure("claude", "Claude Code login not found. Run claude auth login, then refresh.");
            keychain->deleteLater();
        });
    connect(keychain, &QProcess::errorOccurred, this, [this, keychain](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) { failure("claude", "Could not access Claude Code's Keychain login."); keychain->deleteLater(); }
    });
    keychain->start("/usr/bin/security", {"find-generic-password", "-s", "Claude Code-credentials", "-w"});
    deadline->start(15000);
#else
    failure("claude", "Claude Code login not found. Run claude auth login, then refresh.");
#endif
}

void Controller::requestClaude(const QJsonObject &auth) {
    const QString token = auth.value("accessToken").toString();
    if (token.isEmpty()) { failure("claude", "No Claude subscription login found. Run claude auth login."); return; }
    QNetworkRequest request(QUrl("https://api.anthropic.com/api/oauth/usage"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(20000);
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    request.setRawHeader("anthropic-beta", "oauth-2025-04-20");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "usage-float/" + QCoreApplication::applicationVersion().toUtf8());
    auto *reply = m_network.get(request);
    reply->setReadBufferSize(1024 * 1024);
    connect(reply, &QNetworkReply::finished, this, [this, reply, plan = auth.value("subscriptionType").toString()] {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 200 && reply->error() == QNetworkReply::NoError) {
            QJsonParseError error;
            const auto doc = QJsonDocument::fromJson(reply->readAll(), &error);
            if (error.error == QJsonParseError::NoError && doc.isObject()) success("claude", Usage::claudeRows(doc.object()), plan);
            else failure("claude", "Claude returned an unreadable response. Try again later.");
        } else if (status == 401 || status == 403) {
            failure("claude", "Claude login expired or access was denied. Open Claude Code to renew your login.");
        } else if (status == 429) {
            bool ok = false;
            const int retry = reply->rawHeader("Retry-After").toInt(&ok);
            m_claudeNextAttempt = QDateTime::currentSecsSinceEpoch() + qBound(300, ok ? retry : 300, 86400);
            failure("claude", "Claude requested a cooldown. Automatic refresh will resume after the pause.");
        } else failure("claude", status > 0 ? QString("Claude usage is unavailable (HTTP %1). Retrying automatically.").arg(status)
                                           : "Cannot reach Claude. Check your connection; retrying automatically.");
        reply->deleteLater();
    });
}

void Controller::fetchCodex() {
    QString program = codexPath();
    if (program.isEmpty()) program = executable("codex");
    if (program.isEmpty()) { failure("codex", "Codex CLI not found. Set its executable path in Settings."); return; }
    m_codexBusy = true; m_codex.insert("busy", true); emit statesChanged();
    if (m_codexProcess.isOpen()) {
        m_codexProcess.readAllStandardOutput(); m_codexProcess.readAllStandardError();
    }
#ifdef Q_OS_WIN
    // npm installs .cmd shims on Windows. The path is the program, never a shell command from a provider.
    if (program.endsWith(".cmd", Qt::CaseInsensitive) || program.endsWith(".bat", Qt::CaseInsensitive)) {
        m_codexProcess.setProgram(qEnvironmentVariable("COMSPEC", "cmd.exe"));
        QString safe = QDir::toNativeSeparators(program);
        if (safe.contains('"') || safe.contains('%') || safe.contains('\n') || safe.contains('\r')) {
            failure("codex", "Use a Codex executable path without quotes, percent signs, or line breaks."); return;
        }
        m_codexProcess.setNativeArguments("/d /s /c \"\"" + safe + "\" app-server\"");
        m_codexProcess.setArguments({});
    } else
#endif
    { m_codexProcess.setProgram(program); m_codexProcess.setArguments({"app-server"}); }
    m_codexProcess.start(); m_codexDeadline.start();
}
void Controller::sendRpc(const QJsonObject &message) {
    m_codexProcess.write(QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n');
}
void Controller::readCodex() {
    if (m_codexProcess.bytesAvailable() > 2 * 1024 * 1024) {
        failure("codex", "Codex returned an unexpectedly large response."); m_codexProcess.kill(); return;
    }
    while (m_codexBusy && m_codexProcess.canReadLine()) {
        const auto message = QJsonDocument::fromJson(m_codexProcess.readLine()).object();
        const int id = message.value("id").toInt(-1);
        if (id != 1 && id != 2 && id != 3) continue;
        if (id == 3) {
            // Model metadata is optional; failure must not block quota reads.
            const auto config = message.value("result").toObject().value("config").toObject();
            const QString model = config.value("model").toString().trimmed();
            m_codex.insert("model", model.isEmpty() ? "Auto" : model.left(128));
            emit statesChanged();
            sendRpc({{"id", 2}, {"method", "account/rateLimits/read"}});
            continue;
        }
        if (message.contains("error")) {
            failure("codex", "Codex could not read subscription limits. Check codex login and your connection.");
            m_codexProcess.kill(); return;
        }
        if (id == 1) {
            sendRpc({{"method", "initialized"}, {"params", QJsonObject{}}});
            sendRpc({{"id", 3}, {"method", "config/read"}, {"params", QJsonObject{{"includeLayers", false}}}});
        } else {
            const auto result = message.value("result").toObject();
            success("codex", Usage::codexRows(result), result.value("rateLimits").toObject().value("planType").toString());
            m_codexProcess.kill();
        }
    }
}
