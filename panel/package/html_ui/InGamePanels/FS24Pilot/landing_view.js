/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 * Licensed under the MIT License. See the LICENSE file in the project root.
 */

// View: renders a landing view model into the panel. Builds its own DOM inside `root`, so
// the panel HTML and the dev preview share one structure.

const FS24_BASE_FONT_VH = 1.7; // text size at 100 %, as in FS24Pilot.css
const FS24_SVG_NS = "http://www.w3.org/2000/svg";
const FS24_RUNWAY_W = 400;
const FS24_RUNWAY_H = 46;
const FS24_TOUCHDOWN_ZONE_M = 300;
const FS24_LATERAL_SPAN_M = 20; // lateral deviation is exaggerated so metres stay visible

function fs24Element(tag, className, text) {
    const element = document.createElement(tag);
    if (className) {
        element.className = className;
    }
    if (text !== undefined) {
        element.textContent = text;
    }
    return element;
}

function fs24SvgElement(tag, attributes) {
    const element = document.createElementNS(FS24_SVG_NS, tag);
    Object.keys(attributes).forEach((name) => element.setAttribute(name, attributes[name]));
    return element;
}

class Fs24LandingView {
    constructor(root) {
        root.textContent = "";
        this.card = fs24Element("div", "card neutral");
        this.banner = fs24Element("div", "banner");
        this.gradeLabel = fs24Element("div", "grade-label", "NO LANDING YET");
        this.banner.appendChild(this.gradeLabel);
        this.metrics = fs24Element("div", "metrics");
        this.flare = fs24Element("div", "flare");
        this.context = fs24Element("div", "context");
        this.runway = fs24Element("div", "runway");
        [this.banner, this.metrics, this.flare, this.context, this.runway].forEach((part) =>
            this.card.appendChild(part),
        );

        this.footer = fs24Element("div", "footer");
        this.status = fs24Element("span", "status", "waiting for module…");
        this.scaleButtons = fs24Element("span", "scale-buttons");
        this.smaller = fs24Element("span", "scale-button", "A−");
        this.bigger = fs24Element("span", "scale-button", "A+");
        this.scaleButtons.appendChild(this.smaller);
        this.scaleButtons.appendChild(this.bigger);
        this.module = fs24Element("span", "module");
        [this.status, this.scaleButtons, this.module].forEach((part) => this.footer.appendChild(part));

        this.root = root;
        root.appendChild(this.card);
        root.appendChild(this.footer);
        this.showEmpty();
    }

    // `onStep(direction)` is called with +1 for A+ and −1 for A−.
    onScaleStep(onStep) {
        this.smaller.addEventListener("click", () => onStep(-1));
        this.bigger.addEventListener("click", () => onStep(1));
    }

    // Every size in FS24Pilot.css is in em of the root, so its font size scales everything.
    showScale(percent) {
        this.root.style.fontSize = `${(FS24_BASE_FONT_VH * percent) / 100}vh`;
    }

    showStatus(text) {
        this.status.textContent = text;
    }

    showModule(version, profile) {
        this.module.textContent = profile ? `${profile} · v${version}` : `v${version}`;
    }

    showEmpty() {
        this.setGradeClass("neutral");
        this.gradeLabel.textContent = "NO LANDING YET";
        this.metrics.textContent = "";
        this.flare.textContent = "Land somewhere — the rating appears here.";
        this.context.textContent = "";
        this.runway.textContent = "";
    }

    showLanding(model) {
        this.setGradeClass(model.gradeClass);
        this.gradeLabel.textContent = model.grade;
        this.renderMetrics(model.rows);
        this.flare.textContent = model.flare;
        this.renderContext(model.context);
        this.renderRunway(model.showRunway, model.runway);
    }

    setGradeClass(gradeClass) {
        this.card.className = `card ${gradeClass}`;
    }

    renderMetrics(rows) {
        this.metrics.textContent = "";
        const columns = Math.max(...rows.map((row) => row.length));
        rows.forEach((row) => {
            const line = fs24Element("div", "metric-row");
            row.forEach((metric) => {
                const cell = fs24Element("div", `metric columns-${columns}`);
                cell.appendChild(fs24Element("div", "metric-label", metric.label));
                cell.appendChild(fs24Element("div", `metric-value ${metric.tone}`, metric.value));
                line.appendChild(cell);
            });
            this.metrics.appendChild(line);
        });
    }

    renderContext(parts) {
        this.context.textContent = "";
        parts.forEach((part, index) => {
            if (index > 0) {
                this.context.appendChild(fs24Element("span", "separator", "·"));
            }
            this.context.appendChild(fs24Element("span", part.tone, part.text));
        });
    }

    renderRunway(show, runway) {
        this.runway.textContent = "";
        if (!show) {
            return;
        }
        if (!runway) {
            this.runway.appendChild(fs24Element("div", "runway-caption", "No runway data"));
            return;
        }
        this.runway.appendChild(fs24Element("div", "runway-title", runway.title));
        this.runway.appendChild(this.runwayDiagram(runway));
        this.runway.appendChild(fs24Element("div", "runway-caption", runway.caption));
    }

    // Plan view: along-track to scale, the first 300 m tinted as the touchdown zone.
    runwayDiagram(runway) {
        const svg = fs24SvgElement("svg", {
            class: "runway-diagram",
            viewBox: `0 0 ${FS24_RUNWAY_W} ${FS24_RUNWAY_H}`,
        });
        const mid = FS24_RUNWAY_H / 2;
        const zoneW = Math.min(FS24_RUNWAY_W, (FS24_TOUCHDOWN_ZONE_M / runway.lengthM) * FS24_RUNWAY_W);
        svg.appendChild(fs24SvgElement("rect", { class: "runway-strip", width: FS24_RUNWAY_W, height: FS24_RUNWAY_H, rx: 3 }));
        svg.appendChild(fs24SvgElement("rect", { class: "runway-zone", width: zoneW, height: FS24_RUNWAY_H, rx: 3 }));
        svg.appendChild(
            fs24SvgElement("line", { class: "runway-centerline", x1: 12, y1: mid, x2: FS24_RUNWAY_W - 12, y2: mid }),
        );
        svg.appendChild(fs24SvgElement("rect", { class: "runway-threshold", width: 4, height: FS24_RUNWAY_H }));

        const lateral = Math.min(FS24_LATERAL_SPAN_M, Math.max(-FS24_LATERAL_SPAN_M, runway.offsetM));
        svg.appendChild(
            fs24SvgElement("circle", {
                class: "runway-touchdown",
                cx: runway.alongRatio * FS24_RUNWAY_W,
                cy: mid + (lateral / FS24_LATERAL_SPAN_M) * mid,
                r: 6,
            }),
        );
        return svg;
    }
}
