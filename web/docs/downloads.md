# Downloads

<!-- markdownlint-disable MD033 -->

<div id="download-links">Loading latest version...</div>

<script>
(function() {
    const REPO = "pythonscad/pythonscad";
    const PLATFORM_ORDER = ["windows", "linux-debian", "linux-fedora", "linux-appimage", "linux", "macos", "other"];

    function getPlatform(assetName) {
        const n = assetName.toLowerCase();
        if (n.endsWith(".exe") || /windows|win-|win_|-win\.|-win_/.test(n)) return "windows";
        if (n.endsWith(".dmg") || /macos|darwin|osx|-mac\.|-mac_/.test(n)) return "macos";
        if (n.endsWith(".appimage") || /appimage/.test(n)) return "linux-appimage";
        if (n.endsWith(".deb") || /debian|ubuntu|apt/.test(n)) return "linux-debian";
        if (n.endsWith(".rpm") || /fedora|redhat|rhel|centos|yum|dnf/.test(n)) return "linux-fedora";
        if (/linux/.test(n)) return "linux";
        return "other";
    }

    function isChecksumOrMeta(name) {
        return name.endsWith(".sha256") || name.endsWith(".sha512") ||
            name.endsWith(".asc") || name === "checksums.txt";
    }

    function formatSize(bytes) {
        if (bytes >= 1024 * 1024) return (bytes / 1024 / 1024).toFixed(1) + " MB";
        if (bytes >= 1024) return (bytes / 1024).toFixed(0) + " KB";
        return bytes + " B";
    }

    async function getLatestRelease() {
        const el = document.getElementById("download-links");
        try {
            const response = await fetch(`https://api.github.com/repos/${REPO}/releases/latest`);
            const data = await response.json();
            const assets = data.assets || [];

            const byName = {};
            assets.forEach(a => { byName[a.name] = a; });

            const mainAssets = assets.filter(a => !isChecksumOrMeta(a.name));
            const byPlatform = {
                windows: [], "linux-debian": [], "linux-fedora": [], "linux-appimage": [],
                linux: [], macos: [], other: []
            };
            mainAssets.forEach(a => {
                const platform = getPlatform(a.name);
                byPlatform[platform].push(a);
            });

            let html = `<h3>Latest version: ${data.tag_name}</h3>`;

            for (const platform of PLATFORM_ORDER) {
                const list = byPlatform[platform];
                if (list.length === 0) continue;

                const titles = {
                    windows: "Windows",
                    "linux-debian": "Linux (Debian / Ubuntu)",
                    "linux-fedora": "Linux (Fedora / RHEL)",
                    "linux-appimage": "Linux (AppImage)",
                    linux: "Linux (other)",
                    macos: "macOS",
                    other: "Other"
                };
                const sectionBlurbs = {
                    "linux-debian":
                        "The <a href=\"https://repos.pythonscad.org/apt/\">APT repository</a> is the "
                        + "preferred way to install PythonSCAD on Debian and Ubuntu.",
                    "linux-fedora":
                        "The <a href=\"https://repos.pythonscad.org/yum/\">YUM/DNF repository</a> is the "
                        + "preferred way to install PythonSCAD on Fedora and RHEL-based systems."
                };
                html += `<h4>${titles[platform]}</h4>`;
                if (sectionBlurbs[platform]) {
                    html += `<p class="downloads-repo-note">${sectionBlurbs[platform]}</p>`;
                }
                html += `<table class="downloads-table"><thead><tr><th>File</th><th>Size</th><th>SHA256</th><th>SHA512</th></tr></thead><tbody>`;

                list.forEach(asset => {
                    const sha256Asset = byName[asset.name + ".sha256"];
                    const sha512Asset = byName[asset.name + ".sha512"];
                    html += "<tr>";
                    html += `<td><a href="${asset.browser_download_url}">${asset.name}</a></td>`;
                    html += `<td>${formatSize(asset.size)}</td>`;
                    html += `<td>${sha256Asset ? `<a href="${sha256Asset.browser_download_url}">checksum</a>` : "—"}</td>`;
                    html += `<td>${sha512Asset ? `<a href="${sha512Asset.browser_download_url}">checksum</a>` : "—"}</td>`;
                    html += "</tr>";
                });
                html += "</tbody></table>";
            }

            el.innerHTML = html;
        } catch (e) {
            el.innerHTML = `<p>Failed to load release info: ${e.message}</p>`;
        }
    }

    getLatestRelease();
})();
</script>

<!-- markdownlint-enable MD033 -->
