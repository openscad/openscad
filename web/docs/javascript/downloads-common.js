(function() {
'use strict';

const REPO = 'pythonscad/pythonscad';
const GITHUB_RELEASES_LATEST = `https://github.com/${REPO}/releases/latest`;
const GITHUB_RELEASES = `https://github.com/${REPO}/releases`;
const PLATFORM_ORDER =
  ['windows', 'linux-debian', 'linux-fedora', 'linux-appimage', 'linux', 'macos', 'wasm', 'other'];

function escapeHtml(text)
{
  return String(text)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}

function safeGitHubDownloadUrl(url)
{
  try {
    const parsed = new URL(url);
    if (parsed.protocol === 'https:' && parsed.hostname === 'github.com') {
      return parsed.href;
    }
  } catch (e) {
    // ignore invalid URLs
  }
  return '#';
}

function getPlatform(assetName)
{
  const n = assetName.toLowerCase();
  if (n.endsWith('.exe') || n.endsWith('.msix') || /windows|win-|win_|-win\.|-win_/.test(n)) {
    return 'windows';
  }
  if (n.endsWith('.dmg') || /macos|darwin|osx|-mac\.|-mac_/.test(n)) {
    return 'macos';
  }
  if (n.endsWith('.appimage') || /appimage/.test(n)) {
    return 'linux-appimage';
  }
  if (n.endsWith('.deb') || /debian|ubuntu|apt/.test(n)) {
    return 'linux-debian';
  }
  if (n.endsWith('.rpm') || /fedora|redhat|rhel|centos|yum|dnf/.test(n)) {
    return 'linux-fedora';
  }
  if (/linux/.test(n)) {
    return 'linux';
  }
  if (n.endsWith('.zip') && /wasm|webassembly/.test(n)) {
    return 'wasm';
  }
  return 'other';
}

function isChecksumOrMeta(name)
{
  return name.endsWith('.sha256') || name.endsWith('.sha512') || name.endsWith('.asc') ||
    name === 'checksums.txt';
}

function formatSize(bytes)
{
  if (bytes >= 1024 * 1024) {
    return (bytes / 1024 / 1024).toFixed(1) + ' MB';
  }
  if (bytes >= 1024) {
    return (bytes / 1024).toFixed(0) + ' KB';
  }
  return bytes + ' B';
}

function groupAssetsByPlatform(assets)
{
  const byName = {};
  assets.forEach(a => {
    byName[a.name] = a;
  });

  const mainAssets = assets.filter(a => !isChecksumOrMeta(a.name));
  const byPlatform = {
    windows: [],
    'linux-debian': [],
    'linux-fedora': [],
    'linux-appimage': [],
    linux: [],
    macos: [],
    wasm: [],
    other: []
  };
  mainAssets.forEach(a => {
    byPlatform[getPlatform(a.name)].push(a);
  });

  return {byName, byPlatform, mainAssets};
}

let latestReleasePromise = null;
let latestReleaseCache = null;

async function fetchLatestRelease()
{
  if (latestReleaseCache) {
    return latestReleaseCache;
  }
  if (!latestReleasePromise) {
    latestReleasePromise = fetch(`https://api.github.com/repos/${REPO}/releases/latest`)
                             .then(response => {
                               if (!response.ok) {
                                 throw new Error(`GitHub API returned ${response.status}`);
                               }
                               return response.json();
                             })
                             .then(data => {
                               latestReleaseCache = data;
                               return data;
                             })
                             .catch(err => {
                               latestReleasePromise = null;
                               throw err;
                             });
  }
  return latestReleasePromise;
}

function detectUserPlatform()
{
  const ua = navigator.userAgent.toLowerCase();
  const platform =
    (navigator.userAgentData && navigator.userAgentData.platform ? navigator.userAgentData.platform :
                                                                   navigator.platform || '')
      .toLowerCase();

  if (/^win/i.test(platform) || /windows/.test(ua)) {
    return 'windows';
  }
  if (/iphone|ipad|ipod/.test(ua)) {
    return null;
  }
  if (/mac/.test(platform) || /macintosh/.test(ua)) {
    return 'macos';
  }
  if (/android/.test(ua)) {
    return null;
  }
  if (/freebsd|openbsd|netbsd|dragonfly/.test(ua)) {
    return null;
  }
  if (/linux/.test(platform) || /linux/.test(ua)) {
    if (/arch|gentoo|slackware|void linux|nixos|alpine/.test(ua)) {
      return 'linux';
    }
    if (/fedora|rhel|centos|rocky|alma|suse|red hat/.test(ua)) {
      return 'linux-fedora';
    }
    if (/ubuntu|debian|mint|pop!_os|elementary|zorin|kubuntu|xubuntu|lubuntu/.test(ua)) {
      return 'linux-debian';
    }
    return 'linux';
  }
  return null;
}

const PLATFORM_LABELS = {
  windows: 'Windows',
  'linux-debian': 'Linux (Debian / Ubuntu)',
  'linux-fedora': 'Linux (Fedora / RHEL)',
  'linux-appimage': 'Linux (AppImage)',
  linux: 'Linux',
  macos: 'macOS',
  wasm: 'WebAssembly / Browser',
  other: 'your platform'
};

function pickAssetForPlatform(byPlatform, preferredPlatform)
{
  const fallbacks = {
    windows: ['windows'],
    macos: ['macos'],
    'linux-debian': ['linux-debian', 'linux-appimage', 'linux'],
    'linux-fedora': ['linux-fedora', 'linux-appimage', 'linux'],
    'linux-appimage': ['linux-appimage', 'linux-debian', 'linux'],
    linux: ['linux-appimage', 'linux', 'linux-debian'],
    wasm: ['wasm'],
    other: PLATFORM_ORDER.filter(p => p !== 'other')
  };

  const order =
    preferredPlatform ? (fallbacks[preferredPlatform] || [preferredPlatform]) : PLATFORM_ORDER;
  for (const platform of order) {
    const list = byPlatform[platform];
    if (list && list.length > 0) {
      return {asset: list[0], platform};
    }
  }
  return null;
}

function onMkDocsPageLoad(callback)
{
  if (typeof document$ !== 'undefined') {
    document$.subscribe(callback);
    return;
  }
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', callback);
    return;
  }
  callback();
}

function beginProgressiveLoad(container, prefix)
{
  const fallback = container.querySelector(`.${prefix}-fallback`);
  const loading = container.querySelector(`.${prefix}-loading`);
  const enhanced = container.querySelector(`.${prefix}-enhanced`);
  if (fallback) {
    fallback.querySelectorAll(`.${prefix}-error`).forEach(el => el.remove());
    fallback.hidden = true;
  }
  if (loading) {
    loading.hidden = false;
  }
  if (enhanced) {
    enhanced.hidden = true;
    enhanced.innerHTML = '';
  }
}

function showProgressiveContent(container, prefix, html)
{
  const loading = container.querySelector(`.${prefix}-loading`);
  const enhanced = container.querySelector(`.${prefix}-enhanced`);
  if (loading) {
    loading.hidden = true;
  }
  if (enhanced) {
    enhanced.innerHTML = html;
    enhanced.hidden = false;
  }
}

function restoreProgressiveFallback(container, prefix, messageHtml)
{
  const fallback = container.querySelector(`.${prefix}-fallback`);
  const loading = container.querySelector(`.${prefix}-loading`);
  const enhanced = container.querySelector(`.${prefix}-enhanced`);
  if (loading) {
    loading.hidden = true;
  }
  if (enhanced) {
    enhanced.hidden = true;
    enhanced.innerHTML = '';
  }
  if (fallback) {
    fallback.hidden = false;
    fallback.querySelectorAll(`.${prefix}-error`).forEach(el => el.remove());
    if (messageHtml) {
      const err = document.createElement('p');
      err.className = `${prefix}-error`;
      err.innerHTML = messageHtml;
      fallback.insertBefore(err, fallback.firstChild);
    }
  }
}

window.PythonSCADDownloads = {
  REPO,
  GITHUB_RELEASES_LATEST,
  GITHUB_RELEASES,
  PLATFORM_ORDER,
  PLATFORM_LABELS,
  escapeHtml,
  safeGitHubDownloadUrl,
  getPlatform,
  isChecksumOrMeta,
  formatSize,
  groupAssetsByPlatform,
  fetchLatestRelease,
  detectUserPlatform,
  pickAssetForPlatform,
  onMkDocsPageLoad,
  beginProgressiveLoad,
  showProgressiveContent,
  restoreProgressiveFallback
};
})();
