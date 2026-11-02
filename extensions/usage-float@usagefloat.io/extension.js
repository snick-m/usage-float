import Clutter from 'gi://Clutter';
import Gio from 'gi://Gio';
import St from 'gi://St';
import {Extension} from 'resource:///org/gnome/shell/extensions/extension.js';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import * as PanelMenu from 'resource:///org/gnome/shell/ui/panelMenu.js';
import * as PopupMenu from 'resource:///org/gnome/shell/ui/popupMenu.js';
import {panelText, detailLines} from './format.js';
import {graphBars, drawGraph} from './graph.js';

const Proxy = Gio.DBusProxy.makeProxyWrapper(`<node>
  <interface name="io.usagefloat.UsageFloat.Panel">
    <method name="GetSnapshot"><arg type="s" direction="out" name="json"/></method>
    <method name="Show"/><method name="Hide"/><method name="Refresh"/>
    <method name="SetShowRemaining"><arg type="b" direction="in" name="remaining"/></method>
    <method name="SetPanelActive"><arg type="b" direction="in" name="active"/></method>
    <signal name="SnapshotChanged"><arg type="s" name="json"/></signal>
  </interface>
</node>`);

export default class UsageFloatExtension extends Extension {
    enable() {
        this._cancel = new Gio.Cancellable();
        const cancellable = this._cancel;
        this._button = new PanelMenu.Button(0.0, 'Usage Float');
        this._bars = graphBars(null);
        this._graph = new St.DrawingArea({
            width: 64, height: 24, y_align: Clutter.ActorAlign.CENTER,
        });
        this._graph.connect('repaint', area => {
            const cr = area.get_context();
            try {
                const [width, height] = area.get_surface_size();
                drawGraph(cr, width, height, this._bars);
            } finally {
                cr.$dispose();
            }
        });
        this._button.add_child(this._graph);
        this._details = new PopupMenu.PopupMenuSection();
        this._button.menu.addMenuItem(this._details);
        this._button.menu.addMenuItem(new PopupMenu.PopupSeparatorMenuItem());
        this._button.menu.addAction('Open Usage Float', () => this._open());
        this._refresh = this._button.menu.addAction('Refresh usage', () => this._call('Refresh'));
        this._hide = this._button.menu.addAction('Hide floating widget', () => this._call('Hide'));
        this._remaining = new PopupMenu.PopupSwitchMenuItem('Show remaining allowance', false);
        this._remaining.connect('toggled', (_item, value) => this._call('SetShowRemaining', value));
        this._button.menu.addMenuItem(this._remaining);
        // Dash to Panel reuses Main.panel when the stock top panel is disabled.
        // Use GNOME's panel API so the graph lives directly in that taskbar.
        Main.panel.addToStatusArea(this.uuid, this._button, 0, 'right');
        this._offline();
        new Proxy(Gio.DBus.session, 'io.usagefloat.UsageFloat.Panel',
            '/io/usagefloat/UsageFloat/Panel', (proxy, error) => {
                if (cancellable.is_cancelled()) return;
                if (error) { console.warn(`Usage Float: ${error.message}`); return; }
                this._proxy = proxy;
                this._ownerId = proxy.connect('notify::g-name-owner', () => this._load());
                this._signalId = proxy.connectSignal('SnapshotChanged', (_p, _sender, [json]) => this._render(json));
                this._load();
            }, cancellable, Gio.DBusProxyFlags.DO_NOT_AUTO_START);
    }

    _load() {
        if (!this._proxy?.g_name_owner) { this._offline(); return; }
        this._call('SetPanelActive', true);
        const proxy = this._proxy;
        const owner = proxy.g_name_owner;
        proxy.GetSnapshotRemote((result, error) => {
            if (!this._button || this._proxy !== proxy || proxy.g_name_owner !== owner) return;
            if (error) this._offline();
            else this._render(result[0]);
        });
    }

    _render(json) {
        if (!this._button) return;
        try {
            const data = JSON.parse(json);
            this._bars = graphBars(data);
            this._graph.queue_repaint();
            this._button.accessible_name = `Usage Float: ${panelText(data)}`;
            this._details.removeAll();
            this._details.addMenuItem(new PopupMenu.PopupMenuItem(
                'Left: Claude · Right: Codex — each pair: session, weekly', {reactive: false}));
            for (const line of detailLines(data))
                this._details.addMenuItem(new PopupMenu.PopupMenuItem(line, {reactive: false}));
            this._remaining.setToggleState(Boolean(data.showRemaining));
            for (const item of [this._refresh, this._hide, this._remaining]) item.setSensitive(true);
        } catch (error) {
            console.warn(`Usage Float: invalid panel data: ${error.message}`);
            this._offline();
        }
    }

    _offline() {
        this._bars = graphBars(null);
        this._graph.queue_repaint();
        this._button.accessible_name = 'Usage Float · offline';
        this._details.removeAll();
        this._details.addMenuItem(new PopupMenu.PopupMenuItem('Open Usage Float to start monitoring', {reactive: false}));
        for (const item of [this._refresh, this._hide, this._remaining]) item.setSensitive(false);
    }

    _call(method, ...args) {
        if (!this._proxy?.g_name_owner) return;
        const proxy = this._proxy;
        proxy[`${method}Remote`](...args, (_result, error) => {
            if (error && this._button && this._proxy === proxy)
                console.warn(`Usage Float: ${method} failed: ${error.message}`);
        });
    }

    _open() {
        if (this._proxy?.g_name_owner) { this._call('Show'); return; }
        const app = Gio.DesktopAppInfo.new('io.usagefloat.UsageFloat.desktop');
        if (app) app.launch([], null);
        else Main.notify('Usage Float', 'Install the desktop app first, then launch it from Applications.');
    }

    disable() {
        // Restore access to a hidden widget when its panel control is removed.
        this._call('Show');
        this._call('SetPanelActive', false);
        this._cancel.cancel();
        if (this._proxy) {
            this._proxy.disconnect(this._ownerId);
            this._proxy.disconnectSignal(this._signalId);
            this._proxy = null;
        }
        this._button?.destroy();
        this._button = null;
    }
}
