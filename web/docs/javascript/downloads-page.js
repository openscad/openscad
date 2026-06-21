(function() {
'use strict';

function initDownloadsPage()
{
  const el = document.getElementById('download-links');
  if (!el || !window.PythonSCADDownloads) {
    return;
  }

  const {
    fetchLatestRelease,
    groupAssetsByPlatform,
    formatSize,
    PLATFORM_ORDER,
    escapeHtml,
    safeGitHubDownloadUrl,
    beginProgressiveLoad,
    showProgressiveContent,
    restoreProgressiveFallback
  } = window.PythonSCADDownloads;

  beginProgressiveLoad(el, 'download-links');

  async function renderDownloadsPage()
  {
    try {
      const data = await fetchLatestRelease();
      const {byName, byPlatform} = groupAssetsByPlatform(data.assets || []);
      const tagName = escapeHtml(data.tag_name);

      let html = `<h3>Latest version: ${tagName}</h3>`;

      const titles = {
        windows: 'Windows',
        'linux-debian': 'Linux (Debian / Ubuntu)',
        'linux-fedora': 'Linux (Fedora / RHEL)',
        'linux-appimage': 'Linux (AppImage)',
        linux: 'Linux (other)',
        macos: 'macOS',
        wasm: 'WebAssembly / Browser',
        other: 'Other'
      };
      const sectionBlurbs = {
        wasm: 'Run PythonSCAD entirely in your browser — no installation required. ' +
          'The ZIP contains <code>pythonscad.js</code>, <code>pythonscad.wasm</code>, ' +
          '<code>pythonscad.data</code>, and <code>test.html</code>. ' +
          'Serve the extracted folder with the bundled dev server ' +
          '(<code>python3 wasm-test/serve.py 8080 .</code>) and open ' +
          '<code>test.html</code>. A plain <code>python3 -m http.server</code> ' +
          'will not work — browsers require correct MIME types for ' +
          '<code>.wasm</code> and <code>.data</code> files. ' +
          'A browser with WASM support (Chrome, Firefox, ' +
          'Edge, Safari 16+) is required.',
        windows: 'NSIS installer and MSIX packages are Authenticode-signed. ZIP archives ' +
          'are not signed. See the ' +
          '<a href="../installation/#windows">Windows installation instructions</a> ' +
          'for SmartScreen notes.',
        'linux-debian': 'The <a href="https://repos.pythonscad.org/apt/">APT repository</a> is the ' +
          'preferred way to install PythonSCAD on Debian and Ubuntu.',
        'linux-fedora':
          'The <a href="https://repos.pythonscad.org/yum/">YUM/DNF repository</a> is the ' +
          'preferred way to install PythonSCAD on Fedora and RHEL-based systems.'
      };

      for (const platform of PLATFORM_ORDER) {
        const list = byPlatform[platform];
        if (list.length === 0) {
          continue;
        }

        html += `<h4>${titles[platform]}</h4>`;
        if (sectionBlurbs[platform]) {
          html += `<p class="downloads-repo-note">${sectionBlurbs[platform]}</p>`;
        }
        html += `<table class="downloads-table"><thead><tr><th>File</th><th>Size</th>`;
        html += `<th>SHA256</th><th>SHA512</th></tr></thead><tbody>`;

        list.forEach(asset => {
          const sha256Asset = byName[asset.name + '.sha256'];
          const sha512Asset = byName[asset.name + '.sha512'];
          const assetUrl = safeGitHubDownloadUrl(asset.browser_download_url);
          const assetName = escapeHtml(asset.name);
          html += '<tr>';
          html += `<td><a href="${assetUrl}">${assetName}</a></td>`;
          html += `<td>${formatSize(asset.size)}</td>`;
          html += `<td>${
            sha256Asset ?
              `<a href="${safeGitHubDownloadUrl(sha256Asset.browser_download_url)}">checksum</a>` :
              '—'}</td>`;
          html += `<td>${
            sha512Asset ?
              `<a href="${safeGitHubDownloadUrl(sha512Asset.browser_download_url)}">checksum</a>` :
              '—'}</td>`;
          html += '</tr>';
        });
        html += '</tbody></table>';
      }

      showProgressiveContent(el, 'download-links', html);
    } catch (e) {
      restoreProgressiveFallback(
        el, 'download-links', `Failed to load release info: ${escapeHtml(e.message)}`);
    }
  }

  renderDownloadsPage();
}

if (window.PythonSCADDownloads) {
  window.PythonSCADDownloads.onMkDocsPageLoad(initDownloadsPage);
} else {
  document.addEventListener('DOMContentLoaded', initDownloadsPage);
}
})();
