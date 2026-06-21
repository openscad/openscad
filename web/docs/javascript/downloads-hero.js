(function() {
'use strict';

function initHeroDownload()
{
  const el = document.getElementById('hero-download');
  if (!el || !window.PythonSCADDownloads) {
    return;
  }

  const {
    fetchLatestRelease,
    groupAssetsByPlatform,
    detectUserPlatform,
    pickAssetForPlatform,
    PLATFORM_LABELS,
    escapeHtml,
    safeGitHubDownloadUrl,
    beginProgressiveLoad,
    showProgressiveContent,
    restoreProgressiveFallback
  } = window.PythonSCADDownloads;

  beginProgressiveLoad(el, 'hero-download');

  async function renderHeroDownload()
  {
    try {
      const data = await fetchLatestRelease();
      const {byPlatform} = groupAssetsByPlatform(data.assets || []);
      const userPlatform = detectUserPlatform();
      const tagName = escapeHtml(data.tag_name);

      let html = `<p class="hero-download-version">Latest version: <strong>${tagName}</strong></p>`;
      html += `<p class="hero-download-actions">`;

      if (userPlatform === 'linux-debian') {
        html += `<a class="md-button md-button--primary hero-download-button" `;
        html += `href="installation/#linux-debian-ubuntu-apt">Install via APT</a>`;
        html += ` <a class="md-button hero-download-all" href="downloads/">All downloads</a>`;
        html += `</p>`;
        html += `<p class="hero-download-file">Recommended: `;
        html += `<a href="https://repos.pythonscad.org/apt/">repos.pythonscad.org/apt</a></p>`;
      } else if (userPlatform === 'linux-fedora') {
        html += `<a class="md-button md-button--primary hero-download-button" `;
        html += `href="installation/#linux-fedora-centos-rhel-yum-dnf">Install via YUM/DNF</a>`;
        html += ` <a class="md-button hero-download-all" href="downloads/">All downloads</a>`;
        html += `</p>`;
        html += `<p class="hero-download-file">Recommended: `;
        html += `<a href="https://repos.pythonscad.org/yum/">repos.pythonscad.org/yum</a></p>`;
      } else if (userPlatform === 'linux') {
        html += `<a class="md-button md-button--primary hero-download-button" `;
        html += `href="installation/">Linux installation options</a>`;
        html += ` <a class="md-button hero-download-all" href="downloads/">All downloads</a>`;
        html += `</p>`;
      } else if (userPlatform === null) {
        html += `<a class="md-button md-button--primary hero-download-button" href="downloads/">`;
        html += `Download PythonSCAD</a>`;
        html += `</p>`;
      } else {
        const picked = pickAssetForPlatform(byPlatform, userPlatform);

        if (picked) {
          const label = escapeHtml(PLATFORM_LABELS[picked.platform] || picked.platform);
          const url = safeGitHubDownloadUrl(picked.asset.browser_download_url);
          const fileName = escapeHtml(picked.asset.name);
          html += `<a class="md-button md-button--primary hero-download-button" `;
          html += `href="${url}">`;
          html += `Download for ${label}</a>`;
          html += ` <a class="md-button hero-download-all" href="downloads/">All downloads</a>`;
          html += `</p>`;
          html += `<p class="hero-download-file">${fileName}</p>`;
        } else {
          html += `<a class="md-button md-button--primary hero-download-button" href="downloads/">`;
          html += `Download PythonSCAD</a>`;
          html += `</p>`;
        }
      }

      showProgressiveContent(el, 'hero-download', html);
    } catch (e) {
      restoreProgressiveFallback(
        el, 'hero-download', `Could not load release info. <a href="downloads/">Browse downloads</a>.`);
    }
  }

  renderHeroDownload();
}

if (window.PythonSCADDownloads) {
  window.PythonSCADDownloads.onMkDocsPageLoad(initHeroDownload);
} else {
  document.addEventListener('DOMContentLoaded', initHeroDownload);
}
})();
