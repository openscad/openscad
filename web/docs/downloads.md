
# Downloads

<div id="download-links">Loading latest version...</div>

<script>
async function getLatestRelease() {
    const repo = "pythonscad/pythonscad";
    const response = await fetch(`https://api.github.com/repos/${repo}/releases/latest`);
    const data = await response.json();

    let html = `<h3>Latest Version: ${data.tag_name}</h3><ul>`;

    data.assets.forEach(asset => {
        html += `<li><a href="${asset.browser_download_url}">${asset.name}</a> (${(asset.size/1024/1024).toFixed(1)} MB)</li>`;
    });

    html += "</ul>";
    document.getElementById("download-links").innerHTML = html;
}

getLatestRelease();
</script>
