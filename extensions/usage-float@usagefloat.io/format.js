function percent(row) {
    return `${Math.round(row.displayPercent)}%`;
}

export function panelText(data) {
    return [['Claude', data.claude], ['Codex', data.codex]].map(([name, state]) => {
        const rows = (state.rows ?? []).slice(0, 2);
        const values = rows.map(row => `${row.label === 'Session' ? 'S' : row.label === 'Weekly' ? 'W' : row.label} ${percent(row)}`);
        const warning = state.stale || state.status === 'error' ? ' !' : '';
        return `${name} ${values.length ? values.join(' ') : state.busy ? '…' : '—'}${warning}`;
    }).join('   ·   ');
}

export function detailLines(data) {
    const lines = [data.showRemaining ? 'Remaining allowance' : 'Used allowance'];
    for (const [name, state] of [['Claude', data.claude], ['Codex', data.codex]]) {
        lines.push(`${name} · ${state.model || 'Auto'}`);
        for (const row of state.rows ?? [])
            lines.push(`${row.label}: ${percent(row)} · resets in ${row.countdown}`);
        if (state.stale) lines.push(`Stale reading · ${state.age}`);
        if (state.status === 'error') lines.push(state.message || 'Usage unavailable');
        else if (!(state.rows ?? []).length) lines.push(state.busy ? 'Syncing…' : 'Usage unavailable');
    }
    if (data.version) lines.push(`Usage Float v${data.version}`);
    return lines;
}
