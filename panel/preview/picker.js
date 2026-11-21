/* Preview controls: one button per sample landing, ?sample=<name> for the start state. */

(function () {
    const picker = document.getElementById("Picker");
    const show = (name) => {
        fs24Preview.sample = FS24_SAMPLES[name];
        if (fs24Preview.sample) {
            fs24Preview.deliver("landing", fs24Preview.sample);
        }
    };
    Object.keys(FS24_SAMPLES).forEach((name) => {
        const button = document.createElement("button");
        button.textContent = name;
        button.onclick = () => (name === "none" ? location.reload() : show(name));
        picker.appendChild(button);
    });

    const start = new URLSearchParams(location.search).get("sample");
    fs24Preview.sample = start ? FS24_SAMPLES[start] : FS24_SAMPLES.c172_great;
    const width = new URLSearchParams(location.search).get("width");
    if (width) {
        document.getElementById("Frame").style.width = `${width}px`;
        document.getElementById("Frame").style.height = `${Math.round(width * 1.3)}px`;
    }
    if (new URLSearchParams(location.search).has("bare")) {
        picker.style.display = "none";
    }
})();
