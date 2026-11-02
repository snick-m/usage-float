import Cairo from 'cairo';
import {graphBars, drawGraph} from '../extensions/usage-float@usagefloat.io/graph.js';

function assert(value, message) { if (!value) throw new Error(message); }
const state = {status: 'ready', rows: [
    {label: 'Model limit', percent: 99, displayPercent: 99},
    {label: 'Weekly', percent: 58, displayPercent: 58},
    {label: 'Session', percent: 32, displayPercent: 32},
]};
const data = {claude: state, codex: {...state, rows: [
    {label: 'Session', percent: 16, displayPercent: 16},
    {label: 'Weekly', percent: 42, displayPercent: 42},
]}};
const used = graphBars(data);
assert(used.map(bar => bar.fraction).join(',') === '0.32,0.58,0.16,0.42', 'Fixed session/weekly order, independent of model limits');
const remaining = graphBars({...data, claude: {...state, rows: [
    {label: 'Session', percent: 95, displayPercent: 5},
    {label: 'Weekly', percent: 0, displayPercent: 100},
]}});
assert(remaining[0].fraction === 0.05, 'Use remaining values when selected');
assert(remaining[0].color[0] > remaining[0].color[1], 'Low remaining allowance retains red warning');
const missing = graphBars({claude: {status: 'error', stale: true, rows: [
    {label: 'Session', percent: 0, displayPercent: 0},
    {label: 'Weekly', percent: 12, displayPercent: null},
]}});
assert(missing[0].fraction === 0, 'Real zero stays zero');
assert(missing[1].fraction === null && missing[2].fraction === null, 'Missing and null data stay unknown');
assert(missing[0].stale, 'Stale data is visually flagged');
assert(graphBars(null).every(bar => bar.fraction === null), 'Offline graph has no fabricated values');
assert(graphBars({claude: {rows: [{label: 'Session', percent: 110, displayPercent: 110}]}})[0].fraction === 1, 'Clamp to graph bounds');

// Exercise the same Cairo renderer used by GNOME on actual image surfaces.
const surface = new Cairo.ImageSurface(Cairo.Format.ARGB32, 256, 192);
const cr = new Cairo.Context(surface);
cr.setSourceRGB(0.07, 0.09, 0.13);
cr.paint();
for (const [index, bars] of [used, remaining, missing, graphBars(null)].entries()) {
    cr.save();
    cr.translate(12, 12 + index * 44);
    drawGraph(cr, 96, 36, bars);
    cr.restore();
}
if (ARGV[0]) surface.writeToPNG(ARGV[0]);
cr.$dispose();
print('Panel graph mapping and Cairo rendering passed');
