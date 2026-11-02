import {panelText, detailLines} from '../extensions/usage-float@usagefloat.io/format.js';

function assert(value, message) { if (!value) throw new Error(message); }
const state = {rows: [
    {label: 'Session', percent: 32, displayPercent: 32, countdown: '2h'},
    {label: 'Weekly', percent: 58, displayPercent: 58, countdown: '3d'},
], status: 'ready', model: 'Auto'};
const data = {claude: state, codex: {...state, rows: []}};
assert(panelText(data) === 'Claude S 32% W 58%   ·   Codex —', 'Missing limits must not become zero');
data.codex.busy = true;
assert(panelText(data).includes('Codex …'), 'Loading indicator');
data.claude = {...state, stale: true, status: 'error', message: 'Cannot reach Claude', age: '8m ago'};
assert(panelText(data).includes('58% !'), 'Stale warning');
assert(detailLines(data).includes('Stale reading · 8m ago'), 'Stale age');
assert(detailLines(data).includes('Cannot reach Claude'), 'Provider error');
data.showRemaining = true;
data.claude = {...state, rows: state.rows.map(row => ({...row, displayPercent: 100 - row.percent}))};
assert(panelText(data).includes('S 68% W 42%'), 'Remaining values');
assert(detailLines(data)[0] === 'Remaining allowance', 'Remaining label');
data.version = '0.2.0-rc.2';
assert(detailLines(data).includes('Usage Float v0.2.0-rc.2'), 'App version including prerelease');
print('Panel formatting passed');
