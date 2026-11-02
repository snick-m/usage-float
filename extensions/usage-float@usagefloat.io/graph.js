import Cairo from 'cairo';

const ACCENTS = [[0.84, 0.67, 0.54], [0.65, 0.86, 0.79]];
const WARNING = [0.90, 0.74, 0.50];
const DANGER = [0.94, 0.56, 0.58];

// Fixed slots keep missing or extra model limits from shifting the graph legend.
export function graphBars(data) {
    return ['claude', 'codex'].flatMap((provider, group) => {
        const state = data?.[provider];
        return ['Session', 'Weekly'].map(label => {
            const row = state?.rows?.find(item => item.label === label);
            const valid = Number.isFinite(row?.displayPercent) && Number.isFinite(row?.percent);
            return {
                fraction: valid ? Math.max(0, Math.min(100, row.displayPercent)) / 100 : null,
                color: valid && row.percent >= 90 ? DANGER
                    : valid && row.percent >= 75 ? WARNING : ACCENTS[group],
                accent: ACCENTS[group],
                stale: Boolean(state?.stale || state?.status === 'error'),
            };
        });
    });
}

// Shared by the native St.DrawingArea and the standalone Cairo render test.
export function drawGraph(cr, width, height, bars) {
    cr.save();
    cr.rectangle(0, 0, width, height);
    cr.clip();
    cr.setOperator(Cairo.Operator.CLEAR);
    cr.paint();
    cr.setOperator(Cairo.Operator.OVER);
    cr.scale(width / 64, height / 24);
    for (let index = 0; index < bars.length; index++) {
        const bar = bars[index];
        const x = 3 + index * 14 + (index >= 2 ? 6 : 0);
        const y = 2, w = 10, h = 20;
        cr.setSourceRGBA(...bar.accent, 0.20);
        cr.rectangle(x, y, w, h);
        cr.fill();
        if (bar.fraction === null) {
            // Unknown is a crossed track, visibly different from a real zero.
            cr.setSourceRGBA(...bar.accent, 0.65);
            cr.setLineWidth(1.2);
            cr.moveTo(x + 3, y + 8); cr.lineTo(x + 7, y + 12);
            cr.moveTo(x + 7, y + 8); cr.lineTo(x + 3, y + 12);
            cr.stroke();
        } else {
            const filled = h * bar.fraction;
            cr.setSourceRGBA(...bar.color, bar.stale ? 0.55 : 1);
            cr.rectangle(x, y + h - filled, w, filled);
            cr.fill();
            // A baseline also makes a measured 0% legible.
            cr.setSourceRGBA(...bar.accent, 0.8);
            cr.rectangle(x, y + h - 1, w, 1);
            cr.fill();
        }
        if (bar.stale) {
            cr.setSourceRGBA(...WARNING, 1);
            cr.arc(x + w / 2, 1.5, 1.5, 0, Math.PI * 2);
            cr.fill();
        }
    }
    cr.restore();
}
